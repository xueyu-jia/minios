
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            protect.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_PROTECT_H_
#define _ORANGES_PROTECT_H_
#include <kernel/interrupt_x86.h>
#include <kernel/type.h>

/* 存储段描述符/系统段描述符 */
typedef struct s_descriptor /* 共 8 个字节 */
{
  u16 limit_low;       /* Limit */
  u16 base_low;        /* Base */
  u8 base_mid;         /* Base */
  u8 attr1;            /* P(1) DPL(2) DT(1) TYPE(4) */
  u8 limit_high_attr2; /* G(1) D(1) 0(1) AVL(1) LimitHigh(4) */
  u8 base_high;        /* Base */
} DESCRIPTOR;

/* 门描述符 */
// typedef struct s_gate
// {
// 	u16	offset_low;	/* Offset Low */
// 	u16	selector;	/* Selector */
// 	u8	dcount;		/* 该字段只在调用门描述符中有效。
// 				如果在利用调用门调用子程序时引起特权级的转换和堆栈的改变，需要将外层堆栈中的参数复制到内层堆栈。
// 				该双字计数字段就是用于说明这种情况发生时，要复制的双字参数的数量。
// */ 	u8	attr;		/* P(1) DPL(2) DT(1) TYPE(4) */ 	u16
// offset_high;	/* Offset High */ }GATE;

typedef struct s_tss {
  u32 backlink;
  u32 esp0; /* stack pointer to use during interrupt */
  u32 ss0;  /*   "   segment  "  "    "        "     */
  u32 esp1;
  u32 ss1;
  u32 esp2;
  u32 ss2;
  u32 cr3;
  u32 eip;
  u32 flags;
  u32 eax;
  u32 ecx;
  u32 edx;
  u32 ebx;
  u32 esp;
  u32 ebp;
  u32 esi;
  u32 edi;
  u32 es;
  u32 cs;
  u32 ss;
  u32 ds;
  u32 fs;
  u32 gs;
  u32 ldt;
  u16 trap;
  u16 iobase; /* I/O位图基址大于或等于TSS段界限，就表示没有I/O许可位图 */
              /*u8	iomap[2];*/
} TSS;

/* GDT */
/* 描述符索引 */
#define INDEX_DUMMY 0    // ┓
#define INDEX_FLAT_C 1   // ┣ LOADER 里面已经确定了的.
#define INDEX_FLAT_RW 2  // ┃
#define INDEX_VIDEO 3    // ┛
#define INDEX_TSS 4
#define INDEX_LDT_FIRST 5
/* 选择子 */
#define SELECTOR_DUMMY 0           // ┓
#define SELECTOR_FLAT_C 0x08       // ┣ LOADER 里面已经确定了的.
#define SELECTOR_FLAT_RW 0x10      // ┃
#define SELECTOR_VIDEO (0x18 + 3)  // ┛<-- RPL=3
#define SELECTOR_TSS 0x20  // TSS. 从外层跳到内存时 SS 和 ESP 的值从里面获得.
#define SELECTOR_LDT_FIRST 0x28

#define SELECTOR_KERNEL_CS SELECTOR_FLAT_C
#define SELECTOR_KERNEL_DS SELECTOR_FLAT_RW
#define SELECTOR_KERNEL_GS SELECTOR_VIDEO

/* 每个任务有一个单独的 LDT, 每个 LDT 中的描述符个数: */
#define LDT_SIZE 2
// added by zcr
/* descriptor indices in LDT */
#define INDEX_LDT_C 0
#define INDEX_LDT_RW 1
//~zcr

/* 描述符类型值说明 */
#define DA_32 0x4000       /* 32 位段				*/
#define DA_LIMIT_4K 0x8000 /* 段界限粒度为 4K 字节			*/
#define DA_DPL0 0x00       /* DPL = 0				*/
#define DA_DPL1 0x20       /* DPL = 1				*/
#define DA_DPL2 0x40       /* DPL = 2				*/
#define DA_DPL3 0x60       /* DPL = 3				*/
/* 存储段描述符类型值说明 */
#define DA_DR 0x90   /* 存在的只读数据段类型值		*/
#define DA_DRW 0x92  /* 存在的可读写数据段属性值		*/
#define DA_DRWA 0x93 /* 存在的已访问可读写数据段类型值	*/
#define DA_C 0x98    /* 存在的只执行代码段属性值		*/
#define DA_CR 0x9A   /* 存在的可执行可读代码段属性值		*/
#define DA_CCO 0x9C  /* 存在的只执行一致代码段属性值		*/
#define DA_CCOR 0x9E /* 存在的可执行可读一致代码段属性值	*/
/* 系统段描述符类型值说明 */
#define DA_LDT 0x82      /* 局部描述符表段类型值			*/
#define DA_TaskGate 0x85 /* 任务门类型值				*/
#define DA_386TSS 0x89   /* 可用 386 任务状态段类型值		*/
#define DA_386CGate 0x8C /* 386 调用门类型值			*/
#define DA_386IGate 0x8E /* 386 中断门类型值			*/
#define DA_386TGate 0x8F /* 386 陷阱门类型值			*/

/* 选择子类型值说明 */
/* 其中, SA_ : Selector Attribute */
#define SA_RPL_MASK 0xFFFC
#define SA_RPL0 0
#define SA_RPL1 1
#define SA_RPL2 2
#define SA_RPL3 3

#define SA_TI_MASK 0xFFFB
#define SA_TIG 0
#define SA_TIL 4

/* 权限 */
#define PRIVILEGE_KRNL 0
#define PRIVILEGE_TASK 1
#define PRIVILEGE_USER 3
/* RPL */
#define RPL_KRNL SA_RPL0
#define RPL_TASK SA_RPL1
#define RPL_USER SA_RPL3

#define common_cs (((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL)
#define common_ds (((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL)
#define common_es (((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL)
#define common_fs (((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL)
#define common_ss (((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL)
#define common_gs (SELECTOR_KERNEL_GS & SA_RPL_MASK)

/* 8253/8254 PIT (Programmable Interval Timer) */
#define TIMER0 0x40     /* I/O port for timer channel 0 */
#define TIMER_MODE 0x43 /* I/O port for timer mode control */
#define RATE_GENERATOR                                                           \
  0x34                      /* 00-11-010-0 :                                     \
                             * Counter0 - LSB then MSB - rate generator - binary \
                             */
#define TIMER_FREQ 1193182L /* clock frequency for timer in PC and AT */
#define HZ 100              /* clock freq (software settable on IBM-PC) */

/* 宏 */
/* 线性地址 → 物理地址 */
#define vir2phys(seg_base, vir) (u32)(((u32)seg_base) + (u32)(vir))

extern u32 k_reenter;
extern TSS tss;

extern u8 gdt_ptr[6];  // 0~15:Limit  16~47:Base
extern DESCRIPTOR gdt[GDT_SIZE];

PUBLIC u32 seg2phys(u16 seg);
PUBLIC void init_descriptor(DESCRIPTOR* p_desc, u32 base, u32 limit,
                            u16 attribute);

#endif /* _ORANGES_PROTECT_H_ */
