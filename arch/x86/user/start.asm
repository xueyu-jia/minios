
extern 	main
extern 	exit
extern  minios_hook_pre_start
extern  minios_hook_post_start

[bits 32]

[section .text]

    global _start
_start:
    call    minios_hook_pre_start
    call    main
    push    eax
    call    minios_hook_post_start
    call    exit
.1:
    hlt
    jmp     .1
