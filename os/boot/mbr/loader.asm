
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               loader.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

BaseOfLoader              equ  09000h ; LOADER.BIN 被加载到的位置 ----  段地址
OffsetOfLoader            equ  400h ; LOADER.BIN 被加载到的位置 ---- 偏移地址
LoaderSegPhyAddr          equ  90000h ; LOADER.BIN 被加载到的位置 ---- 物理地址 (= Base * 10h)

BaseOfStack               equ OffsetOfLoader

[SECTION .text.s16]
ALIGN 16
[BITS 16]

global _start
_start:
    jmp    Main ; Start
; %include	"boot/mbr/include/fat32.inc"
; %include	"boot/mbr/include/loader.inc"
%include   "os/boot/mbr/include/pm.inc"

ALIGN  4
; GDT ------------------------------------------------------------------------------------------------------------------------------------------------------------
;                                           段基址                段界限                                       属性
LABEL_GDT:              Descriptor             0,                    0, 0                                       ; 空描述符
LABEL_DESC_FLAT_C:      Descriptor             0,              0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K            ; 0 ~ 4G
LABEL_DESC_FLAT_RW:     Descriptor             0,              0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K            ; 0 ~ 4G
LABEL_DESC_VIDEO:       Descriptor       0B8000h,               0ffffh, DA_DRW                       | DA_DPL3  ; 显存首地址

; 准备lgdt必要的数据结构 --------------------------------------------------------------------------------------------------------------------------------------------
GdtLen              equ $ - LABEL_GDT
GdtPtr              dw  GdtLen - 1                    ; gdt界限
                    dd  LoaderSegPhyAddr + LABEL_GDT  ; gdt基地址

; GDT 选择子 ----------------------------------------------------------------------------------
SelectorFlatC       equ LABEL_DESC_FLAT_C   - LABEL_GDT
SelectorFlatRW      equ LABEL_DESC_FLAT_RW  - LABEL_GDT
SelectorVideo       equ LABEL_DESC_VIDEO    - LABEL_GDT + SA_RPL3

Main:            ; <--- 从这里开始 *************
    mov    ax, cs
    mov    ds, ax
    mov    es, ax
    mov    ss, ax
    mov    sp, BaseOfStack

    ;mov    cx, 02000h
    ;mov    ah, 01h
    ;int    10h
;向int 15h询问内存信息
Getmem:
    pusha
    push edx
	mov	ebx, 0			; ebx = 后续值, 开始时需为 0
	mov	di, _MemChkBuf		; es:di 指向一个地址范围描述符结构（Address Range Descriptor Structure）
.MemChkLoop:
	mov	eax, 0E820h		; eax = 0000E820h
	mov	ecx, 20			; ecx = 地址范围描述符结构的大小
	mov	edx, 0534D4150h		; edx = 'SMAP'
	int	15h			; int 15h
	jc	.MemChkFail ;如果产生进位则进行跳转，进位代表没有成功
	add	di, 20
	inc	dword [_dwMCRNumber]	; dwMCRNumber = ARDS 的个数
	cmp	ebx, 0
	jne	.MemChkLoop
	jmp	.MemChkOK
.MemChkFail:
    pop edx
    popa
.MemChkOK:
    pop edx
    popa
; 下面准备跳入保护模式 -------------------------------------------

; 加载 GDTR
    lgdt   [GdtPtr]
	
; 关中断
    cli
	
; 打开地址线A20
    in     al, 92h
    or     al, 00000010b
    out    92h, al
	
; 准备切换到保护模式
    mov    eax, cr0
    or     eax, 1
    mov    cr0, eax
; 真正进入保护模式
    jmp    dword SelectorFlatC:LABEL_PM_START
    ;; 变量
_dwMCRNumber:		dd	0	; Memory Check Result
_dwMemSize:			dd	0
_MemChkBuf:	times	1024	db	0
	
