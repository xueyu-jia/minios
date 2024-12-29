#pragma once

#include <compiler.h>
#include <stdint.h>

static inline uint8_t inb(int port) {
    uint8_t data;
    asm volatile("inb %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

static inline void insb(int port, void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "insb\n"
        : "=D"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "memory", "cc");
}

static inline uint16_t inw(int port) {
    uint16_t data;
    asm volatile("inw %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

static inline void insw(int port, void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "insw\n"
        : "=D"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "memory", "cc");
}

static inline uint32_t inl(int port) {
    uint32_t data;
    asm volatile("inl %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

static inline void insl(int port, void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "insl\n"
        : "=D"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "memory", "cc");
}

static inline void outb(int port, uint8_t data) {
    asm volatile("outb %0, %w1" : : "a"(data), "d"(port));
}

static inline void outsb(int port, const void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "outsb\n"
        : "=S"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "cc");
}

static inline void outw(int port, uint16_t data) {
    asm volatile("outw %0, %w1" : : "a"(data), "d"(port));
}

static inline void outsw(int port, const void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "outsw\n"
        : "=S"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "cc");
}

static inline void outsl(int port, const void *addr, int cnt) {
    asm volatile(
        "cld\n"
        "repne\n"
        "outsl\n"
        : "=S"(addr), "=c"(cnt)
        : "d"(port), "0"(addr), "1"(cnt)
        : "cc");
}

static inline void outl(int port, uint32_t data) {
    asm volatile("outl %0, %w1" : : "a"(data), "d"(port));
}

static inline void lcr0(uint32_t val) {
    asm volatile("movl %0, %%cr0" : : "r"(val));
}

static inline uint32_t rcr0() {
    uint32_t val;
    asm volatile("movl %%cr0, %0" : "=r"(val));
    return val;
}

static inline uint32_t rcr2() {
    uint32_t val;
    asm volatile("movl %%cr2, %0" : "=r"(val));
    return val;
}

static inline void lcr3(uint32_t val) {
    asm volatile("movl %0, %%cr3" : : "r"(val));
}

static inline uint32_t rcr3() {
    uint32_t val;
    asm volatile("movl %%cr3, %0" : "=r"(val));
    return val;
}

static inline void lcr4(uint32_t val) {
    asm volatile("movl %0, %%cr4" : : "r"(val));
}

static inline uint32_t rcr4() {
    uint32_t cr4;
    asm volatile("movl %%cr4, %0" : "=r"(cr4));
    return cr4;
}

static inline void tlbflush() {
    asm volatile(
        "mov %cr3, %eax\n"
        "mov %eax, %cr3\n");
}

static inline uint32_t read_eflags() {
    uint32_t eflags;
    asm volatile(
        "pushfl\n"
        "popl %0\n"
        : "=r"(eflags));
    return eflags;
}

static inline void write_eflags(uint32_t eflags) {
    asm volatile(
        "pushl %0\n"
        "popfl\n"
        :
        : "r"(eflags));
}

static inline uint32_t read_ebp() {
    uint32_t ebp;
    asm volatile("movl %%ebp, %0" : "=r"(ebp));
    return ebp;
}

static inline uint32_t read_esp() {
    uint32_t esp;
    asm volatile("movl %%esp, %0" : "=r"(esp));
    return esp;
}

static inline uint64_t read_tsc() {
    uint64_t tsc;
    asm volatile("rdtsc" : "=A"(tsc));
    return tsc;
}

static inline uint32_t xchg(volatile uint32_t *addr, uint32_t newval) {
    uint32_t result;
    asm volatile(
        "lock\n"
        "xchgl %0, %1\n"
        : "+m"(*addr), "=a"(result)
        : "1"(newval)
        : "cc");
    return result;
}

static inline void clear_dir_flag() {
    asm volatile("cld");
}

static inline void disable_int() {
    asm volatile("cli");
}

static inline void enable_int() {
    asm volatile("sti");
}

static inline void halt() {
    asm volatile("hlt");
}
