%include "off_consts.inc"

TSS3_S_SP0 equ 4

INIT_STACK_SIZE equ 1024 * 8

SELECTOR_FLAT_C    equ 0x08
SELECTOR_FLAT_RW   equ 0x10
SELECTOR_TSS       equ 0x20
SELECTOR_KERNEL_CS equ SELECTOR_FLAT_C
SELECTOR_VIDEO     equ 0x1b

; external fn
extern cstart
extern kernel_main
extern schedule
extern process_signal

; external var
extern gdt_ptr
extern idt_ptr
extern tss
extern syscall_table
extern cr3_ready
extern p_proc_current
extern p_proc_next

[bits 32]

[section .bss]

kernel_stack_space resb 0x1000
kernel_stask_top:

[section .text]

global restart_initial
global restart_restore
global sched
global sys_call
global refresh_gdt

    global _start
_start:
    mov     esp, kernel_stask_top
    push    edx                 ; multiboot info phy addr from bootloader
    call    cstart

    lgdt    [gdt_ptr]
    lidt    [idt_ptr]

    mov     ax, SELECTOR_TSS
    ltr     ax

    jmp     SELECTOR_KERNEL_CS:csinit
csinit:
    mov     ax, SELECTOR_FLAT_RW
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    call    kernel_main
    ; NOTE: force a dead halt here although kernel_main is assumed to be noreturn
.1:
    hlt
    jmp     .1

renew_env:
    mov     eax, [cr3_ready]
    mov     cr3, eax                      ; switch pdbr
    mov     eax, [p_proc_current]
    lldt    [eax + OFF_LDT_SEL]           ; switch ldt
    lea     ebx, [eax + INIT_STACK_SIZE]
    mov     dword [tss + TSS3_S_SP0], ebx ; switch esp0
    ret

    global refresh_gdt
refresh_gdt:
    lgdt    [gdt_ptr]

    mov     ax, SELECTOR_FLAT_RW
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    ret

    global sched
sched:
    pushfd
    pushad
    cli
    mov     ebx,  [p_proc_current]
    mov     dword [ebx + OFF_ESP_SAVE_CONTEXT], esp
    call    schedule
    mov     ebx,  [p_proc_next]
    mov     dword [p_proc_current], ebx
    call    renew_env
    mov     ebx, [p_proc_current]
    mov     esp, [ebx + OFF_ESP_SAVE_CONTEXT]
    popad
    popfd
    ret

    global restart_restore
restart_restore:
    call    process_signal
    jmp     restore

    global restore
restore:
    pop     gs
    pop     fs
    pop     es
    pop     ds
    popad
    add     esp, 4 * 2
    iretd

    global restart_initial
restart_initial:
    call    renew_env
    mov     ebx, [p_proc_current]
    mov     esp, [ebx + OFF_ESP_SAVE_CONTEXT]
    popad
    popfd
    ret

save_syscall:
    pushad
    push    ds
    push    es
    push    fs
    push    gs
    mov     edx,  [p_proc_current]
    mov     dword [edx + OFF_ESP_SAVE_INT], esp

    mov     dx, ss
    mov     ds, dx
    mov     es, dx
    mov     fs, dx
    mov     gs, dx

    mov     esi, esp
    jmp     [esi + OFF_RETADDR]

    global sys_call
sys_call:
    ; ATTENTION: do not modify regs pass to syscall as args before the routine is done
    push    0xffffffff
    call    save_syscall
    sti
    call    [syscall_table + eax * 4]
    cli
    mov     edx, [p_proc_current]
    mov     esi, [edx + OFF_ESP_SAVE_INT]
    mov     [esi + OFF_EAX], eax
    mov     eax, [p_proc_current]
    mov     esp, [eax + OFF_ESP_SAVE_INT]
    call    sched
    jmp     restart_restore
