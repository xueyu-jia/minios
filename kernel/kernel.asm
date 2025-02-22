P_STACKBASE	equ	0
;用作中断栈帧中，数据相对偏移量的计算
GSREG		equ	P_STACKBASE
FSREG		equ	GSREG		+ 4
ESREG		equ	FSREG		+ 4
DSREG		equ	ESREG		+ 4
EDIREG		equ	DSREG		+ 4
ESIREG		equ	EDIREG		+ 4
EBPREG		equ	ESIREG		+ 4
KERNELESPREG	equ	EBPREG		+ 4
EBXREG		equ	KERNELESPREG	+ 4
EDXREG		equ	EBXREG		+ 4
ECXREG		equ	EDXREG		+ 4
EAXREG		equ	ECXREG		+ 4
RETADR		equ	EAXREG		+ 4
ERROR       equ	RETADR		+ 4 ;added by lcy 2023.10.26
EIPREG		equ	ERROR		+ 4
CSREG		equ	EIPREG		+ 4
EFLAGSREG	equ	CSREG		+ 4
ESPREG		equ	EFLAGSREG	+ 4
SSREG		equ	ESPREG		+ 4
P_STACKTOP	equ	SSREG		+ 4

P_LDT_SEL	equ	P_STACKBASE  ;changed by lcy 2023.10.25 PCB已修改，P_LDT_SEL为偏移量0
P_LDT		equ	P_LDT_SEL	+ 4
;added by xw
;begin
INIT_STACK_SIZE   equ  1024 * 8
ESP_SAVE_INT      equ  P_LDT + 16
;ESP_SAVE_SYSCALL  equ  ESP_SAVE_INT + 4
ESP_SAVE_CONTEXT  equ  ESP_SAVE_INT + 4
;SAVE_TYPE		  equ  ESP_SAVE_CONTEXT + 4		;Deleted by xw, 18/4/19
;end
;ESP_SAVE_SYSCALL_ARG  equ  ESP_SAVE_CONTEXT + 4 ;added by zhenhao 2023.3.5

TSS3_S_SP0	equ	4

INT_M_CTL	equ	0x20	; I/O port for interrupt controller         <Master>
INT_M_CTLMASK	equ	0x21	; setting bits in this port disables ints   <Master>
INT_S_CTL	equ	0xA0	; I/O port for second interrupt controller  <Slave>
INT_S_CTLMASK	equ	0xA1	; setting bits in this port disables ints   <Slave>

EOI		equ	0x20

; 以下选择子值必须与 protect.h 中保持一致!!!
SELECTOR_FLAT_C		equ		0x08		; LOADER 里面已经确定了的.
SELECTOR_FLAT_RW	equ     0x10        ;added by sundong 2023.3.8
SELECTOR_TSS		equ		0x20		; TSS. 从外层跳到内存时 SS 和 ESP 的值从里面获得.
SELECTOR_KERNEL_CS	equ		SELECTOR_FLAT_C
SELECTOR_VIDEO		equ		0x1b		; added by xw, 18/6/20

; 导入函数
extern	cstart
extern	kernel_main
extern	irq_table
extern	exception_handler
extern	page_fault_handler
extern  general_protection_handler
extern  schedule
; extern  switch_pde
extern 	process_signal	;added by mingxuan 2021-2-28

; 导入全局变量
extern	gdt_ptr
extern	idt_ptr
extern	tss
extern	syscall_table
extern 	cr3_ready
extern  p_proc_current
extern	p_proc_next			;added by xw, 18/4/26
extern	kstate_reenter_cntr
extern	kstate_on_init

bits 32

[SECTION .data]
clock_int_msg		db	"^", 0

[SECTION .bss]
StackSpace		resb	4 * 1024
StackTop:		; used only as irq-stack in minios. added by xw

; added by xw, 18/6/15
KernelStackSpace	resb	4 * 1024
KernelStackTop:	; used as stack of kernel itself
; ~xw

[section .text]	; 代码在此

global _start	; 导出 _start

;global restart
global restart_initial	;Added by xw, 18/4/21
global restart_restore	;Added by xw, 18/4/21
;global save_context
global sched			;Added by xw, 18/4/21
global sys_call
; global read_cr2   ;//add by visual 2016.5.9
; global read_cr3   ;//add by visual 2016.5.9

