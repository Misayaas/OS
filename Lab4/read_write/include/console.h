
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_


/* CONSOLE */
typedef struct s_console {
	unsigned int current_start_addr;    /* 当前显示到了什么位置 */
	unsigned int original_addr;        /* 当前控制台对应显存位置 */
	unsigned int v_mem_limit;        /* 当前控制台占的显存大小 */
	unsigned int cursor;            /* 当前光标位置 */
	u8 color;   /* 指示lab3的颜色，不过仍然用于lab4中不同状态颜色的使用  */
} CONSOLE;

#define SCR_UP    1    /* scroll forward */
#define SCR_DN    -1    /* scroll backward */

#define SCREEN_SIZE        (80 * 25)
#define SCREEN_WIDTH        80

#define DEFAULT_CHAR_COLOR    0x07    /* 0000 0111 黑底白字 */


#endif /* _ORANGES_CONSOLE_H_ */

PUBLIC void clean(CONSOLE *p_con);