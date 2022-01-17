
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

;%include "sconst.inc"
%include "os/include/sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_get_pid       	equ 1 ;	//add by visual 2016.4.6

;删除了4个系统调用接口：kmalloc、kmalloc_4k、malloc和free, mingxuan 2021-3-25
;_NR_kmalloc       	equ 2 ;		//add by visual 2016.4.6	//deleted by mingxuan 2021-3-25
;_NR_kmalloc_4k     equ 3 ;		//add by visual 2016.4.7	//deleted by mingxuan 2021-3-25
;_NR_malloc      	equ 4 ;		//add by visual 2016.4.7	//deleted by mingxuan 2021-3-25
_NR_malloc_4k      	equ 2 ;		//add by visual 2016.4.7

;_NR_free      		equ 6 ;		//add by visual 2016.4.7	//deleted by mingxuan 2021-3-25
_NR_free_4k      	equ 3 ;		//add by visual 2016.4.7

_NR_fork     		equ 4 ;		//add by visual 2016.4.8
_NR_pthread_create     	equ 5 ;		//add by visual 2016.4.11
_NR_udisp_int     	equ 6 ;		//add by visual 2016.5.16
_NR_udisp_str     	equ 7 ;		//add by visual 2016.5.16
_NR_exec     		equ 8 ;		//add by visual 2016.5.16
_NR_yield			equ 9 ;		//added by xw, 17/12
_NR_sleep			equ 10 ;	//added by xw, 17/12
;_NR_print_E			equ 11 ;	//added by xw, 18/4/27	;deleted by mingxuan 2021-8-13
;_NR_print_F			equ 12 ;	//added by xw, 18/4/27	;deleted by mingxuan 2021-8-13

_NR_open			equ 11 ;	//added by xw, 18/6/18
_NR_close			equ 12 ;	//added by xw, 18/6/18
_NR_read			equ 13 ;	//added by xw, 18/6/18
_NR_write			equ 14 ;	//added by xw, 18/6/18
_NR_lseek			equ 15 ;	//added by xw, 18/6/18
_NR_unlink			equ 16 ;	//added by xw, 18/6/18

_NR_create			equ 17 ;    //added by mingxuan 2019-5-17
_NR_delete 			equ 18 ;    //added by mingxuan 2019-5-17
_NR_opendir 		equ 19 ;    //added by mingxuan 2019-5-17
_NR_createdir  		equ 20 ;    //added by mingxuan 2019-5-17
_NR_deletedir   	equ 21 ;    //added by mingxuan 2019-5-17
_NR_readdir         equ 22 ;    //added by pg999w 2021-1-10
_NR_chdir           equ 23 ;    //added by ran
_NR_getcwd          equ 24 ;    //added by ran

_NR_wait			equ 25;		//added by mingxuan 2021-1-6
_NR_exit			equ 26;		//added by mingxuan 2021-1-6

_NR_signal			equ 27;		//added by mingxuan 2021-2-28
_NR_sigsend			equ 28;		//added by mingxuan 2021-2-28
_NR_sigreturn		equ 29;		//added by mingxuan 2021-2-28
_NR_total_mem_size  equ 30;      //added by wang     2021-8-21

_NR_shmget          equ  31  	;added by xiaofeng 2021-9-8
_NR_shmat           equ  32		;added by xiaofeng 2021-9-8
_NR_shmdt           equ  33		;added by xiaofeng 2021-9-8
_NR_shmctl          equ  34  	;added by xiaofeng 2021-9-8
_NR_shmmemcpy       equ  35  	;added by xiaofeng 2021-9-8

_NR_ftok			equ 36 ;		//added by yingchi	2021.12.20
_NR_msgget			equ 37 ;		//added by yingchi	2021.12.20
_NR_msgsnd			equ 38 ;		//added by yingchi	2021.12.20
_NR_msgrcv			equ 39 ;		//added by yingchi	2021.12.20
_NR_msgctl			equ 40 ;		//added by yingchi	2021.12.20

_NR_test			equ 41 ;

_NR_execvp			equ 42 ;	//added by xyx&&wjh 2021.12.31
_NR_execv			equ 43 ;	//added by xyx&&wjh 2021.12.31

INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks
global	get_pid		;		//add by visual 2016.4.6

;global	kmalloc		;		//add by visual 2016.4.6	//deleted by mingxuan 2021-3-25
;global	kmalloc_4k	;		//add by visual 2016.4.7	//deleted by mingxuan 2021-3-25
;global	malloc		;		//add by visual 2016.4.7	//deleted by mingxuan 2021-3-25
global	malloc_4k	;		//add by visual 2016.4.7
;global	free		;		//add by visual 2016.4.7	//deleted by mingxuan 2021-3-25
global	free_4k		;		//add by visual 2016.4.7

global	fork		;		//add by visual 2016.4.8
global	pthread_create		;		//add by visual 2016.4.11
global	udisp_int	;		//add by visual 2016.5.16
global	udisp_str	;		//add by visual 2016.5.16
global	exec		;		//add by visual 2016.5.16
global	execvp		;		//added by xyx&&wjh 2021.12.31
global	execv		;		//added by xyx&&wjh 2021.12.31
global  yield		;		//added by xw
global  sleep		;		//added by xw
global	print_E		;		//added by xw
global	print_F		;		//added by xw

global	open		;		//added by xw, 18/6/18
global	close		;		//added by xw, 18/6/18
global	read		;		//added by xw, 18/6/18
global	write		;		//added by xw, 18/6/18
global	lseek		;		//added by xw, 18/6/18
global	unlink		;		//added by xw, 18/6/19

global	create		;		//added by mingxuan 2019-5-17
global	delete		;		//added by mingxuan 2019-5-17
global  opendir		;		//added by mingxuan 2019-5-17
global	createdir	;		//added by mingxuan 2019-5-17
global  deletedir	;		//added by mingxuan 2019-5-17
global  readdir     ;       //added by pg999w 2021-1-10
;实现进程独立的工作目录         //comment added by ran 
global  chdir       ;       //added by ran
global  getcwd      ;       //added by ran

global	wait_		;		//added by mingxuan 2021-1-6
global 	exit		;		//added by mingxuan 2021-1-6

global __signal		;		//added by mingxuan 2021-2-28
global sigsend		;		//added by mingxuan 2021-2-28
global sigreturn	;		//added by mingxuan 2021-2-28
global total_mem_size  ;    //added by wang 2021-8-26

global shmget ;added by Ding Leilei 2021-1-7
global _shmat ;added by Ding Leilei 2021-1-7
global _shmdt ;added by Ding Leilei 2021-1-7
global shmctl ;added by Ding Leilei 2021-1-7
global shmmemcpy

global	ftok		;		//added by yingchi	2021.12.20
global	msgget		;		//added by yingchi	2021.12.20
global	msgsnd		;		//added by yingchi	2021.12.20
global	msgrcv		;		//added by yingchi	2021.12.20
global	msgctl		;		//added by yingchi	2021.12.20

global test

bits 32
[section .text]
; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================
;                              get_pid		//add by visual 2016.4.6
; ====================================================================
get_pid:
	mov	eax, _NR_get_pid
	int	INT_VECTOR_SYS_CALL
	ret
	
; ====================================================================
;                              kmalloc		//add by visual 2016.4.6
; ====================================================================
;deleted by mingxuan 2021-3-25
;kmalloc:
;	mov ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!
;	mov	eax, _NR_kmalloc
;	int	INT_VECTOR_SYS_CALL
;	ret
	
; ====================================================================
;                              kmalloc_4k		//add by visual 2016.4.7
; ====================================================================
;deleted by mingxuan 2021-3-25
;kmalloc_4k:
;	mov ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!111
;	mov	eax, _NR_kmalloc_4k
;	int	INT_VECTOR_SYS_CALL
;	ret
	
; ====================================================================
;                              malloc		//add by visual 2016.4.7
; ====================================================================
;deleted by mingxuan 2021-3-8
;malloc:
;   push ebx
;	mov ebx,[esp+8] ; 将C函数调用时传来的参数放到ebx里!!111
;	mov	eax, _NR_malloc
;	int	INT_VECTOR_SYS_CALL
;	pop ebx
;	ret
	
