/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-14 13:24:41
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-18 22:18:38
 * @FilePath: /minios/os/kernel/protect.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */

#include <kernel/const.h>
#include <kernel/protect.h>
#include <kernel/proc.h>
#include <kernel/proto.h>
#include <kernel/console.h>
#include <kernel/interrupt_x86.h>

u32		k_reenter;
TSS		tss;

u8		gdt_ptr[6];	// 0~15:Limit  16~47:Base
DESCRIPTOR	gdt[GDT_SIZE];

/*======================================================================*
                           seg2phys
 *----------------------------------------------------------------------*
 由段名求绝对地址
 *======================================================================*/
PUBLIC u32 seg2phys(u16 seg)
{
	DESCRIPTOR* p_dest = &gdt[seg >> 3];

	return (p_dest->base_high << 24) | (p_dest->base_mid << 16) | (p_dest->base_low);
}

/*======================================================================*
                           init_descriptor
 *----------------------------------------------------------------------*
 初始化段描述符
 *======================================================================*/
PUBLIC void init_descriptor(DESCRIPTOR * p_desc, u32 base, u32 limit, u16 attribute)	//modified by mingxuan 2021-8-25
{
	p_desc->limit_low		= limit & 0x0FFFF;		// 段界限 1		(2 字节)
	p_desc->base_low		= base & 0x0FFFF;		// 段基址 1		(2 字节)
	p_desc->base_mid		= (base >> 16) & 0x0FF;		// 段基址 2		(1 字节)
	p_desc->attr1			= attribute & 0xFF;		// 属性 1
	p_desc->limit_high_attr2	= ((limit >> 16) & 0x0F) |
						(attribute >> 8) & 0xF0;// 段界限 2 + 属性 2
	p_desc->base_high		= (base >> 24) & 0x0FF;		// 段基址 3		(1 字节)
}
