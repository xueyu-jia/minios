%ifndef OFF_CONSTS_INC
%define OFF_CONSTS_INC

OFF_STACKBASE  equ 0
OFF_GS         equ OFF_STACKBASE
OFF_FS         equ OFF_GS + 4
OFF_ES         equ OFF_FS + 4
OFF_DS         equ OFF_ES + 4
OFF_EDI        equ OFF_DS + 4
OFF_ESI        equ OFF_EDI + 4
OFF_EBP        equ OFF_ESI + 4
OFF_KERNEL_ESP equ OFF_EBP + 4
OFF_EBX        equ OFF_KERNEL_ESP + 4
OFF_EDX        equ OFF_EBX + 4
OFF_ECX        equ OFF_EDX + 4
OFF_EAX        equ OFF_ECX + 4
OFF_RETADDR    equ OFF_EAX + 4
OFF_ERROR      equ OFF_RETADDR + 4
OFF_EIP        equ OFF_ERROR + 4
OFF_CS         equ OFF_EIP + 4
OFF_EFLAGS     equ OFF_CS + 4
OFF_ESP        equ OFF_EFLAGS + 4
OFF_SS         equ OFF_ESP + 4
OFF_STACKTOP   equ OFF_SS + 4

OFF_LDT_SEL          equ 0
OFF_LDT              equ OFF_LDT_SEL + 4
OFF_ESP_SAVE_INT     equ OFF_LDT + 16
OFF_ESP_SAVE_CONTEXT equ OFF_ESP_SAVE_INT + 4

%endif