global refresh_page_cache ; // add by visual 2016.5.12
global refresh_gdt 		;add by sundong 2023.3.8

global	divide_error
global	single_step_exception
global	nmi
global	breakpoint_exception
global	overflow
global	bounds_check
global	inval_opcode
global	copr_not_available
global	double_fault
global	copr_seg_overrun
global	inval_tss
global	segment_not_present
global	stack_exception
global	general_protection
global	page_fault
global	copr_error
global	hwint00
global	hwint01
global	hwint02
global	hwint03
global	hwint04
global	hwint05
global	hwint06
global	hwint07
global	hwint08
global	hwint09
global	hwint10
global	hwint11
global	hwint12
global	hwint13
global	hwint14
global	hwint15

    global _start
_start:
    mov	    esp, KernelStackTop

    push    edx                 ; multiboot info phy addr from bootloader
    call	cstart

    lgdt	[gdt_ptr]
    lidt	[idt_ptr]

    mov     ax, SELECTOR_TSS
    ltr     ax

    jmp	    SELECTOR_KERNEL_CS:csinit
csinit:
    mov     ax, SELECTOR_FLAT_RW
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     ss, ax
    mov     ax, SELECTOR_VIDEO
    mov     gs, ax

    call    kernel_main
    ; NOTE: force a dead halt here although kernel_main is assumed to be noreturn
.1:
    hlt
    jmp     .1

refresh_page_cache:
    push eax
    mov eax,cr3
    mov cr3,eax
    pop eax
    ret

refresh_gdt:
    lgdt	[gdt_ptr]

    mov		ax, SELECTOR_FLAT_RW
    mov		ds, ax
    mov		es, ax
    mov		fs, ax
    mov		ss, ax

    mov 	ax,	SELECTOR_VIDEO
    mov 	gs,	ax

    ret

; 中断和异常 -- 硬件中断
; ---------------------------------
%macro	hwint_master	1
    ;call save
    push	0xFFFFFFFF			;no err code added by lcy 2023.10.26
    call save_int			;save registers and some other things. modified by xw, 17/12/11
    inc  dword [kstate_reenter_cntr]  ;If kstate_reenter_cntr isn't equal to 0, there is no switching to the irq-stack,
                            ;which is performed in save_int. Added by xw, 18/4/21

    in	al, INT_M_CTLMASK	; `.
    or	al, (1 << %1)		;  | 屏蔽当前中断
    out	INT_M_CTLMASK, al	; /

    mov	al, EOI				; `. 置EOI位
    out	INT_M_CTL, al		; /

    sti						; CPU在响应中断的过程中会自动关中断，这句之后就允许响应新的中断
    push %1						; `.
    call [irq_table + 4 * %1]	;  | 中断处理程序
    pop	ecx						; /

;	push eax								; 	┓				add by visual 2016.4.5
;	mov eax,[cr3_ready]						; 	┣改变cr3
;	mov cr3,eax								;	┃
;	pop eax									; 	┛

    cli
    dec dword [kstate_reenter_cntr]


    in	al, INT_M_CTLMASK	; `.
    and	al, ~(1 << %1)		;  | 恢复接受当前中断
    out	INT_M_CTLMASK, al	; /

    cmp	    dword [kstate_reenter_cntr], 0			;Added by xw, 18/4/19
    jnz		restore
;	ret
;restart_int:
;	mov		eax, [p_proc_current]
;	mov 	esp, [eax + ESP_SAVE_INT]		;switch back to the kernel stack from the irq-stack
    pop     esp
    cmp	    dword [kstate_on_init], 0		;added by xw, 18/6/10
    jnz		restore
    call	sched							;save current process's context, invoke schedule(), and then
                                            ;switch to the chosen process's kernel stack and restore it's context
                                            ;added by xw, 18/4/19
;	call	renew_env
    jmp     restart_restore
%endmacro


ALIGN	16
hwint00:		; Interrupt routine for irq 0 (the clock).
    hwint_master	0

ALIGN	16
hwint01:		; Interrupt routine for irq 1 (keyboard)
    hwint_master	1

ALIGN	16
hwint02:		; Interrupt routine for irq 2 (cascade!)
    hwint_master	2

