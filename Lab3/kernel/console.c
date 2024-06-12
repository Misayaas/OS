
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);

PRIVATE void set_video_start_addr(u32 addr);

PRIVATE void flush(CONSOLE *p_con);

//PRIVATE TTY *search_tty;

//PRIVATE u8 *search_vmem = 0;

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty) {
    int nr_tty = p_tty - tty_table;
    p_tty->p_console = console_table + nr_tty;


    int v_mem_size = V_MEM_SIZE >> 1;    /* 显存总大小 (in WORD) */

    int con_v_mem_size = v_mem_size / NR_CONSOLES;
    p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
    p_tty->p_console->v_mem_limit = con_v_mem_size;
    p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;
    //初始化pos_stack的ptr指针
    p_tty->p_console->pos_stack.ptr = 0;
    /* 默认光标位置在最开始处 */
    p_tty->p_console->cursor = p_tty->p_console->original_addr;

    if (nr_tty == 0) {
        /* 第一个控制台沿用原来的光标位置 */
        p_tty->p_console->cursor = disp_pos / 2;
        disp_pos = 0;
    } else {
        out_char(p_tty->p_console, nr_tty + '0');
        out_char(p_tty->p_console, '#');
    }

    set_cursor(p_tty->p_console->cursor);
    //search_tty = p_tty;
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con) {
    return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE *p_con, char ch) {
    u8 *p_vmem = (u8 *) (V_MEM_BASE + p_con->cursor * 2);

    /*
    目前未解决问题：TAB在第一个无法直接删除；换行后的字符会一次性被删除(已解决)
    */
    switch (ch) {
        case '\n':
            //如果光标不在最后一行
            if (p_con->cursor < p_con->original_addr +
                                p_con->v_mem_limit - SCREEN_WIDTH) {
                push_pos(p_con, p_con->cursor);
                //通过行数来计算光标位置
                p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
                                                       ((p_con->cursor - p_con->original_addr) /
                                                        SCREEN_WIDTH + 1);
            }
            break;
        case '\b':
            //保证光标移动的合法
            if (p_con->cursor > p_con->original_addr) {
                if(mode == INSERTMODE){
                    int prePos = pop_pos(p_con);

                    for (int i = prePos; i < p_con->cursor; ++i) {
                        *(p_vmem - 2) = ' ';
                        *(p_vmem - 1) = DEFAULT_CHAR_COLOR;
                        p_vmem -= 2;
                    }
                    p_con->cursor = prePos;
                }
                else{
                    if(ESCInputLength != 0){
                        if(*(p_vmem - 1) == BLUE){
                            ESCInputLength -= TAB_WIDTH;
                            p_con->cursor -= TAB_WIDTH;
                            for(int i = 0; i < TAB_WIDTH;i++){
                                *(p_vmem - 2) = ' ';
                                *(p_vmem - 1) = DEFAULT_CHAR_COLOR;
                                p_vmem -= 2;
                            }
                        }
                        else{
                            ESCInputLength--;
                            *(p_vmem - 2) = ' ';
                            *(p_vmem - 1) = DEFAULT_CHAR_COLOR;
                            p_vmem -= 2;
                            p_con->cursor--;
                        }
                        /**(search_vmem - 2) = ' ';
                        *(search_vmem - 1) = DEFAULT_CHAR_COLOR;
                        search_vmem -= 2;*/
                    }

                }
            }
            break;
        case '\t':
            //保证可以TAB的距离合法
            if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - TAB_WIDTH) {
                if(mode == INSERTMODE){
                    for (int i = 0; i < TAB_WIDTH; i++) {
                        *p_vmem++ = ' ';
                        *p_vmem++ = BLUE;
                    }
                    push_pos(p_con, p_con->cursor);
                    p_con->cursor += TAB_WIDTH;
                }
                else{
                    for (int i = 0; i < TAB_WIDTH; i++) {
                        *p_vmem++ = ' ';
                        *p_vmem++ = BLUE;

                        /**search_vmem++ = ' ';
                        *search_vmem++ = GREEN;*/
                    }
                    ESCInputLength += TAB_WIDTH;
                    p_con->cursor += TAB_WIDTH;
                }
            }
            break;
        default:
            if (p_con->cursor <
                p_con->original_addr + p_con->v_mem_limit - 1) {
                *p_vmem++ = ch;
                if (mode == INSERTMODE) {
                    *p_vmem++ = DEFAULT_CHAR_COLOR;
                    push_pos(p_con, p_con->cursor);
                } else {
                    *p_vmem++ = RED;
                    ESCInputLength++;

                    /**search_vmem++ = ch;
                    *search_vmem++ = RED;*/
                }
                p_con->cursor++;
            }
            break;
    }

    while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
        scroll_screen(p_con, SCR_DN);
    }

    flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con) {
    set_cursor(p_con->cursor);
    set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position) {
    disable_int();
    out_byte(CRTC_ADDR_REG, CURSOR_H);
    out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, CURSOR_L);
    out_byte(CRTC_DATA_REG, position & 0xFF);
    enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr) {
    disable_int();
    out_byte(CRTC_ADDR_REG, START_ADDR_H);
    out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, START_ADDR_L);
    out_byte(CRTC_DATA_REG, addr & 0xFF);
    enable_int();
}

/*======================================================================*
			   退出ESC模式
 *======================================================================*/
