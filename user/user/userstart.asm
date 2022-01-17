;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 	userstart.asm  //added by mingxuan 2021-3-29
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
