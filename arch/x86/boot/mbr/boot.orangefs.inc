; ATTENTION: compatiblility with current mbr is not guaranteed
; TODO: check compatiblility

;适用于orangefs的boot
BaseOfStack			equ		0x07c00		; Boot状态下堆栈基地址
STACK_ADDR  		equ  	0x7bea 		; 堆栈栈顶

;BaseOfBoot 				equ	1000h 		; added by mingxuan 2020-9-12
OffsetOfBoot				equ	7c00h		; load Boot sector to BaseOfBoot:OffsetOfBoot
OffsetOfActiPartStartSec	equ 7e00h		; 活动分区的起始扇区号相对于BaseOfBoot的偏移量	;added by mingxuan 2020-9-12
											; 该变量来自分区表，保存在该内存地址，用于在os_boot和loader中查找FAT32文件
OffsetOfStartSecInPET		equ 0x08		; 分区表表项中用于存储物理起始扇区变量的相对于分区表项的偏移量

DATA_BUF_OFF 		equ  	0x2000			;临时缓冲区地址
SB_DATA_OFF			equ		0x3000			;Superblock的加载位置
INODE_DATA_OFF		equ		0x3200			;inode区的加载位置

SECTOR_SIZE			equ		0x200			;扇区大小512

OSLOADER_SEG 		equ  	0x09000 	; 起始段地址
OSLOADER_SEG_OFF	equ		0x0400

; 扩展磁盘服务所使用的地址包
DAP_SECTOR_HIGH  	equ  	4  			; 起始扇区号的高32位 ( 每次调用需要重置 ) DWORD
DAP_SECTOR_LOW  	equ  	8  			; 起始扇区号的低32位 ( 每次调用需要重置 ) DWORD

DAP_BUFFER_SEG  	equ  	10  		; 缓冲区段地址   ( 每次调用需要重置 ) WORD
DAP_BUFFER_OFF  	equ  	12  		; 缓冲区偏移   ( 每次调用需要重置 ) WORD

DAP_RESERVED2  		equ  	13  		; 保留字节

DAP_READ_SECTORS 	equ  	14  		; 要处理的扇区数(1 - 127 )
DAP_RESERVED1  		equ  	15  		; 保留字节
DAP_PACKET_SIZE  	equ  	16  		; 包的大小为16字节

;orangefs super_block
SB_MAGIC            equ     0;		    /**< MAGIC NUMBER */
SB_NR_INODES        equ     4;	        /**< HOW MANY INODES */
SB_NR_SECTS         equ     8;	        /**< HOW MANY SECTORS */
SB_NR_IMAP_SECTS    equ     12;	        /**< HOW MANY INODE-MAP SECTORS */
SB_NR_SMAP_SECTS    equ     16;	        /**< HOW MANY SECTOR-MAP SECTORS */
SB_N_1ST_SECT       equ     20;	        /**< NUMBER OF THE 1ST DATA SECTOR */
SB_NR_INODE_SECTS   equ     24;         /**< HOW MANY INODE SECTORS */
SB_ROOT_INODE       equ     28;         /**< INODE NR OF ROOT DIRECTORY */
SB_INODE_SIZE       equ     32;         /**< INODE_SIZE */
SB_INODE_ISIZE_OFF  equ     36;         /**< OFFSET OF `STRUCT INODE::I_SIZE' */
SB_INODE_START_OFF  equ     40;         /**< OFFSET OF `STRUCT INODE::I_START_SECT' */
SB_DIR_ENT_SIZE     equ     44;         /**< DIR_ENTRY_SIZE */
SB_DIR_ENT_INODE_OFF    equ     48;     /**< OFFSET OF `STRUCT DIR_ENTRY::INODE_NR' */
SB_DIR_ENT_FNAME_OFF    equ     52;     /**< OFFSET OF `STRUCT DIR_ENTRY::NAME' */
;orangefs inode
INODE_I_MODE        equ     0;		    /**< ACCSESS MODE */
INODE_I_SIZE        equ     4;		    /**< FILE SIZE */
INODE_I_START_SECT  equ     8;	        /**< THE FIRST SECTOR OF THE DATA */
INODE_I_NR_SECTS    equ     12;	        /**< HOW MANY SECTORS THE FILE OCCUPIES */
;orangefs dentry
DENTRY_INODE_NR     equ     0;		    /**< INODE NR. */
DENTRY_NAME         equ     4;	        /**< FILENAME */