ALIGN	16
hwint03:		; Interrupt routine for irq 3 (second serial)
    hwint_master	3

ALIGN	16
hwint04:		; Interrupt routine for irq 4 (first serial)
    hwint_master	4

ALIGN	16
hwint05:		; Interrupt routine for irq 5 (XT winchester)
    hwint_master	5

ALIGN	16
hwint06:		; Interrupt routine for irq 6 (floppy)
    hwint_master	6

ALIGN	16
hwint07:		; Interrupt routine for irq 7 (printer)
    hwint_master	7

; ---------------------------------
%macro	hwint_slave	1
;primary edition, commented by xw
;	push	%1
;	call	spurious_irq
;	add	esp, 4
;	hlt
;~xw

    ;added by xw, 18/5/29
    push	0xFFFFFFFF			;no err code added by lcy 2023.10.26
    call save_int			;save registers and some other things.
    inc  dword [kstate_reenter_cntr]  ;If kstate_reenter_cntr isn't equal to 0, there is no switching to the irq-stack,
                            ;which is performed in save_int. Added by xw, 18/4/21

    in	al, INT_S_CTLMASK	; `.
    or	al, (1 << (%1 - 8))		;  | 屏蔽当前中断
    out	INT_S_CTLMASK, al	; /
    mov	al, EOI				; `.
    out	INT_M_CTL, al		; / 置EOI位(master)
    nop						; `.一定注意：slave和master都要置EOI
    out	INT_S_CTL, al		; / 置EOI位(slave)

    sti						; CPU在响应中断的过程中会自动关中断，这句之后就允许响应新的中断
    push %1						; `.
    call [irq_table + 4 * %1]	;  | 中断处理程序
    pop	ecx						; /

    cli
    dec dword [kstate_reenter_cntr]
    in	al, INT_S_CTLMASK		; `.
    and	al, ~(1 << (%1 - 8))	;  | 恢复接受当前中断
    out	INT_S_CTLMASK, al		; /

    cmp	    dword [kstate_reenter_cntr], 0			;Added by xw, 18/4/19
    jnz		restore
;	ret
;restart_int:
;	mov		eax, [p_proc_current]
;	mov 	esp, [eax + ESP_SAVE_INT]		;switch back to the kernel stack from the irq-stack
    pop     esp
    cmp	    dword [kstate_on_init], 0		;added by xw, 18/6/10
    jnz		restore
    call	sched							;save current process's context, invoke schedule(), and then
                                            ;switch to the chosen process's kernel stack and restore it's context
                                            ;added by xw, 18/4/19
    jmp     restart_restore
    ;~xw
%endmacro
; ---------------------------------

ALIGN	16
hwint08:		; Interrupt routine for irq 8 (realtime clock).
    hwint_slave	8

ALIGN	16
hwint09:		; Interrupt routine for irq 9 (irq 2 redirected)
    hwint_slave	9

ALIGN	16
hwint10:		; Interrupt routine for irq 10
    hwint_slave	10

ALIGN	16
hwint11:		; Interrupt routine for irq 11
    hwint_slave	11

ALIGN	16
hwint12:		; Interrupt routine for irq 12
    hwint_slave	12

ALIGN	16
hwint13:		; Interrupt routine for irq 13 (FPU exception)
    hwint_slave	13

ALIGN	16
hwint14:		; Interrupt routine for irq 14 (AT winchester)
    hwint_slave	14

ALIGN	16
hwint15:		; Interrupt routine for irq 15
    hwint_slave	15

%macro	exception_no_errcode	2
    push	0xFFFFFFFF			;no err code
    call	save_exception		;save registers and some other things.
    mov		esi, esp			;esp points to pushed address of restart_exception at present
    add		esi, 4 * 17			;we use esi to help to fetch arguments of exception handler from the stack.
                                ;17 is calculated by: 4+8+retaddr+errcode+eip+cs+eflag=17
    mov		eax, [esi]			;saved eflags
    push	eax
    mov		eax, [esi - 4]		;saved cs
    push	eax
    mov		eax, [esi - 4 * 2]	;saved eip
    push	eax
    mov		eax, [esi - 4 * 3]	;saved err code
    push	eax
    push	%1					;vector_no
    sti
    call	%2
    cli
    add		esp, 4 * 5			;clear arguments of exception handler in stack
    ret							;returned to 'restart_exception' procedure
%endmacro

%macro	exception_errcode	2
    call	save_exception		;save registers and some other things.
    mov		esi, esp			;esp points to pushed address of restart_exception at present
    add		esi, 4 * 17			;we use esi to help to fetch arguments of exception handler from the stack.
                                ;17 is calculated by: 4+8+retaddr+errcode+eip+cs+eflag=17
    mov		eax, [esi]			;saved eflags
    push	eax
    mov		eax, [esi - 4]		;saved cs
    push	eax
    mov		eax, [esi - 4 * 2]	;saved eip
    push	eax
    mov		eax, [esi - 4 * 3]	;saved err code
    push	eax
    push	%1					;vector_no
    sti
    call	%2
    cli
    add		esp, 4 * 5			;clear arguments of exception handler in stack
    ret							;returned to 'restart_exception' procedure
%endmacro

divide_error:					; vector_no	= 0
    exception_no_errcode	0, exception_handler

single_step_exception:			; vector_no	= 1
    exception_no_errcode	1, exception_handler

nmi:							; vector_no	= 2
    exception_no_errcode	2, exception_handler

breakpoint_exception:			; vector_no	= 3
    exception_no_errcode	3, exception_handler

overflow:						; vector_no	= 4
    exception_no_errcode	4, exception_handler

bounds_check:					; vector_no	= 5
    exception_no_errcode	5, exception_handler

inval_opcode:					; vector_no	= 6
    exception_no_errcode	6, exception_handler

copr_not_available:				; vector_no	= 7
    exception_no_errcode	7, exception_handler

double_fault:					; vector_no	= 8
    exception_errcode	8, exception_handler

copr_seg_overrun:				; vector_no	= 9
    exception_no_errcode	9, exception_handler

inval_tss:						; vector_no	= 10
    exception_errcode	10, exception_handler

segment_not_present:			; vector_no	= 11
    exception_errcode	11, exception_handler

stack_exception:				; vector_no	= 12
    exception_errcode	12, exception_handler

general_protection:				; vector_no	= 13
    exception_errcode	13, general_protection_handler

page_fault:						; vector_no	= 14
    exception_errcode	14, page_fault_handler

copr_error:						; vector_no	= 16
    exception_no_errcode	16, exception_handler

save_exception:
    pushad          ; `.
    push    ds      ;  |
    push    es      ;  | 保存原寄存器值
    push    fs      ;  |
    push    gs      ; /
    mov     dx, ss
    mov     ds, dx
    mov     es, dx
    mov		fs, dx							;value of fs and gs in user process is different to that in kernel
    mov		dx, SELECTOR_VIDEO - 2			;added by xw, 18/6/20
    mov		gs, dx

    mov     esi, esp
    push    restart_exception
    jmp     [esi + RETADR - P_STACKBASE]

