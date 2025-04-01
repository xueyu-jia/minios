; MEMORY LAYOUT
; ===
; 0xc0600000               kernel start linear address
; 0x00600000               kernel start physical address
; ---
; 0x00200000~0x00600000    page table
; 0x001ff000~0x00200000    page directory
; 0x00100000~0x001ff000    heap for loader
; ---
; 0x000f0000~              [STABLE] system rom
; 0x000e0000~0x000f0000    [STABLE] expansion of system rom
; 0x000c0000~0x000e0000    [STABLE] reserved for rom expansion
; 0x000b8000               base of video memory for gs
; 0x000a0000~0x000c0000    [STABLE] reserved for display adapter
; 0x0009fc00~0x000a0000    [STABLE] extended bios data area
; 0x0009fc00~0x00100000    [STABLE] reserved for hardware
; ---
; 0x00090400~              loader file
;           ~0x00090400    stack for loader in real mode
; ---
; 0x00007e00~              buffer used to read sectors
; 0x00007c00~0x00007e00    [STABLE] boot sector
; 0x00007c00~0x00007e00    [STABLE] mbr, moved to other place before booting
; 0x00007c00~0x00007c5a    fat32 bpb
;           ~0x00007c00    stack for boot
; ---
; 0x00000800~0x00000a00    a copy of mbr
; ---
; 0x00000400~0x00000500    [STABLE] rom bios parameter block
; 0x00000000~0x00000400    [STABLE] interrupt vectors
; ===

%include "layout.inc"
%include "fat32.inc"
%include "packet.inc"
%include "protect.inc"
%include "paging.inc"
%include "msg_helper.inc"

[SECTION .text.s16]
[BITS 16]

align 16

    global _start
_start:
    jmp     Main

; variables
DriverNumber    dd 0            ; current driver number
PartitionLBA    dd 0            ; current partition start LBA
TotalARDS       dd 0            ; total ards found
ARDSBuffer      times 1024 db 0 ; buffer for reading ards
ARDSBufferLimit equ $

; declare messages and introduce ShowMessage
MSG_BEGIN 32
    NewMessage FailedToQueryMemory, "failed to get memory size"
MSG_END

align 4

; layout of our gdt specified in loader:
;
; ┏━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
; ┃ Descriptors         ┃ Selectors                 ┃
; ┣━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
; ┃ RESERVED            ┃                           ┃
; ┣━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
; ┃ DESC_FLAT_C 0~4G    ┃ 0x08 = cs                 ┃ <- 0x00000000
; ┣━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
; ┃ DESC_FLAT_RW 0~4G   ┃ 0x10 = ds, es, gs, fs, ss ┃ <- 0x00000000
; ┗━━━━━━━━━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━━━━━━━┛

; NOTE: entry 0 should always be null and subsequent entries should be used instead
LABEL_GDT:          times 8 db 0
LABEL_DESC_FLAT_C:  Descriptor 0x00000000, 0xfffff, DA_DPL0 | DA_CR  | DA_32 | DA_LIMIT_4K ; 0 ~ 4G
LABEL_DESC_FLAT_RW: Descriptor 0x00000000, 0xfffff, DA_DPL0 | DA_DRW | DA_32 | DA_LIMIT_4K ; 0 ~ 4G

GdtLen equ $ - LABEL_GDT - 1

GdtPtr: dw GdtLen                    ; limit
        dd PhyLoaderBase + LABEL_GDT ; base addr

; selector = [13 bits index] << 3 | [1 bits TI tag] << 2 | [2 bits RPL tag] << 0
SelectorFlatC  equ (LABEL_DESC_FLAT_C  - LABEL_GDT) | SA_TIG | SA_RPL0
SelectorFlatRW equ (LABEL_DESC_FLAT_RW - LABEL_GDT) | SA_TIG | SA_RPL0

align 16

Main:
    mov     ax, cs
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    mov     sp, OffsetOfLoader

    ; save boot info from boot
    xor     eax, eax
    mov     al, dl
    mov     [DriverNumber], eax
    mov     [PartitionLBA], ebx

    ; clear screen and hide cursor for a better view of messages
    mov     ax, 0x0003
    int     10h
    mov     ah, 0x01
    mov     ch, 0b00100000
    int     10h

    ; detect and store all ards to get total memory size later
    call    GetAllARDS

    ; load gdt
    lgdt    [GdtPtr]

    ; turn off interrupts
    cli

    ; enable A20 gate
    in      al, 0x92
    or      al, 0b00000010
    out     0x92, al

    ; set PE flag in cr0 and prepare to jump into protect mode
    mov     eax, cr0
    or      eax, 1
    mov     cr0, eax

    ; use a long jump to switch to protect mode, after the jump, the processor
    ; will set cs to 0x8, i.e. selector LABEL_DESC_FLAT_C
    jmp     dword SelectorFlatC:ProtectMain

GetAllARDS:
    pusha
    xor     ebx, ebx
    mov     di, ARDSBuffer
.loop:
    mov     eax, 0xe820
    mov     ecx, 20             ; size of ards, actually always 20 no matter what is set
    mov     edx, 0x534d4150     ; 'SMAP'
    int     15h
    jc      .fail
    inc     dword [TotalARDS]
    add     di, 20
    cmp     di, ARDSBufferLimit - 20
    jae     .done               ; buffer exceeded, break the detection
    test    ebx, ebx
    jnz     .loop
    jmp     .done
.fail:
    mov     dh, MSG_FailedToQueryMemory
    call    ShowMessage
.1:
    hlt
    jmp     .1
.done:
    popa
    ret

[SECTION .text]
[BITS 32]

align 32

extern cstart

; physical address of variables used in real mode
PhyDriverNumber equ PhyLoaderBase + DriverNumber
PhyPartitionLBA equ PhyLoaderBase + PartitionLBA
PhyTotalARDS    equ PhyLoaderBase + TotalARDS
PhyARDSBuffer   equ PhyLoaderBase + ARDSBuffer

ProtectMain:
    ; reset seg regs
    mov     ax, SelectorFlatRW
    mov     ds, ax
    mov     es, ax
    mov     gs, ax
    mov     fs, ax
    mov     ss, ax

    mov     esp, StackTop

    push    dword [PhyTotalARDS]
    push    dword PhyARDSBuffer
    push    dword [PhyPartitionLBA]
    push    dword [PhyDriverNumber]
    call    cstart

[SECTION .data]
[BITS 32]

align 32

; allocate 4KB stack
times 4096 db 0
StackTop:
