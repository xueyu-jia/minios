#pragma once

#include <klib/stdint.h>
#include <compiler.h>

#ifdef GCC_COMPILER
#define ASMCALL __attribute__((optimize("O3"))) static inline
#else
#define ASMCALL static inline
#endif

ASMCALL u8 inb(int port) {
    u8 data;
    asm volatile("inb %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

ASMCALL void insb(int port, void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "insb\n"
        : "=D"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "memory", "cc");
}

ASMCALL u16 inw(int port) {
    u16 data;
    asm volatile("inw %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

ASMCALL void insw(int port, void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "insw\n"
        : "=D"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "memory", "cc");
}

ASMCALL u32 inl(int port) {
    u32 data;
    asm volatile("inl %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

ASMCALL void insl(int port, void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "insl\n"
        : "=D"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "memory", "cc");
}

ASMCALL void outb(int port, u8 data) {
    asm volatile("outb %0, %w1" : : "a"(data), "d"(port));
}

ASMCALL void outsb(int port, const void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "outsb\n"
        : "=S"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "cc");
}

ASMCALL void outw(int port, u16 data) {
    asm volatile("outw %0, %w1" : : "a"(data), "d"(port));
}

ASMCALL void outsw(int port, const void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "outsw\n"
        : "=S"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "cc");
}

ASMCALL void outsl(int port, const void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "outsl\n"
        : "=S"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "cc");
}

ASMCALL void outl(int port, u32 data) {
    asm volatile("outl %0, %w1" : : "a"(data), "d"(port));
}

ASMCALL void lcr0(u32 val) {
    asm volatile("movl %0, %%cr0" : : "r"(val));
}

ASMCALL u32 rcr0() {
    u32 val;
    asm volatile("movl %%cr0, %0" : "=r"(val));
    return val;
}

ASMCALL u32 rcr2() {
    u32 val;
    asm volatile("movl %%cr2, %0" : "=r"(val));
    return val;
}

ASMCALL void lcr3(u32 val) {
    asm volatile("movl %0, %%cr3" : : "r"(val));
}

ASMCALL u32 rcr3() {
    u32 val;
    asm volatile("movl %%cr3, %0" : "=r"(val));
    return val;
}

ASMCALL void lcr4(u32 val) {
    asm volatile("movl %0, %%cr4" : : "r"(val));
}

ASMCALL u32 rcr4() {
    u32 cr4;
    asm volatile("movl %%cr4, %0" : "=r"(cr4));
    return cr4;
}

ASMCALL void tlbflush() {
    asm volatile(
        "mov %cr3, %eax\n"
        "mov %eax, %cr3\n");
}

ASMCALL u32 read_eflags() {
    u32 eflags;
    asm volatile(
        "pushfl\n"
        "popl %0\n"
        : "=r"(eflags));
    return eflags;
}

ASMCALL void write_eflags(u32 eflags) {
    asm volatile(
        "pushl %0\n"
        "popfl\n"
        :
        : "r"(eflags));
}

ASMCALL u32 read_ebp() {
    u32 ebp;
    asm volatile("movl %%ebp, %0" : "=r"(ebp));
    return ebp;
}

ASMCALL u32 read_esp() {
    u32 esp;
    asm volatile("movl %%esp, %0" : "=r"(esp));
    return esp;
}

ASMCALL u64 rdtsc() {
    return __builtin_ia32_rdtsc();
}

ASMCALL u32 xchg(volatile u32 *addr, u32 newval) {
    u32 result;
    asm volatile(
        "lock\n"
        "xchgl %0, %1\n"
        : "+m"(*addr), "=a"(result)
        : "1"(newval)
        : "cc");
    return result;
}

ASMCALL void clear_dir_flag() {
    asm volatile("cld");
}

ASMCALL void disable_int() {
    asm volatile("cli");
}

ASMCALL void enable_int() {
    asm volatile("sti");
}

ASMCALL void halt() {
    asm volatile("hlt");
}

ASMCALL void cpuid(u32 id, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx) {
    asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(id), "c"(0));
}

ASMCALL u32 cpuid_eax(u32 id) {
    u32 eax = 0;
    asm volatile("cpuid" : "=a"(eax) : "a"(id), "c"(0));
    return eax;
}

ASMCALL u32 cpuid_ebx(u32 id) {
    u32 ebx = 0;
    asm volatile("cpuid" : "=b"(ebx) : "a"(id), "c"(0));
    return ebx;
}

ASMCALL u32 cpuid_ecx(u32 id) {
    u32 ecx = 0;
    asm volatile("cpuid" : "=c"(ecx) : "a"(id), "c"(0));
    return ecx;
}

ASMCALL u32 cpuid_edx(u32 id) {
    u32 edx = 0;
    asm volatile("cpuid" : "=d"(edx) : "a"(id), "c"(0));
    return edx;
}

#undef ASMCALL
