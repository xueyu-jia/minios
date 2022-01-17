;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 	initstart.asm  //add by visual 2016.5.16
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


extern main

bits 32

[section .text]

global _start

_start:
	;push 	eax
	push 	edx
	push	ecx
	call	main
	
	hlt
