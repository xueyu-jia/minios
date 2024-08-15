#pragma once

#include <kernel/stdint.h>

#define ASMCALL __attribute__((optimize("O3"))) static inline

ASMCALL u8 inb(int port) {
    u8 data;
    __asm__ volatile("inb %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

ASMCALL void insb(int port, void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "insb\n"
                     : "=D"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "memory", "cc");
}

ASMCALL u16 inw(int port) {
    u16 data;
    __asm__ volatile("inw %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

ASMCALL void insw(int port, void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "insw\n"
                     : "=D"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "memory", "cc");
}

ASMCALL u32 inl(int port) {
    u32 data;
    __asm__ volatile("inl %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

ASMCALL void insl(int port, void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "insl\n"
                     : "=D"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "memory", "cc");
}

ASMCALL void outb(int port, u8 data) {
    __asm__ volatile("outb %0, %w1" : : "a"(data), "d"(port));
}

ASMCALL void outsb(int port, const void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "outsb\n"
                     : "=S"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "cc");
}

ASMCALL void outw(int port, u16 data) {
    __asm__ volatile("outw %0, %w1" : : "a"(data), "d"(port));
}

ASMCALL void outsw(int port, const void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "outsw\n"
                     : "=S"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "cc");
}

ASMCALL void outsl(int port, const void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "outsl\n"
                     : "=S"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "cc");
}

ASMCALL void outl(int port, u32 data) {
    __asm__ volatile("outl %0, %w1" : : "a"(data), "d"(port));
}

ASMCALL void lcr0(u32 val) {
    __asm__ volatile("movl %0, %%cr0" : : "r"(val));
}

ASMCALL u32 rcr0() {
    u32 val;
    __asm__ volatile("movl %%cr0, %0" : "=r"(val));
    return val;
}

ASMCALL u32 rcr2() {
    u32 val;
    __asm__ volatile("movl %%cr2, %0" : "=r"(val));
    return val;
}

ASMCALL void lcr3(u32 val) {
    __asm__ volatile("movl %0, %%cr3" : : "r"(val));
}

ASMCALL u32 rcr3() {
    u32 val;
    __asm__ volatile("movl %%cr3, %0" : "=r"(val));
    return val;
}

ASMCALL void lcr4(u32 val) {
    __asm__ volatile("movl %0, %%cr4" : : "r"(val));
}

ASMCALL u32 rcr4() {
    u32 cr4;
    __asm__ volatile("movl %%cr4, %0" : "=r"(cr4));
    return cr4;
}

ASMCALL void tlbflush() {
    __asm__ volatile("mov %cr3, %eax\n"
                     "mov %eax, %cr3\n");
}

ASMCALL u32 read_eflags() {
    u32 eflags;
    __asm__ volatile("pushfl\n"
                     "popl %0\n"
                     : "=r"(eflags));
    return eflags;
}

ASMCALL void write_eflags(u32 eflags) {
    __asm__ volatile("pushl %0\n"
                     "popfl\n"
                     :
                     : "r"(eflags));
}

ASMCALL u32 read_ebp() {
    u32 ebp;
    __asm__ volatile("movl %%ebp, %0" : "=r"(ebp));
    return ebp;
}

ASMCALL u32 read_esp() {
    u32 esp;
    __asm__ volatile("movl %%esp, %0" : "=r"(esp));
    return esp;
}

ASMCALL u64 read_tsc() {
    u64 tsc;
    __asm__ volatile("rdtsc" : "=A"(tsc));
    return tsc;
}

ASMCALL u32 xchg(volatile u32 *addr, u32 newval) {
    u32 result;
    __asm__ volatile("lock\n"
                     "xchgl %0, %1\n"
                     : "+m"(*addr), "=a"(result)
                     : "1"(newval)
                     : "cc");
    return result;
}

ASMCALL void clear_dir_flag() {
    __asm__ volatile("cld");
}

ASMCALL void disable_int() {
    __asm__ volatile("cli");
}

ASMCALL void enable_int() {
    __asm__ volatile("sti");
}

ASMCALL void halt() {
    __asm__ volatile("hlt");
}

#undef ASMCALL


#include <kernel/protect.h>

// ring0权限
#define kern_cs ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_KRNL
#define kern_ds ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_KRNL
#define kern_es ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_KRNL
#define kern_fs ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_KRNL
#define kern_ss ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_KRNL
#define kern_gs (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_KRNL
// ring1权限
#define task_cs ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK
#define task_ds ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK
#define task_es ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK
#define task_fs ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK
#define task_ss ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK
#define task_gs (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK

// 切换进程要保存的寄存器
typedef struct s_stackframe {	/* proc_ptr points here				↑ Low			*/
	u32	gs;			/* ┓						│			*/
	u32	fs;			/* ┃						│			*/
	u32	es;			/* ┃						│			*/
	u32	ds;			/* ┃						│			*/
	u32	edi;		/* ┃						│			*/
	u32	esi;		/* ┣ pushed by save()				│			*/
	u32	ebp;		/* ┃						│			*/
	u32	kernel_esp;	/* <- 'popad' will ignore it			│			*/
	u32	ebx;		/* ┃						↑栈从高地址往低地址增长*/
	u32	edx;		/* ┃						│			*/
	u32	ecx;		/* ┃						│			*/
	u32	eax;		/* ┛						│			*/
	u32	retaddr;	/* return address for assembly code save()	│			*/
	u32	error;		//add by lcy 2023.10.26
	u32	eip;		/*  ┓						│			*/
	u32	cs;			/*  ┃						│			*/
	u32	eflags;		/*  ┣ these are pushed by CPU during interrupt	│			*/
	u32	esp;		/*  ┃						│			*/
	u32	ss;			/*  ┛						┷High			*/
}__attribute__((packed)) STACK_FRAME;
// } STACK_FRAME;

typedef struct CONTEXT_FRAME{
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 esp;
	u32 ebx;
	u32 edx;
	u32 ecx;
	u32 eax;			// 以上均由pushad压栈
	u32 eflags;			// 由 pushfd 压栈
	u32 eip;			// 由函数调用自动压栈
}__attribute__((packed)) CONTEXT_FRAME;
// } CONTEXT_FRAME;
/*
因为x86版本使用偏移量访问变量，所以该结构体内部成员请保持原有顺序
注意：sconst.inc文件中规定了变量间的偏移值，新添变量不要破坏原有顺序结构
*/
typedef struct cpu_context{
	u16 ldt_sel;               /* gdt selector giving ldt base and limit */
	DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data */

	STACK_FRAME* esp_save_int;	// 需要保存的寄存器，定义在各个架构的processor.h中
	//char* esp_save_int;		//to save the position of esp in the kernel stack of the process
							//added by xw, 17/12/11
	// char* esp_save_syscall;	//to save the position of esp in the kernel stack of the process
	CONTEXT_FRAME* esp_save_context;	//to save the position of esp in the kernel stack of the process

} cpu_context;  // 加上关键字__attribute__((packed))反而报错了

void init_cpu_context(cpu_context *context, int pid, u32 int_eip, u32 int_esp, u32 context_eip, int rpl);
