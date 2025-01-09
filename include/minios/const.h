#pragma once

#include <minios/type.h>
#include <minios/layout.h>
#include <klib/compiler.h>
#include <klib/stddef.h>
#include <limits.h> // IYWU pragma: export

/* Color */
/*
 * e.g. MAKE_COLOR(BLUE, RED)
 *      MAKE_COLOR(BLACK, RED) | BRIGHT
 *      MAKE_COLOR(BLACK, RED) | BRIGHT | FLASH
 */
// added by mingxuan 2019-5-19
#define BLACK 0x0   /* 0000 */
#define WHITE 0x7   /* 0111 */
#define RED 0x4     /* 0100 */
#define GREEN 0x2   /* 0010 */
#define BLUE 0x1    /* 0001 */
#define FLASH 0x80  /* 1000 0000 */
#define BRIGHT 0x08 /* 0000 1000 */
#define MAKE_COLOR(x, y)                                \
    ((x << 4) | y) /* MAKE_COLOR(Background,Foreground) \
                    */

/* EXTERN */
// #define	EXTERN	extern	/* EXTERN is defined as extern except in
// global.c */

/* 函数类型 */
#define PUBLIC         /* PUBLIC is the opposite of PRIVATE */
#define PRIVATE static /* PRIVATE x limits the scope of x */

// devfs支持以后，移除初始化设备的系统调用
/* TTY */
// added by mingxuan 2019-5-19
#define NR_CONSOLES 3 /* consoles */

/*页表相关*/
//! FIXME: 这个不再有效
#define PageTblNumAddr 0x500

// 物理内存长度
//! 存在bug，当PHY_MEM_SIZE超过2G时，无论物理内存为多少，都会会闪退，推测是数据结构mem_map[ALL_PAGES]太大了，内核空间不够
//! 当PHY_MEM_SIZE超过1G时，KernelLinLimitMAX和KernelLinMapBase会限制

//! ATTENTION: qemu 改内存的时候这里一定要改！！！！
//! NOTE: 当然，你改了也没用，大于 64M 照爆不误
#define PHY_MEM_SIZE 0x04000000 // 64M

#define UPPER_BOUND(x, size) ((((u32)(x)) + ((size) - 1)) & (~((size) - 1)))
#define UPPER_BOUND_4K(x) UPPER_BOUND(x, num_4K)
#define LOWER_BOUND(x, size) ((((u32)(x))) & (~((size) - 1)))
#define LOWER_BOUND_4K(x) LOWER_BOUND(x, num_4K)
#define num_4B 0x4      // 4B大小
#define num_1K 0x400    // 1k大小
#define num_4K 0x1000   // 4k大小
#define num_4M 0x400000 // 4M大小
#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define TextLinBase ((u32)0x0) // 进程代码的起始地址，这是参考值，具体以elf描述为准
#define TextLinLimitMAX (TextLinBase + 0x20000000) // 大小：512M，这是参考值，具体以elf描述为准，
#define DataLinBase TextLinLimitMAX // 进程数据的起始地址，这是参考值，具体以elf描述为准
#define DataLinLimitMAX \
    (DataLinBase +      \
     0x20000000) // 大小：512M，这是参考值，具体以elf描述为准，但是代码和数据长度总和不能超过这个值
#define VpageLinBase DataLinLimitMAX                         // 保留内存起始地址
#define VpageLinLimitMAX (VpageLinBase + 0x8000000 - num_4K) // 大小：128M-4k
#define SharePageBase VpageLinLimitMAX // 共享页线性地址，执行fork\pthread的时候用,共享页必须4K对齐
#define SharePageLimit (SharePageBase + num_4K)    // 大小：4k
#define HeapLinBase SharePageLimit                 // 堆的起始地址
#define HeapLinLimitMAX (HeapLinBase + 0x3FD00000) // 大小：1G-3M
#define ShareLinBase \
    (HeapLinLimitMAX + 0x100000)                   // 共享内存的起始地址
                                                   // 共享内存与堆顶有1M空闲
#define ShareLinLimitMAX (ShareLinBase + 0x100000) // 大小：1M
#define StackLinLimitMAX \
    (ShareLinLimitMAX + 0x100000) // 栈的大小： 1G-128M-4K（注意栈的基址和界限方向）
#define StackLinBase \
    (ArgLinBase) //=(StackLinLimitMAX+1G-128M-4K)栈的起始地址,放在参数位置之前（注意堆栈的增长方向）
#define ArgLinBase (KernelLinBase - 0x1000) // 参数存放位置起始地址，放在3G前，暂时还没没用到
#define ArgLinLimitMAX KernelLinBase //=(ArgLinBase+0x1000)大小：4K。