; ====================================================================
;                              malloc_4k		//add by visual 2016.4.7
; ====================================================================
malloc_4k:
	;mov ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!111	;deleted by mingxuan 2021-8-13
	mov	eax, _NR_malloc_4k
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================
;                              free		//add by visual 2016.4.7
; ====================================================================
;deleted by mingxuan 2021-3-8
;free:
;	mov ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!111
;	mov	eax, _NR_free
;	int	INT_VECTOR_SYS_CALL
;	ret

; ====================================================================
;                              free_4k		//add by visual 2016.4.7
; ====================================================================
;free_4k:
;	mov ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!111
;	mov	eax, _NR_free_4k
;	int	INT_VECTOR_SYS_CALL
;	ret
;modified by mingxuan 2021-8-14
free_4k:
	push 1			;the number of parameters
	push ebx		;protect ebx
	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_free_4k
	int	INT_VECTOR_SYS_CALL
	pop ebx			;restore ebx
	add esp, 4
	ret

; ====================================================================
;                              fork		//add by visual 2016.4.8
; ====================================================================
fork:
	;mov ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!说明:含有一个参数时,一定要这句,不含参数时,可以要这句,也可以不要这句,并不影响结果
	mov	eax, _NR_fork
	int	INT_VECTOR_SYS_CALL
	ret
	
; ====================================================================
;                              pthread		//add by visual 2016.4.11
; ====================================================================
;pthread:
;   push ebx
;	mov ebx,[esp+8] ; 将C函数调用时传来的参数放到ebx里!!说明:含有一个参数时,一定要这句,不含参数时,可以要这句,也可以不要这句,并不影响结果
;	mov	eax, _NR_pthread
;	int	INT_VECTOR_SYS_CALL
;	pop ebx
;	ret
;modified by mingxuan 2021-8-14
pthread_create:
	push 4			;the number of parameters
	push ebx		;protect ebx
	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_pthread_create
	int	INT_VECTOR_SYS_CALL
	pop ebx			;restore ebx
	add esp, 4
	ret

; ====================================================================
;                              udisp_int		//add by visual 2016.5.16
; ====================================================================	
;udisp_int:
;	mov ebx,[esp+4]
;	mov	eax, _NR_udisp_int
;	int	INT_VECTOR_SYS_CALL
;	ret
;modified by mingxuan 2021-8-13
udisp_int:
	push 1			;the number of parameters
	push ebx		;protect ebx
	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_udisp_int
	int	INT_VECTOR_SYS_CALL
	pop ebx			;restore ebx
	add esp, 4
	ret

; ====================================================================
;                              udisp_str		//add by visual 2016.5.16
; ====================================================================	
;udisp_str:
;    push ebx
;	mov ebx,[esp+8]
;	mov	eax, _NR_udisp_str
;	int	INT_VECTOR_SYS_CALL
;	pop ebx
;	ret
;modified by mingxuan 2021-8-13
udisp_str:
	push 1			;the number of parameters
	push ebx		;protect ebx
	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_udisp_str
	int	INT_VECTOR_SYS_CALL
	pop ebx			;restore ebx
	add esp, 4
	ret

; ====================================================================
;                              execvp	added by xyx&&wjh  2021-12-31
; ====================================================================	
execvp:
	push 2			;the number of parameters
	push ebx		;protect ebx
	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_execvp
	int	INT_VECTOR_SYS_CALL
	pop ebx			;restore ebx
	add esp, 4
	ret

; ====================================================================
;                              execv	
; ====================================================================	
execv:
	push 2			;the number of parameters
	push ebx		;protect ebx
	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_execv
	int	INT_VECTOR_SYS_CALL
	pop ebx			;restore ebx
	add esp, 4
	ret

;end added   

; ====================================================================
;                              exec		//add by visual 2016.5.16
; ====================================================================	
;exec:
;	mov ebx,[esp+4]
;	mov	eax, _NR_exec
;	int	INT_VECTOR_SYS_CALL
;	ret
;modified by mingxuan 2021-8-11
exec:
	push 3			;the number of parameters
	push ebx		;protect ebx
	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_exec
	int	INT_VECTOR_SYS_CALL
	pop ebx			;restore ebx
	add esp, 4
	ret

