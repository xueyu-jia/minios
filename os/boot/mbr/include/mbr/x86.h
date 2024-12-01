#ifndef MINIOS_X86_H
#define MINIOS_X86_H
// ported by sundong 2023.3.26

#include "type.h"
static inline u8 inb(int port) {
  u8 data;
  asm volatile("inb %w1,%0" : "=a"(data) : "d"(port));
  return data;
}
static inline void insl(int port, void *addr, int cnt) {
  asm volatile("cld\n\trepne\n\tinsl"
               : "=D"(addr), "=c"(cnt)
               : "d"(port), "0"(addr), "1"(cnt)
               : "memory", "cc");
}

static inline void outb(int port, u8 data) {
  asm volatile("outb %0,%w1" : : "a"(data), "d"(port));
}
static inline void lcr0(u32 val) { asm volatile("movl %0,%%cr0" : : "r"(val)); }

static inline u32 rcr0(void) {
  u32 val;
  asm volatile("movl %%cr0,%0" : "=r"(val));
  return val;
}
static inline void lcr3(u32 val) { asm volatile("movl %0,%%cr3" : : "r"(val)); }

static inline u32 rcr3(void) {
  u32 val;
  asm volatile("movl %%cr3,%0" : "=r"(val));
  return val;
}
static inline void outl(int port, u32 data) {
  asm volatile("outl %0,%w1" : : "a"(data), "d"(port));
}
static inline u32 inl(int port) {
  u32 data;
  asm volatile("inl %w1,%0" : "=a"(data) : "d"(port));
  return data;
}
#define out_dword(port, data) outl(port, data)
#define in_dword(port) inl(port)

#endif /* MINIOS_X86_H */