#pragma once

#include <klib/compiler.h>
#include <klib/stdint.h>
#include <stdbool.h>

#define GDT_SIZE 128
#define IDT_SIZE 256
#define LDT_SIZE 2

//! selector
//! ┏━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┓
//! ┃15┃14┃13┃12┃11┃10┃09┃08┃07┃06┃05┃04┃03┃02┃01┃00┃
//! ┣━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━╋━━╋━━┻━━┫
//! ┃ descriptor index                     ┃TI┃ RPL ┃
//! ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┻━━┻━━━━━┛
typedef struct selector {
    uint16_t rpl : 2;
    uint16_t ti : 1;
    uint16_t index : 13;
} PACKED selector_t;

//! descriptor of GDT & SDT
typedef struct descriptor {
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t attr0;
    uint8_t attr1_limit1;
    uint8_t base2;
} PACKED descriptor_t;

//! pointer to the descriptor table
typedef struct descptr_s {
    uint16_t limit;
    uint32_t base;
} PACKED descptr_t;

//! gate descriptor
typedef struct gate_descriptor {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t dcount;

    union {
        uint8_t attr;

        struct {
            uint8_t attr_type : 4;
            uint8_t attr_s : 1;
            uint8_t attr_dpl : 2;
            uint8_t attr_present : 1;
        };
    };

    uint16_t offset_high;
} PACKED gate_descriptor_t;

typedef struct tss {
    uint32_t backlink;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t flags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iobase;
    uint8_t iomap[0];
} PACKED tss_t;

//! 描述符类型值说明
#define DA_32 0x4000       //<! 32 位段
#define DA_LIMIT_4K 0x8000 //<! 段界限粒度为 4K 字节
#define DA_DPL0 0x00       //<! DPL = 0
#define DA_DPL1 0x20       //<! DPL = 1
#define DA_DPL2 0x40       //<! DPL = 2
#define DA_DPL3 0x60       //<! DPL = 3

//! 存储段描述符类型值说明
#define DA_DR 0x90   //<! 存在的只读数据段类型值
#define DA_DRW 0x92  //<! 存在的可读写数据段属性值
#define DA_DRWA 0x93 //<! 存在的已访问可读写数据段类型值
#define DA_C 0x98    //<! 存在的只执行代码段属性值
#define DA_CR 0x9a   //<! 存在的可执行可读代码段属性值
#define DA_CCO 0x9c  //<! 存在的只执行一致代码段属性值
#define DA_CCOR 0x9e //<! 存在的可执行可读一致代码段属性值

//! 系统段描述符类型值说明
#define DA_LDT 0x82      //<! 局部描述符表段类型值
#define DA_TaskGate 0x85 //<! 任务门类型值
#define DA_386TSS 0x89   //<! 可用 386 任务状态段类型值
#define DA_386CGate 0x8c //<! 386 调用门类型值
#define DA_386IGate 0x8e //<! 386 中断门类型值
#define DA_386TGate 0x8f //<! 386 陷阱门类型值

#define SELECTOR_RPL_MASK 0x0003
#define SELECTOR_TI_MASK 0x0004
#define SELECTOR_INDEX_MASK 0xfff8

#define SELECTOR_RPL_SHIFT 0
#define SELECTOR_TI_SHIFT 2
#define SELECTOR_INDEX_SHIFT 3

#define RPL_0 0
#define RPL_1 1
#define RPL_2 2
#define RPL_3 3

#define TI_GDT 0
#define TI_LDT 1

#define SELECTOR_MAKE_RPL(rpl) (((rpl) << SELECTOR_RPL_SHIFT) & SELECTOR_RPL_MASK)
#define SELECTOR_MAKE_TI(ti) (((ti) << SELECTOR_TI_SHIFT) & SELECTOR_TI_MASK)
#define SELECTOR_MAKE_INDEX(index) (((index) << SELECTOR_INDEX_SHIFT) & SELECTOR_INDEX_MASK)

#define SELECTOR_GET_RPL(sel) (((sel) & SELECTOR_RPL_MASK) >> SELECTOR_RPL_SHIFT)
#define SELECTOR_GET_TI(sel) (((sel) & SELECTOR_TI_MASK) >> SELECTOR_TI_SHIFT)
#define SELECTOR_GET_INDEX(sel) (((sel) & SELECTOR_INDEX_MASK) >> SELECTOR_INDEX_SHIFT)

#define SA_RPL0 SELECTOR_MAKE_RPL(RPL_0)
#define SA_RPL1 SELECTOR_MAKE_RPL(RPL_1)
#define SA_RPL2 SELECTOR_MAKE_RPL(RPL_2)
#define SA_RPL3 SELECTOR_MAKE_RPL(RPL_3)

