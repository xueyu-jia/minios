#include <kernel/buddy.h>
#include <kernel/const.h>
#include <kernel/pagetable.h>
#include <kernel/proc.h>
#include <kernel/protect.h>
#include <kernel/proto.h>
#include <kernel/type.h>
#include <kernel/uart.h>
#include <kernel/vga.h>
#include <klib/string.h>

// added by mingxuan 2021-8-29
PUBLIC void init_gdt() {
    // init_gdt 只有3个段
    init_descriptor(&gdt[INDEX_DUMMY], 0, 0, 0);
    init_descriptor(&gdt[INDEX_FLAT_C], 0, 0x0fffff,
                    DA_CR | DA_32 | DA_LIMIT_4K);
    init_descriptor(&gdt[INDEX_FLAT_RW], 0, 0x0fffff,
                    DA_DRWA | DA_32 | DA_LIMIT_4K);
    // init_descriptor(&gdt[INDEX_VIDEO], 0x0B8000, 0x0ffff, DA_DRWA | DA_DPL3);

    // 显存描述符 //add by visual 2016.5.12
    init_descriptor(
        &gdt[INDEX_VIDEO],
        K_PHY2LIN(
            V_MEM_BASE), // fix:
                         // 硬编码修改为线性地址而没有对应更改console相应访问地址参数
        0x0ffff, DA_DRW | DA_DPL3);

    // 填充 GDT 中 TSS 这个描述符
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = SELECTOR_KERNEL_DS;
    init_descriptor(&gdt[INDEX_TSS],
                    vir2phys(seg2phys(SELECTOR_KERNEL_DS), &tss),
                    sizeof(tss) - 1, DA_386TSS);
    tss.iobase = sizeof(tss); // 没有I/O许可位图

    // 填充 GDT 中进程的 LDT 的描述符
    int i;
    PROCESS *p_proc = proc_table;
    u16 selector_ldt = INDEX_LDT_FIRST << 3;
    // for(i=0;i<NR_TASKS;i++){
    for (i = 0; i < NR_PCBS; i++) { // edit by visual 2016.4.5
        init_descriptor(&gdt[selector_ldt >> 3],
                        vir2phys(seg2phys(SELECTOR_KERNEL_DS),
                                 proc_table[i].task.context.ldts),
                        LDT_SIZE * sizeof(DESCRIPTOR) - 1, DA_LDT);
        p_proc++;
        selector_ldt += 1 << 3;
    }

    u16 *p_gdt_limit = (u16 *)(&gdt_ptr[0]);
    u32 *p_gdt_base = (u32 *)(&gdt_ptr[2]);
    *p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
    *p_gdt_base = (u32)&gdt;
}

/*======================================================================*
                            cstart
 *======================================================================*/
PUBLIC void cstart() {
    kernel_initial = 1;

#ifdef OPT_DISP_SERIAL
    init_simple_serial();
#endif
    disp_str("\n\n\n\n\n\n-----\"cstart\" begins-----\n");

    memory_init(); // moved from kernel_main, mingxuan 2021-8-25

    if (-1 == init_kernel_page()) {
        disp_color_str("reinit_kernel_page error!\n", 0x74);
    };
    init_gdt();
    refresh_gdt(); // added by sundong 2023.3.8
                   // 更新gdtr的值，并且刷新gs、es、ds、cs的隐藏部分。
    init_interrupt_controller();
    disp_str("-----\"cstart\" finished-----\n");
}
