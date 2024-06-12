
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */
#define TAB_WIDTH	4

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80

#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */


/*记录光标所在位置*/
typedef struct cursor_pos_stack{
    int ptr; //当前光标的索引
    int pos[SCREEN_SIZE]; //存储所有的光标位置
}POSSTACK;

/* CONSOLE */
typedef struct s_console
{
    unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
    unsigned int	original_addr;		/* 当前控制台对应显存位置 */
    unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
    unsigned int	cursor;			/* 当前光标位置 */
    POSSTACK pos_stack;
}CONSOLE;

PRIVATE void push_pos(CONSOLE* p_con, int pos);

PRIVATE int pop_pos(CONSOLE* p_con);

PUBLIC void EXITEsc(CONSOLE* p_con);

PUBLIC void search(CONSOLE* p_con);

#endif /* _ORANGES_CONSOLE_H_ */