#define KernelLinMapBase (KernelLinBase + PHY_MEM_SIZE)
#define KernelLinMapMaxPage 1024
#define KernelLinMapLimit (KernelLinMapBase + (KernelLinMapMaxPage * PAGE_SIZE))
#define KernelLinLimitMAX (KernelLinBase + 0x40000000) // 大小：1G
// #define Kernel_space_max		(0x100000000-1)
// // 4GB-1 jiangfeng 2024.05 线性地址KernelLinBase---KernelLinLimitMAX
// 3G---3G+900M 分配所有页目录 KernelLinBase--KernelLinBase+kernel_size
// 3G--3G+8M(或3G+32M,由物理内存大小决定) 内核初始化页表
// KernelLinBase---KernLinMapBase
// 3G---(3G+64M)默认不分配页表，可通过kmapping_phy建立临时页表映射访问如用户物理页等
// 上述两种内核映射都是固定为虚拟地址=3G+物理地址
// KernLinMapBase---KernelLinMapLimit (3G+64M)---(3G+68M) 默认不分配页表，
// 也通过kmapping_phy建立映射并返回具体的映射线性地址,
// 但是可以映射任意高物理地址

// added by mingxuan 2021-1-7
#define MEMMAP_TEXT 0x0
#define MEMMAP_DATA 0x1
#define MEMMAP_VPAGE 0x2
#define MEMMAP_SHARE 0x3
#define MEMMAP_HEAP 0x4
#define MEMMAP_STACK 0x5
#define MEMMAP_ARG 0x6
#define MEMMAP_KERNEL 0x7

/***************目前线性地址布局*****************************		edit by
 *visual 2016.5.25 进程代码		0 ~ 512M ,限制大小为512M 进程数据
 *512M ~ 1G，限制大小为512M 进程保留内存（以后可能存放虚页表和其他一些信息） 1G
 *~ 1G+128M，限制大小为128M,共享页放在这个位置 进程堆			1G+128M
 *~
 *2G+128M，限制大小为1G 进程栈			2G+128M ~ 3G-4K,限制大小为
 *1G-128M-4K 进程参数		3G-4K~3G，限制大小为4K 内核
 *3G~4G，限制大小为1G
 ***********************************************************/

// #define ShareTblLinAddr			(KernelLinLimitMAX-0x1000)
// //公共临时共享页，放在内核最后一个页表的最后一项上
/*分页机制常量的定义,必须与load.inc中一致*/ // add by visual 2016.4.5
#define PG_P 1                              // 页存在属性位
#define PG_RWR 0                            // R/W 属性位值, 读/执行
#define PG_RWW 2                            // R/W 属性位值, 读/写/执行
#define PG_USS 0                            // U/S 属性位值, 系统级
#define PG_USU 4                            // U/S 属性位值, 用户级
#define PG_PS 64                            // PS属性位值，4K页

/* AT keyboard */
/* 8042 ports */
// added by mingxuan 2019-5-19
#define KB_DATA                                    \
    0x60 /* I/O port for keyboard data             \
          *	Read : Read Output Buffer              \
          *	Write: Write Input Buffer              \
          *             (8042 Data & 8048 Command) \
          */
#define KB_CMD                            \
    0x64 /* I/O port for keyboard command \
          *	Read : Read Status Register   \
          *	Write: Write Input Buffer     \
          *	       (8042 Command)         \
          */
#define KB_STA 0x64
#define KEYSTA_SEND_NOTREADY 0x02
#define KBSTATUS_IBF 0x02
#define KBSTATUS_OBF 0x01

#define wait_KB_write() while (inb(KB_STA) & KBSTATUS_IBF)
#define wait_KB_read() while (inb(KB_STA) & KBSTATUS_OBF)

#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47

#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4
#define KBCMD_EN_MOUSE_INTFACE 0xa8

#define LED_CODE 0xED
#define KB_ACK 0xFA

// /* VGA */
// // added by mingxuan 2019-5-19
// #define CRTC_ADDR_REG 0x3D4   /* CRT Controller Registers - Addr Register */
// #define CRTC_DATA_REG 0x3D5   /* CRT Controller Registers - Data Register */
// #define START_ADDR_H  0xC     /* reg index of video mem start addr (MSB) */
// #define START_ADDR_L  0xD     /* reg index of video mem start addr (LSB) */
// #define CURSOR_H      0xE     /* reg index of cursor position (MSB) */
// #define CURSOR_L      0xF     /* reg index of cursor position (LSB) */
// #define V_MEM_BASE    0xB8000 /* base of color video memory */
// #define V_MEM_SIZE    0x8000  /* 32K: B8000H -> BFFFFH */

#define STD_IN 0
#define STD_OUT 1
#define STD_ERR 2
#define MAXARG \
    ((ArgLinLimitMAX - ArgLinBase) / 4 - 2) // 减2是因为Arg开头用来保存argv,末尾需要保存一个NULL

// Options
#define OPT_DISP_SERIAL // 是否将disp_xx的输出打印到串口
// #define OPT_MMU_COW // 是否启用page cache 的写时复制 (Copy-On-Write)
#define OPT_PAGE_CACHE // 是否启用一般read/write的Page Cache