; ====================================================================
;                              yield	//added by xw
; ====================================================================	
;yield:
;	mov ebx,[esp+4]
;	mov	eax, _NR_yield
;	int	INT_VECTOR_SYS_CALL
;	ret

; modified by mingxuan 2021-8-13
; yield does not have param
yield:
	mov	eax, _NR_yield
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================
;                              sleep	//added by xw
; ====================================================================	
;sleep:
;	mov ebx,[esp+4]
;	mov	eax, _NR_sleep
;	int	INT_VECTOR_SYS_CALL
;	ret
; modified by mingxuan 2021-8-13
sleep:
	push 1			;the number of parameters
	push ebx		;protect ebx
	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_sleep
	int	INT_VECTOR_SYS_CALL
	pop ebx			;restore ebx
	add esp, 4
	ret

; ====================================================================
;                              print_E	//added by xw
; ====================================================================	
;deleted by mingxuan 2021-8-13
;print_E:
;	mov ebx,[esp+4]
;	mov	eax, _NR_print_E
;	int	INT_VECTOR_SYS_CALL
;	ret

; ====================================================================
;                              print_F	//added by xw
; ====================================================================	
;deleted by mingxuan 2021-8-13
;print_F:
;	mov ebx,[esp+4]
;	mov	eax, _NR_print_F
;	int	INT_VECTOR_SYS_CALL
;	ret

; ====================================================================
;                              open		//added by xw, 18/6/18
; ====================================================================	
; open has more than one parameter, to pass them, we save the esp to ebx, 
; and ebx will be passed into kernel as usual. In kernel, we use the saved
; esp in user space to get the number of parameters and the values of them.
open:
	push 2			;the number of parameters
	
	push ebx		;protect ebx
	
	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_open
	int	INT_VECTOR_SYS_CALL
	
	pop ebx			;restore ebx
	
	add esp, 4
	ret
	
; ====================================================================
;                              close	//added by xw, 18/6/18
; ====================================================================	
; close has only one parameter, but we can still use this method.
close:
	push 1			;the number of parameters
	mov ebx, esp
	mov	eax, _NR_close
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret

; ====================================================================
;                              read		//added by xw, 18/6/18
; ====================================================================
read:
	push 3			;the number of parameters
	mov ebx, esp
	mov	eax, _NR_read
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret

; ====================================================================
;                              write		//added by xw, 18/6/18
; ====================================================================
write:
	push 3			;the number of parameters
	push ebx

	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_write
	int	INT_VECTOR_SYS_CALL

	pop ebx
	add esp, 4
	ret

; ====================================================================
;                              lseek		//added by xw, 18/6/18
; ====================================================================
lseek:
	push 3			;the number of parameters
	mov ebx, esp
	mov	eax, _NR_lseek
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret
	
; ====================================================================
;                              unlink		//added by xw, 18/6/18
; ====================================================================
unlink:
	push 1			;the number of parameters
	mov ebx, esp
	mov	eax, _NR_unlink
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret

;和FAT32有关的系统调用
create:
	push 1
	mov ebx, esp
	mov eax, _NR_create
	int INT_VECTOR_SYS_CALL
	add esp, 4
	ret

delete:
	push 1
	mov ebx, esp
	mov eax, _NR_delete
	int INT_VECTOR_SYS_CALL
	add esp, 4
	ret

opendir:
	push 1
	mov ebx, esp
	mov eax, _NR_opendir
	int INT_VECTOR_SYS_CALL
	add esp, 4
	ret

createdir:
	push 1
	mov ebx, esp
	mov eax, _NR_createdir
	int INT_VECTOR_SYS_CALL
	add esp, 4
	ret

deletedir:
	push 1
	mov ebx, esp
	mov eax, _NR_deletedir
	int INT_VECTOR_SYS_CALL
	add esp, 4
	ret

readdir:
	push 3			;the number of parameters
	mov ebx, esp
	mov	eax, _NR_readdir
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret

chdir:
	push 1
	mov ebx, esp
	mov eax, _NR_chdir
	int INT_VECTOR_SYS_CALL
	add esp, 4
	ret