MAX_FILENAME_LEN    equ     12;

    org		(07c00h)  ;ORANGFS 0号扇区的整个扇区都是启动扇区
										;modified by sundong 2023.4.7

	jmp	START		;deleted by mingxuan 2020-9-15
	;nop				;deleted by mingxuan 2020-9-15

	FAT_START_SECTOR 	DD 	0 ;FAT表的起始扇区号 ;added by mingxuan 2020-9-17
	DATA_START_SECTOR 	DD  0 ;数据区起始扇区号 ;added by mingxuan 2020-9-17
	DATA_BUFF_SEG		DW	0 ;存储数据缓冲区的段地址;added by sundong 2023.3.16
	DRIVE_NUM			DW  0x80;存储器驱动号 硬盘为0x80
	LoaderName     db "loader.bin"       ; 第二阶段启动程序 FDOSLDR.BIN
START:
	cld
	mov    	ax, cs
	mov		ds, ax
	mov		es, ax ;deleted by mingxuan 2020-9-13
	mov		ss, ax
	mov		fs,	ax

		; 清屏
;	mov		ax, 0600h		; AH = 6,  AL = 0h
;	mov		bx, 0700h		; 黑底白字(BL = 07h)
;	mov		cx, 0			; 左上角: (0, 0)
;	mov		dx, 0184fh		; 右下角: (80, 50)
;	int		10h				; int 10h
;	pusha
;	mov		dx, 0184fh		; 右下角: (80, 50)
;	int		10h				; int 10h
;	popa
	mov	word[DATA_BUFF_SEG],ax ;数据缓冲区的段地址与DS一致 add by sundong 2023.3.16
	cmp 	si,0
	jz	.Init_DAP
	;grub 链式引导，将当前分区表表项的起始地址放在DS：SI处
	;si加上一个起始扇区在表项中的偏移量就是用于存储起始扇区的那段内存
	mov	word[DRIVE_NUM],dx
	mov		eax, [ds:si+OffsetOfStartSecInPET]
	mov		[OffsetOfActiPartStartSec],eax	;将起始扇区的物理扇区号放入OffsetOfActiPartStartSec地址处 loader中会用到该值
.Init_DAP:
	;mov 	eax,18432
	mov		eax,[OffsetOfActiPartStartSec]
	mov		sp, STACK_ADDR
	mov  	bp, BaseOfStack
	mov 	dword [bp - DAP_SECTOR_HIGH ],	00h
	;mov 	byte  [bp - DAP_RESERVED1   ], 	00h ;deleted by mingxuan 2020-9-17
	;mov 	byte  [bp - DAP_RESERVED2   ], 	00h ;deleted by mingxuan 2020-9-17
	mov 	byte  [bp - DAP_PACKET_SIZE ], 	10h
	mov		byte  [bp - DAP_READ_SECTORS],  01h
	mov		ax,	[DATA_BUFF_SEG]
	mov		word  [bp - DAP_BUFFER_SEG  ],	ax
;找loader.bin过程：
;	1.读入superblock的扇区
;	2.读入所有的inode
;	3.根据根目录的inode找到根目录的数据（根目录下所有的dentry）所在的扇区
;	4.遍历根目录的所有dentry，找到name与loadername相同的dentry 然后找到dentry对应的inode
;	5.找到该inode然后，根据inode记录的起始扇区以及文件大小，把loader.bin加载进内存
.Search_Loader:
	call 	Read_SB
	call 	Read_Inode_Area
	mov 	bp,SB_DATA_OFF
	mov 	eax,[bp+SB_ROOT_INODE]
	dec	 	eax
	mov 	ecx,[bp+SB_INODE_SIZE]
	mul 	ecx
	mov 	si,ax
	lea 	bp,[INODE_DATA_OFF+si]
	;eax 存放根节点的起始扇区
	mov 	eax,[bp+INODE_I_START_SECT]
	lea     eax,[eax*8]
	add		eax,[OffsetOfActiPartStartSec]
	;ecx 根节点占用的扇区数目，也是外循环的此时
	mov		ecx,[bp+INODE_I_NR_SECTS]
	lea 	ecx,[ecx*8]
	mov		dx,	DATA_BUF_OFF
	mov		si, LoaderName
;这个循环负责遍历根目录所有的扇区
.L1:
	mov		bp,BaseOfStack
	mov	dword[bp-DAP_SECTOR_LOW], eax
	mov	word[bp-DAP_BUFFER_OFF],dx
	call 	ReadSector
	;开始比较每个目录项的name
	pusha
	mov 	bp,SB_DATA_OFF
	;每个目录项的大小放到ebx
	mov 	ebx,[bp+SB_DIR_ENT_SIZE]
	mov 	eax,SECTOR_SIZE
	xor		edx,edx
	div		ebx
	mov 	ecx,eax
	mov 	bp,DATA_BUF_OFF
	mov 	si,LoaderName
	lea 	di,[bp+DENTRY_NAME]
