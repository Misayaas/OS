
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifdef    GLOBAL_VARIABLES_HERE
#undef    EXTERN
#define    EXTERN
#endif

EXTERN    int ticks;
EXTERN    int disp_pos;
EXTERN    u8 gdt_ptr[6];    // 0~15:Limit  16~47:Base
EXTERN    DESCRIPTOR gdt[GDT_SIZE];
EXTERN    u8 idt_ptr[6];    // 0~15:Limit  16~47:Base
EXTERN    GATE idt[IDT_SIZE];

EXTERN    u32 k_reenter;

EXTERN    TSS tss;
EXTERN    PROCESS * p_proc_ready;

//todo: 定义读者写者数量等信息
EXTERN  int readers;
EXTERN  int writers;
EXTERN  int nr_current_console;

extern PROCESS proc_table[];
extern char task_stack[];
extern TASK task_table[];
extern irq_handler irq_table[];

extern TASK user_proc_table[];
extern TTY tty_table[];
extern CONSOLE console_table[];

//todo: 定义各个信号量
extern SEMAPHORE rw_mutex; // 实现读写互斥
extern SEMAPHORE writer_mutex; // 写进程互斥信号量
extern SEMAPHORE reader_mutex; // 读进程互斥信号量
extern SEMAPHORE S; // 制衡读写者
extern SEMAPHORE reader_count_mutex; // 限制读者人数

