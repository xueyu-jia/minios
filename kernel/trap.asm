%include "off_consts.inc"

; external fn
extern irq_router
extern sched
extern restore
extern restart_restore
extern exception_handler
extern page_fault_handler
extern general_protection_handler

; external var
extern kstate_reenter_cntr
extern kstate_on_init

[bits 32]

[section .bss]

irq_stack_space resb 0x1000
irq_stack_top:

[section .text]

; impl_irq_handler <irq-name>, <irq-id>
%macro impl_irq_handler 2
    global %1
    align 16
%1:
    push    0xffffffff                  ; push dummy value to the err code place in the context
    call    save_int
    inc     dword [kstate_reenter_cntr]
    push    %2
    call    irq_router
    add     esp, 4
    dec     dword [kstate_reenter_cntr]
    cmp     dword [kstate_reenter_cntr], 0
    jnz     restore
    pop     esp
    cmp     dword [kstate_on_init], 0
    jnz     restore
    call    sched
    jmp     restart_restore
%endmacro

; impl_exception_no_errcode <exception-name>, <vec-no>, <handler>
%macro impl_exception_no_errcode 3
    global %1
%1:
    push    0xffffffff                  ; push dummy err code
    call    save_exception              ; save context
    mov     esi, esp                    ; shift to exception context
    add     esi, 4                      ; here 4 refs to the pushed restart_exception in save_exception
    mov     eax, [esi + OFF_EFLAGS]     ; push exception handler args -->
    push    eax                         ;
    mov     eax, [esi + OFF_CS]         ;
    push    eax                         ;
    mov     eax, [esi + OFF_EIP]        ;
    push    eax                         ;
    mov     eax, [esi + OFF_ERROR]      ;
    push    eax                         ;
    push    %2                          ; <--
    sti                                 ; run exception handler -->
    call    %3                          ;
    cli                                 ; <--
    add     esp, 4 * 5                  ; clean up args
    ret                                 ; transfer ctrl to pushed restart_exception
%endmacro

; impl_exception_errcode <exception-name>, <vec-no>, <handler>
%macro impl_exception_errcode 3
    global %1
%1:
    call    save_exception              ; save context
    mov     esi, esp                    ; shift to exception context
    add     esi, 4                      ; here 4 refs to the pushed restart_exception in save_exception
    mov     eax, [esi + OFF_EFLAGS]     ; push exception handler args -->
    push    eax                         ;
    mov     eax, [esi + OFF_CS]         ;
    push    eax                         ;
    mov     eax, [esi + OFF_EIP]        ;
    push    eax                         ;
    mov     eax, [esi + OFF_ERROR]      ;
    push    eax                         ;
    push    %2                          ; <--
    sti                                 ; run exception handler -->
    call    %3                          ;
    cli                                 ; <--
    add     esp, 4 * 5                  ; clean up args
    ret                                 ; transfer ctrl to pushed restart_exception
%endmacro

save_exception:
    pushad          ; save context -->
    push    ds      ;
    push    es      ;
    push    fs      ;
    push    gs      ; <--
    mov     dx, ss
    mov     ds, dx
    mov     es, dx
    mov     fs, dx
    mov     gs, dx
    mov     esi, esp
    push    restart_exception
    jmp     [esi + OFF_RETADDR]

restart_exception:
    call    sched
    pop     gs      ; restore context -->
    pop     fs      ;
    pop     es      ;
    pop     ds      ;
    popad           ; <--
    ; clear retaddr and error code in stack
    add     esp, 4 * 2
    iretd

save_int:
    pushad          ; save context -->
    push    ds      ;
    push    es      ;
    push    fs      ;
    push    gs      ; <--
    cmp     dword [kstate_reenter_cntr], 0
    jnz     .saved

    mov     dx, ss
    mov     ds, dx
    mov     es, dx
    mov     fs, dx
    mov     gs, dx

    ; switch to irq-stack
    mov     esi, esp
    mov     esp, irq_stack_top
    push    esi

    jmp     [esi + OFF_RETADDR]
.saved:
    ; already in irq stack
    jmp     [esp + OFF_RETADDR]

impl_irq_handler hwint00, 0  ; interrupt routine for irq 0 (the clock)
impl_irq_handler hwint01, 1  ; interrupt routine for irq 1 (keyboard)
impl_irq_handler hwint02, 2  ; interrupt routine for irq 2 (cascade)
impl_irq_handler hwint03, 3  ; interrupt routine for irq 3 (second serial)
impl_irq_handler hwint04, 4  ; interrupt routine for irq 4 (first serial)
impl_irq_handler hwint05, 5  ; interrupt routine for irq 5 (XT winchester)
impl_irq_handler hwint06, 6  ; interrupt routine for irq 6 (floppy)
impl_irq_handler hwint07, 7  ; interrupt routine for irq 7 (printer)
impl_irq_handler hwint08, 8  ; interrupt routine for irq 8 (realtime clock)
impl_irq_handler hwint09, 9  ; interrupt routine for irq 9 (irq 2 redirected)
impl_irq_handler hwint10, 10 ; interrupt routine for irq 10
impl_irq_handler hwint11, 11 ; interrupt routine for irq 11
impl_irq_handler hwint12, 12 ; interrupt routine for irq 12
impl_irq_handler hwint13, 13 ; interrupt routine for irq 13 (fpu exception)
impl_irq_handler hwint14, 14 ; interrupt routine for irq 14 (AT winchester)
impl_irq_handler hwint15, 15 ; interrupt routine for irq 15

impl_exception_no_errcode division_error,           0,  exception_handler          ; division error, fault, #DE, no error code
impl_exception_no_errcode debug_exception,          1,  exception_handler          ; debug exception, fault/trap, #DB, no error code
impl_exception_no_errcode nmi,                      2,  exception_handler          ; non-maskable interrupt, int, \, no error code
impl_exception_no_errcode breakpoint_exception,     3,  exception_handler          ; breakpoint exception, trap, #BP, no error code
impl_exception_no_errcode overflow_exception,       4,  exception_handler          ; overflow exception, trap, #OF, no error code
impl_exception_no_errcode bound_range_exceeded,     5,  exception_handler          ; bound range exceeded exception, fault, #BR, no error code
impl_exception_no_errcode invalid_opcode,           6,  exception_handler          ; invalid opcode, fault, #UD, no error code
impl_exception_no_errcode device_not_available,     7,  exception_handler          ; device not available, fault, #NM, no error code
impl_exception_errcode    double_fault,             8,  exception_handler          ; double fault, abort, #DF, error code = 0
impl_exception_no_errcode copr_seg_overrun,         9,  exception_handler          ; coprocessor segment overrun, fault, \, no error code
impl_exception_errcode    invalid_tss,              10, exception_handler          ; invalid tss, fault, #TS, error code
impl_exception_errcode    segment_not_present,      11, exception_handler          ; segment not present, fault, #NP, error code
impl_exception_errcode    stack_seg_exception,      12, exception_handler          ; stack-segment fault, fault, #SS, error code
impl_exception_errcode    general_protection,       13, general_protection_handler ; general protection fault, fault, #PF, error code
impl_exception_errcode    page_fault,               14, page_fault_handler         ; page fault, fault, #PF, error code
impl_exception_no_errcode floating_point_exception, 16, exception_handler          ; floating-point exception, fault, #MF, no error code