restart_exception:
    call    sched
    pop		gs
    pop		fs
    pop		es
    pop		ds
    popad
    add		esp, 4 * 2	; clear retaddr and error code in stack
    iretd

; ====================================================================================
;                                   save
; ====================================================================================
;modified by xw, 17/12/11
;modified begin
;save:
;        pushad          ; `.
;        push    ds      ;  |
;        push    es      ;  | 保存原寄存器值
;        push    fs      ;  |
;        push    gs      ; /
;        mov     dx, ss
;        mov     ds, dx
;        mov     es, dx
;
;        mov     esi, esp                    ;esi = 进程表起始地址
;
;        inc     dword [kstate_reenter_cntr]           ;kstate_reenter_cntr++;
;        cmp     dword [kstate_reenter_cntr], 0        ;if(kstate_reenter_cntr ==0)
;        jne     .1                          ;{
;        mov     esp, StackTop               ;  mov esp, StackTop <--切换到内核栈
;        push    restart                     ;  push restart
;        jmp     [esi + RETADR - P_STACKBASE];  return;
;.1:                                         ;} else { 已经在内核栈，不需要再切换
;        push    restart_reenter             ;  push restart_reenter
;        jmp     [esi + RETADR - P_STACKBASE];  return;
;                                            ;}
save_int:
        pushad          ; `.
        push    ds      ;  |
        push    es      ;  | 保存原寄存器值
        push    fs      ;  |
        push    gs      ; /

        cmp	    dword [kstate_reenter_cntr], 0			;Added by xw, 18/4/19
        jnz		instack