PUBLIC void EXITEsc(CONSOLE *p_con) {
    //获取光标在显存中的位置
    u8 *p_vmem = (u8 *) (V_MEM_BASE + p_con->cursor * 2);
    for (int i = 0; i < ESCInputLength; ++i) {
        *(p_vmem - 1) = DEFAULT_CHAR_COLOR;
        *(p_vmem - 2) = ' ';
        p_vmem -= 2;
    }
    p_con->cursor = ESCBeginPos;
    p_vmem = (u8 *) (V_MEM_BASE);
    //将光标之前的字符全部恢复为默认颜色
    for (int i = 0; i < p_con->cursor; i++) {
        if(*(p_vmem + 1) == DEFAULT_CHAR_COLOR || *(p_vmem + 1) == RED){
            *(p_vmem + 1) = DEFAULT_CHAR_COLOR;
        }
        else if(*(p_vmem + 1) == BLUE){
            *(p_vmem + 1) = BLUE;
        }
        p_vmem += 2;
    }
    ESCInputLength = 0;
    flush(p_con);
}

/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)    /* 0 ~ (NR_CONSOLES - 1) */
{
    if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
        return;
    }

    nr_current_console = nr_console;

    set_cursor(console_table[nr_console].cursor);
    set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction) {
    if (direction == SCR_UP) {
        if (p_con->current_start_addr > p_con->original_addr) {
            p_con->current_start_addr -= SCREEN_WIDTH;
        }
    } else if (direction == SCR_DN) {
        if (p_con->current_start_addr + SCREEN_SIZE <
            p_con->original_addr + p_con->v_mem_limit) {
            p_con->current_start_addr += SCREEN_WIDTH;
        }
    } else {
    }

    set_video_start_addr(p_con->current_start_addr);
    set_cursor(p_con->cursor);
}

PRIVATE void push_pos(CONSOLE *p_con, int pos) {
    p_con->pos_stack.pos[p_con->pos_stack.ptr++] = pos;
}

PRIVATE int pop_pos(CONSOLE *p_con) {
    if (p_con->pos_stack.ptr == 0) {
        return 0;
    } else {
        --p_con->pos_stack.ptr;
        return p_con->pos_stack.pos[p_con->pos_stack.ptr];
    }
}

PUBLIC void search(CONSOLE *p_con) {
    //将前面的颜色变为默认颜色，以便二次查找
    u8 *temp_p_vmem = (u8 *)(V_MEM_BASE);
    for (int i = 0; i < ESCBeginPos; i++) {
        if(*(temp_p_vmem + 1) == DEFAULT_CHAR_COLOR || *(temp_p_vmem + 1) == RED){
            *(temp_p_vmem + 1) = DEFAULT_CHAR_COLOR;
        }
        else if(*(temp_p_vmem + 1) == BLUE){
            *(temp_p_vmem + 1) = BLUE;
        }
        temp_p_vmem += 2;
    }

/*    TTY *temp = search_tty;
    int begin = 0, end = 0;
    for(int i = 0; i < ESCInputLength;){
        if(*(search_tty->p_inbuf_head - 1) == '\t'){
            i += 4;
            search_tty->p_inbuf_head--;
        }
        else if(*(search_tty->p_inbuf_head - 1) == '\b'){
            search_tty->p_inbuf_head -= 2;
        }
        else{
            i++;
            search_tty->p_inbuf_head--;
        }
    }
    for(int i = 0; i < ESCBeginPos; i++){
        begin = end = i;

        int isFind = 1;
        for(int k = 0, j = begin; k < temp->p_inbuf_head - search_tty->p_inbuf_head; k++, j++){
            if(*(search_tty->p_inbuf_head + k) == *(temp->p_inbuf_head - temp->inbuf_count + j)){
                end++;
            }
            else{
                isFind = 0;
                break;
            }

        }
        if(isFind == 1){
            for(int j = begin; j < end; j++){
                *((u8 *)(V_MEM_BASE + j * 2 + 1)) = RED;
            }
        }
    }*/

    //滑动窗口
    int begin = 0, end = 0;
    //遍历所有的字符
    for (int i = 0; i < ESCBeginPos * 2; i += 2) {
        begin = end = i;

        int isFind = 1;
        //判断相同
        for (int j = begin, k = 0; k < ESCInputLength * 2; j += 2, k += 2) {
            if (*((u8 *) (V_MEM_BASE + j)) == *((u8 *) (V_MEM_BASE + ESCBeginPos * 2 + k))) {
                //判断空格和TAB
                if(((*((u8 *) (V_MEM_BASE + j + 1)) == BLUE) && (*((u8 *) (V_MEM_BASE + ESCBeginPos * 2 + k + 1)) != BLUE)) || ((*((u8 *) (V_MEM_BASE + j + 1)) != BLUE) && (*((u8 *) (V_MEM_BASE + ESCBeginPos * 2 + k + 1)) == BLUE))){
                    isFind = 0;
                    break;
                }
                end += 2;
            } else {
                isFind = 0;
                break;
            }
        }
        //显示为红色
        if (isFind == 1) {
            for (int j = begin; j < end; j += 2) {
                //保持tab的蓝色不变用于多次匹配
                if(*((u8 *) (V_MEM_BASE + j + 1)) != BLUE){
                    *((u8 *) (V_MEM_BASE + j + 1)) = RED;
                }
            }
        }
    }


    flush(p_con);
}