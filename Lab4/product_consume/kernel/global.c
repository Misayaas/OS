
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "proc.h"
#include "global.h"
#include "proto.h"


PUBLIC	PROCESS	proc_table[NR_TASKS + NR_PROCS];

PUBLIC	TASK	task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "tty"}};

PUBLIC  TASK    user_proc_table[NR_PROCS] = {
	{reporter_A, STACK_SIZE_TESTA, "reporter_A"},
	{product_p1, STACK_SIZE_TESTB, "product_p1"},
	{product_p2, STACK_SIZE_TESTC, "product_p2"},
	{consume_p1, STACK_SIZE_TESTD, "consume_p1"},
	{consume_p2_1, STACK_SIZE_TESTE, "consume_p2_1"},
	{consume_p2_2, STACK_SIZE_TESTF, "consume_p2_2"}};

PUBLIC	char		task_stack[STACK_SIZE_TOTAL];

PUBLIC	TTY		tty_table[NR_CONSOLES];
PUBLIC	CONSOLE		console_table[NR_CONSOLES];

PUBLIC	irq_handler	irq_table[NR_IRQ];

PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {sys_get_ticks, 
                                                      sys_print, sys_sleep, sys_p, sys_v};

PUBLIC int mode;

PUBLIC Semaphore empty_mutex;
PUBLIC Semaphore full_mutex;
PUBLIC Semaphore p1_mutex;
PUBLIC Semaphore p2_mutex;
PUBLIC Semaphore consume_p2_mutex;