getcwd:
	push 2
	mov ebx, esp
	mov eax, _NR_getcwd
	int INT_VECTOR_SYS_CALL
	add esp, 4
	ret

; ====================================================================
;                              wait		added by mingxuan 2021-1-6
; ====================================================================
wait_:
	mov	eax, _NR_wait
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================
;                              exit		added by mingxuan 2021-1-6
; ====================================================================
;exit:
;	mov ebx,[esp+4]
;	mov	eax, _NR_exit
;	int	INT_VECTOR_SYS_CALL
;	ret
; modified by mingxuan 2021-8-13
exit:
	push 1			;the number of parameters
	push ebx		;protect ebx
	mov ebx, esp
	add ebx, 4
	mov	eax, _NR_exit
	int	INT_VECTOR_SYS_CALL
	pop ebx			;restore ebx
	add esp, 4
	ret



; ====================================================================
;                              total_mem_size		added by wang 2021-8-21
; ====================================================================

total_mem_size:
	mov	eax, _NR_total_mem_size
	int	INT_VECTOR_SYS_CALL
	ret


; ====================================================================
;                             signal		added by mingxuan 2021-2-28
; ====================================================================

;/**********************shm.c*****************/
;//add by dingleilei,2021.1.1
; ====================================================================
;                              shmget		added by Ding Leilei 2021-1-8
; ====================================================================
shmget:
	push 3			;the number of parameters
	mov ebx, esp
	mov	eax, _NR_shmget
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret
; ====================================================================
;                              shmat	added by Ding Leilei 2021-1-8
; ====================================================================
_shmat:
	push 3			;the number of parameters
	mov ebx, esp
	mov	eax, _NR_shmat
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret
; ====================================================================
;                              shmdt 		added by Ding Leilei 2021-1-8
; ====================================================================
_shmdt:
	push 1		;the number of parameters
	mov ebx, esp
	mov	eax, _NR_shmdt
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret
; ====================================================================
;                              shmctl		added by Ding Leilei 2021-1-8
; ====================================================================
shmctl:
	push 3			;the number of parameters
	mov ebx, esp
	mov	eax, _NR_shmctl
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret
; ====================================================================
;                              shmmemcpy	added by Ding Leilei 2021-1-8
; ====================================================================
shmmemcpy:
	push 3			;the number of parameters
	mov ebx, esp
	mov	eax, _NR_shmmemcpy
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret

; ====================================================================
;					ftok	added by yingchi yuanxiang 2021.12.22
; ====================================================================
ftok:
	push 2
	mov ebx, esp
	mov	eax, _NR_ftok
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret

; ====================================================================
;					msgget	added by yingchi yuanxiang 2021.12.22
; ====================================================================
msgget:
	push 2
	mov ebx, esp
	mov	eax, _NR_msgget
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret

; ====================================================================
;					msgsnd	added by yingchi yuanxiang 2021.12.22
; ====================================================================
msgsnd:
	push 4
	mov ebx, esp
	mov	eax, _NR_msgsnd
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret

; ====================================================================
;					msgrcv	added by yingchi yuanxiang 2021.12.22
; ====================================================================
msgrcv:
	push 5
	mov ebx, esp
	mov	eax, _NR_msgrcv
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret

; ====================================================================
;					msgctl	added by yingchi yuanxiang 2021.12.22
; ====================================================================
msgctl:
	push 3
	mov ebx, esp
	mov	eax, _NR_msgctl
	int	INT_VECTOR_SYS_CALL
	add esp, 4
	ret

; ====================================================================
;                   test			//added by cjjx, 2021-12-26
;用于测试内核信号量是否有效
; ====================================================================
test:
	mov ebx,[esp+4]
	mov eax,_NR_test
	int INT_VECTOR_SYS_CALL
	ret
	
%macro	SYS_CALL 1
	push ebx
	mov ebx, esp
	mov	eax, %1
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret 
%endmacro

__signal:
	SYS_CALL _NR_signal
	
sigsend :
	SYS_CALL _NR_sigsend

sigreturn :
	SYS_CALL _NR_sigreturn