;这个循环完成遍历sector内的所有dentry
.L11:
	push 	cx
	push    si
	push	di
	mov		cx, 10
	repe	cmpsb;把si指向的数据与di指向的数据按byte比较，相同时继续比较 不同时就停止，比较一次cx-1
	pop 	di
	pop		si
	jcxz	_FOUND_LOADER
	pop 	cx
	add 	di,bx 	;di指向下一个dentry的name
	loop 	.L11
	popa
	inc 	eax		;扇区数+1
	loop 	.L1
	jmp _NO_LOADER



_FOUND_LOADER:
	sub 	di,DENTRY_NAME
	mov 	eax,[di] ;inode id 存在eax中
	dec		eax
	mov 	bp,SB_DATA_OFF
	mov 	ebx,[bp+SB_INODE_SIZE];ebx存储inode size
	mul 	ebx
	;si表示该inode在inode区内部的偏移量
	mov 	si,ax
	mov 	bp,INODE_DATA_OFF
	;bp指向inode
	add 	bp,si
	mov 	eax,[bp+INODE_I_SIZE]
	mov 	ebx,SECTOR_SIZE
	div 	ebx
	mov 	ecx,eax
	inc 	ecx ;上取整，cx存储的是实际占用的扇区数
	mov 	eax,[bp+INODE_I_START_SECT]
	lea     eax,[eax*8]
	add 	eax,[OffsetOfActiPartStartSec]
	mov 	bp,BaseOfStack
	;loader 加载的段地址
	mov		bx,	OSLOADER_SEG
	mov		word [bp - DAP_BUFFER_SEG],bx
	mov		bx,OSLOADER_SEG_OFF
	mov		word [bp - DAP_BUFFER_OFF], OSLOADER_SEG_OFF
	mov		dword[bp - DAP_SECTOR_LOW], eax
.L2:
	call	ReadSector
	add		bx,SECTOR_SIZE
	mov		word [bp - DAP_BUFFER_OFF], bx
	inc		eax
	mov		dword[bp - DAP_SECTOR_LOW], eax
	loop .L2
	;跳转到loader
	jmp  OSLOADER_SEG : OSLOADER_SEG_OFF


;读superblock所在的扇区
Read_SB:
	pusha
	mov		eax,[OffsetOfActiPartStartSec]
	add		eax,8
	mov		dword[bp-DAP_SECTOR_LOW], eax
	mov		bx,	SB_DATA_OFF
	mov		word[bp-DAP_BUFFER_OFF], bx
	call ReadSector
	popa
	ret
;将inode区读入内存中
Read_Inode_Area:
	pusha
	mov 	bp,SB_DATA_OFF
	mov		eax,[OffsetOfActiPartStartSec]
	add		eax,16
	mov		ebx,[bp+SB_NR_IMAP_SECTS]
	lea		ebx,[ebx*8]
	add		eax,ebx
	mov		ebx,[bp+SB_NR_SMAP_SECTS]
	lea		ebx,[ebx*8]
	add		ebx,eax
;	mov		ecx,[bp+SB_NR_INODE_SECTS]
	mov		ecx,1;根目录对应着1号inode，存放在inode区域的第一个扇区中，因此只读1个扇区
	mov		dx,	INODE_DATA_OFF
	mov 	bp,	BaseOfStack
;ebx 存储要读的扇区的绝对扇区号，dx存储内存中的位置
;循环读，inode区占用的sector可能不止一个
.L3:
	mov		dword[bp-DAP_SECTOR_LOW], ebx
	mov		word[bp-DAP_BUFFER_OFF],dx
	call ReadSector
	inc		ebx
	add		dx,SECTOR_SIZE
	loop	.L3

	popa
	ret

;读扇区函数，
ReadSector:
	pusha
	mov		bp,BaseOfStack
	mov		ah, 42h						;ah是功能号，扩展读对应0x42
	lea 	si, [bp - DAP_PACKET_SIZE]  ;使用扩展int13时DAP结构体首地址传给si
	mov		dl, [DRIVE_NUM]
	;mov	dl, 0x80

	;jmp 	$

	int 	13h

	jc 		_DISK_ERROR

	;jmp	$
	popa
	ret
_NO_LOADER:
_DISK_ERROR:      	 ; 显示磁盘错误信息
	;for test, added by mingxuan 2020-9-4

	; 清屏
	; for test, added by mingxuan 2020-9-15
	pusha
	mov		dx, 0184fh		; 右下角: (80, 50)
	int		10h				; int 10h
	popa
	JMP  	$

times 	510-($-$$)	db	0	; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw 	0xaa55				; 结束标志
