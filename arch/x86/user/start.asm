;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 	userstart.asm  //added by mingxuan 2021-3-29
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


extern 	main
extern 	exit
bits 32

[section .text]

global _start

_start:
	call 	main
	push	eax
	call    exit
	hlt