#define SA_TIG SELECTOR_MAKE_TI(TI_GDT)
#define SA_TIL SELECTOR_MAKE_TI(TI_LDT)

#define RPL_KERNEL RPL_0
#define RPL_TASK RPL_1
#define RPL_USER RPL_3

#define __MAKE_SELECTOR(index, ti, rpl) \
    (SELECTOR_MAKE_INDEX(index) | SELECTOR_MAKE_TI(ti) | SELECTOR_MAKE_RPL(rpl))
#define __MAKE_SELECTOR_1(index) __MAKE_SELECTOR(index, TI_GDT, RPL_KERNEL)
#define __MAKE_SELECTOR_2(index, rpl) __MAKE_SELECTOR(index, TI_GDT, rpl)
#define __MAKE_SELECTOR_3(index, ti, rpl) __MAKE_SELECTOR(index, ti, rpl)
#define __MAKE_SELECTOR_IMPL(N, ...) MH_EXPAND(MH_CONCAT(__MAKE_SELECTOR_, N)(__VA_ARGS__))
#define MAKE_SELECTOR(...) MH_EXPAND(__MAKE_SELECTOR_IMPL(MH_NARG(__VA_ARGS__), __VA_ARGS__))

#define INDEX_DUMMY 0
#define INDEX_FLAT_C 1
#define INDEX_FLAT_RW 2
#define INDEX_VIDEO 3
#define INDEX_TSS 4
#define INDEX_LDT_FIRST 5

#define INDEX_LDT_C 0
#define INDEX_LDT_RW 1

#define SELECTOR_FLAT_C MAKE_SELECTOR(INDEX_FLAT_C)
#define SELECTOR_FLAT_RW MAKE_SELECTOR(INDEX_FLAT_RW)
#define SELECTOR_VIDEO MAKE_SELECTOR(INDEX_VIDEO, TI_GDT, RPL_USER)
#define SELECTOR_TSS MAKE_SELECTOR(INDEX_TSS)
#define SELECTOR_LDT_FIRST MAKE_SELECTOR(INDEX_LDT_FIRST)

#define SELECTOR_KERNEL_CS SELECTOR_FLAT_C
#define SELECTOR_KERNEL_DS SELECTOR_FLAT_RW
#define SELECTOR_KERNEL_GS SELECTOR_VIDEO

#define SELECTOR_DEFAULT_CS(rpl) MAKE_SELECTOR(INDEX_LDT_C, TI_LDT, rpl)
#define SELECTOR_DEFAULT_DS(rpl) MAKE_SELECTOR(INDEX_LDT_RW, TI_LDT, rpl)
#define SELECTOR_DEFAULT_ES(rpl) MAKE_SELECTOR(INDEX_LDT_RW, TI_LDT, rpl)
#define SELECTOR_DEFAULT_FS(rpl) MAKE_SELECTOR(INDEX_LDT_RW, TI_LDT, rpl)
#define SELECTOR_DEFAULT_SS(rpl) MAKE_SELECTOR(INDEX_LDT_RW, TI_LDT, rpl)
#define SELECTOR_DEFAULT_GS(rpl) MAKE_SELECTOR(INDEX_VIDEO, TI_GDT, rpl)

#define vir2phys(seg_base, vir) (u32)(((u32)seg_base) + (u32)(vir))

#define DESC_GET_BASE(desc) \
    ((((desc).base2 & 0xff) << 24) | (((desc).base1 & 0xff) << 16) | ((desc).base0 & 0xffff))
#define DESC_GET_LIMIT(desc) ((((desc).attr1_limit1 & 0xf) << 16) | ((desc).limit0 & 0xffff))
#define DESC_GET_ATTR(desc) ((((desc).attr1_limit1 & 0xf0) << 8) | ((desc).attr0 & 0xff))

extern tss_t tss;
extern descptr_t gdt_ptr;
extern descriptor_t gdt[GDT_SIZE];
extern descptr_t idt_ptr;
extern gate_descriptor_t idt[IDT_SIZE];

void init_descriptor(descriptor_t* desc, uint32_t base, uint32_t limit, uint16_t attr);

uint32_t seg2phys(uint16_t seg);

#define is_rpl_supported(rpl)    \
    ({                           \
        ((bool[]){               \
            [RPL_KERNEL] = true, \
            [RPL_TASK] = true,   \
            [RPL_USER] = true,   \
        })[(rpl) & 0x3];         \
    })
