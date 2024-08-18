
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            start.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <kernel/type.h>
#include <kernel/const.h>
#include <kernel/proto.h>
#include <kernel/protect.h>
#include <kernel/pagetable.h>
#include <kernel/proc.h>
#include <kernel/buddy.h>
#include <kernel/uart.h>
#include <kernel/vga.h>

PUBLIC void init_descriptor(DESCRIPTOR *p_desc, u32 base, u32 limit, u16 attribute); //added by mingxuan 2021-8-25


//added by mingxuan 2021-8-29
PUBLIC void init_gdt()
{
	//modified by mingxuan 2021-8-25
	//init_gdt 只有3个段
	init_descriptor(&gdt[INDEX_DUMMY], 0, 0, 0);
	init_descriptor(&gdt[INDEX_FLAT_C], 0, 0x0fffff, DA_CR | DA_32 | DA_LIMIT_4K);
	init_descriptor(&gdt[INDEX_FLAT_RW], 0, 0x0fffff, DA_DRWA | DA_32 | DA_LIMIT_4K);
	//init_descriptor(&gdt[INDEX_VIDEO], 0x0B8000, 0x0ffff, DA_DRWA | DA_DPL3);

	//显存描述符 //add by visual 2016.5.12
	init_descriptor(&gdt[INDEX_VIDEO],
					K_PHY2LIN(V_MEM_BASE), // fix: 硬编码修改为线性地址而没有对应更改console相应访问地址参数
					0x0ffff,
					DA_DRW | DA_DPL3);

	// 填充 GDT 中 TSS 这个描述符
	memset(&tss, 0, sizeof(tss));
	tss.ss0		= SELECTOR_KERNEL_DS;
	init_descriptor(&gdt[INDEX_TSS],
			vir2phys(seg2phys(SELECTOR_KERNEL_DS), &tss),
			sizeof(tss) - 1,
			DA_386TSS);
	tss.iobase	= sizeof(tss);	// 没有I/O许可位图

	// 填充 GDT 中进程的 LDT 的描述符
	int i;
	PROCESS* p_proc	= proc_table;
	u16 selector_ldt = INDEX_LDT_FIRST << 3;
	//for(i=0;i<NR_TASKS;i++){
	for(i=0;i<NR_PCBS;i++){										//edit by visual 2016.4.5
		init_descriptor(&gdt[selector_ldt>>3],
				vir2phys(seg2phys(SELECTOR_KERNEL_DS),proc_table[i].task.context.ldts),
				LDT_SIZE * sizeof(DESCRIPTOR) - 1,
				DA_LDT);
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
PUBLIC void cstart()
{
	kernel_initial = 1;
	#ifdef OPT_DISP_SERIAL
	init_simple_serial();
	#endif
	disp_str("\n\n\n\n\n\n-----\"cstart\" begins-----\n");

	memory_init(); //moved from kernel_main, mingxuan 2021-8-25

	if (-1 == init_kernel_page())
	{
		disp_color_str("reinit_kernel_page error!\n", 0x74);
	};

	// 将 LOADER 中的 GDT 复制到新的 GDT 中
	/*	//deleted by mingxuan 2021-8-25
	memcpy(	&gdt,				    // New GDT
		(void*)(*((u32*)(&gdt_ptr[2]))),   // Base  of Old GDT
		*((u16*)(&gdt_ptr[0])) + 1	    // Limit of Old GDT
		);
	// gdt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sgdt 以及 lgdt 的参数。
	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
	u32* p_gdt_base  = (u32*)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
	*p_gdt_base  = (u32)&gdt;
	*/

	//modified by mingxuan 2021-8-25
	/*
	//init_gdt 只有4个段
	init_descriptor(&gdt[INDEX_DUMMY], 0, 0, 0);
	init_descriptor(&gdt[INDEX_FLAT_C], 0, 0x0fffff, DA_CR | DA_32 | DA_LIMIT_4K);
	init_descriptor(&gdt[INDEX_FLAT_RW], 0, 0x0fffff, DA_DRWA | DA_32 | DA_LIMIT_4K);
	init_descriptor(&gdt[INDEX_VIDEO], 0x0B8000, 0x0ffff, DA_DRWA | DA_DPL3);

	//修改显存描述符 //add by visual 2016.5.12
	init_descriptor(&gdt[INDEX_VIDEO],
					K_PHY2LIN(0x0B8000),
					0x0ffff,
					DA_DRW | DA_DPL3);

	// 填充 GDT 中 TSS 这个描述符
	memset(&tss, 0, sizeof(tss));
	tss.ss0		= SELECTOR_KERNEL_DS;
	init_descriptor(&gdt[INDEX_TSS],
			vir2phys(seg2phys(SELECTOR_KERNEL_DS), &tss),
			sizeof(tss) - 1,
			DA_386TSS);
	tss.iobase	= sizeof(tss);	// 没有I/O许可位图

	// 填充 GDT 中进程的 LDT 的描述符
	int i;
	PROCESS* p_proc	= proc_table;
	u16 selector_ldt = INDEX_LDT_FIRST << 3;
	//for(i=0;i<NR_TASKS;i++){
	for(i=0;i<NR_PCBS;i++){										//edit by visual 2016.4.5
		init_descriptor(&gdt[selector_ldt>>3],
				vir2phys(seg2phys(SELECTOR_KERNEL_DS),proc_table[i].task.ldts),
				LDT_SIZE * sizeof(DESCRIPTOR) - 1,
				DA_LDT);
		p_proc++;
		selector_ldt += 1 << 3;
	}

	u16 *p_gdt_limit = (u16 *)(&gdt_ptr[0]);
	u32 *p_gdt_base = (u32 *)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
	*p_gdt_base = (u32)&gdt;
	*/

	init_gdt();
	refresh_gdt();	//added by sundong 2023.3.8 更新gdtr的值，并且刷新gs、es、ds、cs的隐藏部分。

	// idt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sidt 以及 lidt 的参数。
	/*
	u16 *p_idt_limit = (u16 *)(&idt_ptr[0]);
	u32 *p_idt_base = (u32 *)(&idt_ptr[2]);
	*p_idt_limit = IDT_SIZE * sizeof(GATE) - 1;
	*p_idt_base = (u32)&idt;
	*/

	//init_prot();
	init_8259A();	//added by mingxuan 2021-8-29
	init_idt();		//added by mingxuan 2021-8-29

	disp_str("-----\"cstart\" finished-----\n");
	// while(1);
}

/*======================================================================*
                           init_page_tbl		delete by visual 2016.4.19
 *======================================================================
PUBLIC	void init_page_tbl()
{
	PageTblNum = *(u32*)PageTblNumAddr;		//读取保存的内存信息
	//disp_int(PageTblNum);

	int i,j;
	int *addr_temp;
	int	*value_temp;
	int temp;

	for(i=0;  i<NR_PCBS ; i++)
	{//NR_PCBS是进程表的数量,每个进程一个页目录
		addr_temp =	(u32*)PageDirBaseProc + 1024*(PageTblNum+1) * i;
		value_temp = addr_temp + 1024;
		for(j=0; j<PageTblNum; j++)
		{//页目录
			*(addr_temp) = (((u32)value_temp) & 0xFFFFF000) | PG_P  | PG_USU | PG_RWW;	//1111 1111 1111 1111 0000 0000 0000
			addr_temp++;
			value_temp += 1024;
		}

		addr_temp = (u32*)PageDirBaseProc + 1024*(PageTblNum+1) * i + 1024;//跳过页目录
		value_temp = 0; //用这个变量,表示物理地址,从0开始
		for(j=1024; j<2048 ; j++)
		{//第一页,与前4M物理地址一一对应
			*(addr_temp) = (((u32)value_temp) & 0xFFFFF000) | PG_P  | PG_USU | PG_RWW;
			addr_temp++;
			value_temp += 1024;
		}

		temp = 1024 *(PageTblNum+1);
		for(  ; j<temp ; j++)
		{//其它页也与物理地址一一对应
			*(addr_temp) = (((u32)value_temp) & 0xFFFFF000) | PG_P  | PG_USU | PG_RWW;  //1111 1111 1111 1111 0000 0000 0000
			addr_temp++;
			value_temp += 1024;
		}
	}
}*/
