
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule() {
	PROCESS *p;
	int greatest_ticks = 0;

    //ticks决定哪个进程该被执行
	while (!greatest_ticks) {
		for (p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++) {
			//正在睡眠/阻塞的进程不会被执行
			if (p->sleeping > 0 || p->blocked == 1){
                continue;
            }
            //找到ticks最大的进程
			if (p->ticks > greatest_ticks) {
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

		//全是0则重新设置tick，避免有的进程一直不被执行
		if (!greatest_ticks) {
			for (p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++) {
                //说明被阻塞
				if (p->ticks > 0){
                    continue;
                }
				p->ticks = p->priority;
			}
		}
	}
}

//todo: 增加的系统调用
PUBLIC void p_process(SEMAPHORE *s) {
	disable_int();
	s->value--;
    // value < 0,说明原来没有足够的资源分配，因此需要阻塞
	if (s->value < 0) {
		p_proc_ready->blocked = 1;
		p_proc_ready->status = WAITING;
		s->p_list[s->tail] = p_proc_ready;
		s->tail = (s->tail + 1) % NR_PROCS;
		schedule();
	}
	enable_int();
}

PUBLIC void v_process(SEMAPHORE *s) {
	disable_int();
	s->value++;
    // value <= 0,说明有进程在等待资源，因此需要唤醒
	if (s->value <= 0) {
		s->p_list[s->head]->blocked = FALSE;
		s->head = (s->head + 1) % NR_PROCS;
	}
	enable_int();
}

PUBLIC int sys_get_ticks() {
	return ticks;
}

PUBLIC void sys_sleep(int milli_sec) {
	int ticks = milli_sec / 1000 * HZ * 10;
	p_proc_ready->sleeping = ticks;
	schedule();
}

PUBLIC void sys_write_str(char *buf, int len) {
	CONSOLE *p_con = console_table;
	for (int i = 0; i < len; i++) {
		out_char(p_con, buf[i]);
	}
}

