#include <minios/protect.h>

tss_t tss;
descptr_t gdt_ptr;
descriptor_t gdt[GDT_SIZE];
descptr_t idt_ptr;
gate_descriptor_t idt[IDT_SIZE];

void init_descriptor(descriptor_t* desc, uint32_t base, uint32_t limit, uint16_t attr) {
    desc->limit0 = limit & 0xffff;
    desc->base0 = base & 0xffff;
    desc->base1 = (base >> 16) & 0xff;
    desc->attr0 = attr & 0xff;
    desc->attr1_limit1 = ((limit >> 16) & 0x0f) | ((attr >> 8) & 0xf0);
    desc->base2 = (base >> 24) & 0xff;
}

uint32_t seg2phys(uint16_t seg) {
    const descriptor_t* desc = &gdt[SELECTOR_GET_INDEX(seg)];
    return (desc->base2 << 24) | (desc->base1 << 16) | desc->base0;
}