;		mov		ebx,  [p_proc_current]			;xw
;		mov		dword [ebx + ESP_SAVE_INT], esp	;xw save esp position in the kernel-stack of the process
;		or		dword [ebx + SAVE_TYPE], 1		;set 1st-bit of save_type, added by xw, 17/12/04
        mov     dx, ss
        mov     ds, dx
        mov     es, dx
        mov		fs, dx							;value of fs and gs in user process is different to that in kernel
        mov		dx, SELECTOR_VIDEO - 2			;added by xw, 18/6/20
        mov		gs, dx

        mov     esi, esp
        mov     esp, StackTop   ;switches to the irq-stack from current process's kernel stack
;		push    restart_int		;added by xw, 18/4/19    deleted by lcy , 2023.10.28
        push    esi
        jmp     [esi + RETADR - P_STACKBASE]
instack:						;already in the irq-stack
;	 	push    restart_restore	;modified by xw, 18/4/19
;		jmp		[esp + RETADR - P_STACKBASE]
        jmp		[esp + RETADR - P_STACKBASE]	;modified by xw, 18/6/4


save_syscall:			;can't modify EAX, for it contains syscall number
                        ;can't modify EBX, for it contains the syscall argument
        pushad          ; `.
        push    ds      ;  |
        push    es      ;  | 保存原寄存器值
        push    fs      ;  |
        push    gs      ; /
        mov		edx,  [p_proc_current]				;xw
        mov		dword [edx + ESP_SAVE_INT], esp	;xw save esp position in the kernel-stack of the process
;		mov		dword [edx + ESP_SAVE_SYSCALL_ARG], esp	;added by zhenhao 2023.3.5
;		or		dword [edx + SAVE_TYPE], 4			;set 3rd-bit of save_type, added by xw, 17/12/04
        mov     dx, ss
        mov     ds, dx
        mov     es, dx
        mov		fs, dx							;value of fs and gs in user process is different to that in kernel
        mov		dx, SELECTOR_VIDEO - 2			;added by xw, 18/6/20
        mov		gs, dx

        mov     esi, esp
;       inc     dword [kstate_reenter_cntr]
;	    push    restart_syscall		;modified by xw, 17/12/04   deleted by lcy 2023.10.28
;		push	judge
        jmp     [esi + RETADR - P_STACKBASE]
;modified end

; ====================================================================================
;                                sched(process switch)
; ====================================================================================
sched:
;could be called by C function, you must save ebp, ebx, edi, esi,
;for C function assumes that they stay unchanged. added by xw, 18/4/19

;save_context
        pushfd
        pushad			;modified by xw, 18/6/4
        cli
;		push	ebp
;		push    ebx
;		push    edi
;		push    esi
        mov		ebx,  [p_proc_current]
        mov		dword [ebx + ESP_SAVE_CONTEXT], esp	;save esp position in the kernel-stack of the process
;schedule
        call	schedule			;schedule is a C function, save eax, ecx, edx if you want them to stay unchanged.
;prepare to run new process
        mov		ebx,  [p_proc_next]	;added by xw, 18/4/26
        mov		dword [p_proc_current], ebx
        call	renew_env			;renew process executing environment
;restore_context
        mov		ebx, [p_proc_current]
        mov 	esp, [ebx + ESP_SAVE_CONTEXT]		;switch to a new kernel stack
;		pop		esi
;		pop		edi
;		pop		ebx
;		pop		ebp
;		sti			;popfd will be executed below, so sti is not needed. modified by xw, 18/12/27
        popad
        popfd
        ret
; ====================================================================================
;                        			renew_env
; ====================================================================================
;renew process executing environment. Added by xw, 18/4/19
renew_env:
    mov     eax, [cr3_ready]
    mov     cr3, eax                      ; switch pdbr
    mov     eax, [p_proc_current]
    lldt    [eax + P_LDT_SEL]             ; switch ldt
    lea     ebx, [eax + INIT_STACK_SIZE]
    mov     dword [tss + TSS3_S_SP0], ebx ; switch esp0
    ret

; ====================================================================================
;                                 judge
; ====================================================================================
;added by xw, 18/03/24
;added begin
;judge:
;		mov		eax, [p_proc_current]			;get save_type
;		mov		ebx, [eax + SAVE_TYPE]
;		test	ebx, 1
;		jnz		restart_int
;		test	ebx, 2
;		jnz		restart_context
;		test	ebx, 4
;		jnz		restart_syscall
;added end

; ====================================================================================
;                                 sys_call
; ====================================================================================
sys_call:
;get syscall number from eax
;syscall that's called gets its argument from pushed ebx
;so we can't modify eax and ebx in save_syscall
    push	0xFFFFFFFF			;no err code added by lcy 2023.10.26
    call	save_syscall	;save registers and some other things. modified by xw, 17/12/11
    sti
    ; push 	ebx							;push the argument the syscall need
    call    [syscall_table + eax * 4]	;将参数压入堆栈后再调用函数			add by visual 2016.4.6
    ; add		esp, 4						;clear the argument in the stack, modified by xw, 17/12/11
    cli
    mov		edx, [p_proc_current]
    mov 	esi, [edx + ESP_SAVE_INT]
    mov     [esi + EAXREG - P_STACKBASE], eax	;the return value of C function is in EAX
    ;ret
;restart_syscall:
;	sub 	ebx, 4							;ebx gets its value from judge
;	mov		dword [eax + SAVE_TYPE], ebx	;clear 3rd-bit of save_type
    mov		eax, [p_proc_current]
    mov 	esp, [eax + ESP_SAVE_INT]	;xw	restore esp position
    call	sched							;added by xw, 18/4/26
    jmp 	restart_restore
; ====================================================================================
;				    restart
; ====================================================================================
;modified by xw
;xw 	restart:
;xw		mov	esp, [p_proc_ready]
;xw		lldt	[esp + P_LDT_SEL]
;xw		lea	eax, [esp + P_STACKTOP]
;xw		mov	dword [tss + TSS3_S_SP0], eax

; restart_int:
; 	mov		eax, [p_proc_current]
; 	mov 	esp, [eax + ESP_SAVE_INT]		;switch back to the kernel stack from the irq-stack
; 	cmp	    dword [kstate_on_init], 0		;added by xw, 18/6/10
; 	jnz		restart_restore
; 	call	sched							;save current process's context, invoke schedule(), and then
; 											;switch to the chosen process's kernel stack and restore it's context
; 											;added by xw, 18/4/19
; ;	call	renew_env
; 	jmp     restart_restore

;restart_syscall:                           deleted by lcy 2023.10.28
;	sub 	ebx, 4							;ebx gets its value from judge
;	mov		dword [eax + SAVE_TYPE], ebx	;clear 3rd-bit of save_type
;	mov		eax, [p_proc_current]
;	mov 	esp, [eax + ESP_SAVE_SYSCALL]	;xw	restore esp position
;	call	sched							;added by xw, 18/4/26
;	jmp 	restart_restore

;xw	restart_reenter:
restart_restore:
;	dec		dword [kstate_reenter_cntr]
    call 	process_signal
restore:
    pop		gs
    pop		fs
    pop		es
    pop		ds
    popad
    add		esp, 4 * 2
    iretd

;restart_initial:
;	mov		esp, [p_proc_current]
;	lldt	[esp + P_LDT_SEL]
;	lea		eax, [esp + INIT_STACK_SIZE]
;	mov		dword [tss + TSS3_S_SP0], eax
;	mov 	esp, [esp + ESP_SAVE_INT]		;restore esp position
;	jmp 	restart_restore

;to launch the first process in the os. added by xw, 18/4/19
restart_initial:
;	mov		eax, [p_proc_current]
;	lldt	[eax + P_LDT_SEL]
;	lea		ebx, [eax + INIT_STACK_SIZE]
;	mov		dword [tss + TSS3_S_SP0], ebx
    call	renew_env						;renew process executing environment
    mov		ebx, [p_proc_current]
    mov 	esp, [ebx + ESP_SAVE_CONTEXT]		;switch to a new kernel stack
    popad
    popfd
    ret

    ;mov		eax, [p_proc_current]
    ;mov 	esp, [eax + ESP_SAVE_INT]		;restore esp position

    ;jmp 	restart_restore