; 从此以后的代码在保护模式下执行 ----------------------------------------------------
; 32 位代码段. 由实模式跳入 ---------------------------------------------------------
[SECTION .text]
ALIGN    32
[BITS    32]

;extern load_kernel
;extern paging
extern loader_cstart
LABEL_PM_START:
    mov    ax, SelectorVideo
    mov    gs, ax
    mov    ax, SelectorFlatRW
    mov    ds, ax
    mov    es, ax
    mov    fs, ax
    mov    ss, ax
    mov    esp, TopOfStack
    
    push dword [dwMCRNumber]
    push dword MemChkBuf

    call loader_cstart
;	call	paging			
;    add	esp,4
;    add	esp,4
;    call  load_kernel

    ;***************************************************************
    ; 内存看上去是这样的：
    ;              ┃                                    ┃
    ;              ┃                 .                  ┃
    ;              ┃                 .                  ┃
    ;              ┃                 .                  ┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;              ┃■■■■■■Page  Tables■■■■■■┃
    ;              ┃■■■■■(大小由LOADER决定)■■■■┃
    ;    00101000h ┃■■■■■■■■■■■■■■■■■■┃ PageTblBase
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;    00100000h ┃■■■■Page Directory Table■■■■┃ PageDirBase  <- 1M
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃
    ;       F0000h ┃□□□□□□□System ROM□□□□□□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃
    ;       E0000h ┃□□□□Expansion of system ROM □□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃
    ;       C0000h ┃□□□Reserved for ROM expansion□□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃ B8000h ← gs
    ;       A0000h ┃□□□Display adapter reserved□□□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃
    ;       9FC00h ┃□□extended BIOS data area (EBDA)□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;       90000h ┃■■■■■■■LOADER.BIN■■■■■■┃ somewhere in LOADER ← esp
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;       80000h ┃■■■■■■■KERNEL.BIN■■■■■■┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;       30000h ┃■■■■■■■■KERNEL■■■■■■■┃ 30400h ← KERNEL 入口 (KernelEntryPointPhyAddr)
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃                                    ┃
    ;        7E00h ┃              F  R  E  E            ┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;        7C00h ┃■■■■■■BOOT  SECTOR■■■■■■┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃                                    ┃
    ;         500h ┃              F  R  E  E            ┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃
    ;         400h ┃□□□□ROM BIOS parameter area □□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇┃
    ;           0h ┃◇◇◇◇◇◇Int  Vectors◇◇◇◇◇◇┃
    ;              ┗━━━━━━━━━━━━━━━━━━┛ ← cs, ds, es, fs, ss
    ;
    ;
    ;        ┏━━━┓        ┏━━━┓
    ;        ┃■■■┃ 我们使用     ┃□□□┃ 不能使用的内存
    ;        ┗━━━┛        ┗━━━┛
    ;        ┏━━━┓        ┏━━━┓
    ;        ┃      ┃ 未使用空间    ┃◇◇◇┃ 可以覆盖的内存
    ;        ┗━━━┛        ┗━━━┛
    ;
    ; 注：KERNEL 的位置实际上是很灵活的，可以通过同时改变 LOAD.INC 中的 KernelEntryPointPhyAddr 和 MAKEFILE 中参数 -Ttext 的值来改变。
    ;     比如，如果把 KernelEntryPointPhyAddr 和 -Ttext 的值都改为 0x400400，则 KERNEL 就会被加载到内存 0x400000(4M) 处，入口在 0x400400。
    ;


 ;.data 之开始 ---------------------------------------------------------------------------------------------
[SECTION .data]
ALIGN    32
[BITS    32]

dwMemSize		equ	LoaderSegPhyAddr + _dwMemSize
dwMCRNumber		equ	LoaderSegPhyAddr + _dwMCRNumber
MemChkBuf		equ	LoaderSegPhyAddr + _MemChkBuf


; 堆栈就在数据段的末尾
StackSpace:    times    1000h    db    0
TopOfStack     equ      $    ; 栈顶
