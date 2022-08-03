#########################
# Makefile for Orange'S #
#########################

# Entry point of Orange'S
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
#############edit by visual 2016.5.10####
#ENTRYPOINT	= 0xC0030400
ENTRYPOINT  = 0xC0120400	#modified by mingxuan 2021-8-7

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
ASM		= nasm
DASM	= ndisasm#########################
# Makefile for Orange'S #
#########################

# Entry point of Orange'S
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
#############edit by visual 2016.5.10####
#ENTRYPOINT	= 0xC0030400
ENTRYPOINT  = 0xC0120400	#modified by mingxuan 2021-8-7

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
ASM		= nasm
DASM	= ndisasm
CC		= gcc
LD		= ld
AR		= ar

ASMBFLAGS	= -I os/boot/floppy/include/	#added by mingxuan 2019-5-17
ASMBFLAGS_mbr   = -I os/boot/mbr/include/	#added by mingxuan 2019-5-17
ASMKFLAGS	= -I include/ -f elf
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)
ARFLAGS		= rcs

CFLAGS		= -I include/ -m32 -c -fno-builtin -fno-stack-protector -Wall -Wextra -g -std=gnu99

LDFLAGS_kernel	= -m elf_i386 -s -Ttext $(ENTRYPOINT) #modified by mingxuan 2021-3-29
#LDFLAGS_init	= -m elf_i386 -s -T../user/init/lds

LDFLAGS_kernel_gdb	= -m elf_i386 -Ttext $(ENTRYPOINT)
#LDFLAGS_init_gdb	= -m elf_i386

# This Program
ORANGESBOOT	= os/boot/floppy/boot.bin os/boot/floppy/loader.bin \
			  os/boot/mbr/mbr.bin os/boot/mbr/boot.bin os/boot/mbr/loader.bin #modified by mingxuan 2019-5-17

ORANGESKERNEL	= kernel.bin

OBJS		= os/kernel/kernel.o os/kernel/start.o os/kernel/main.o os/kernel/clock.o\
			os/kernel/i8259.o os/kernel/global.o os/kernel/protect.o os/kernel/proc.o\
			os/kernel/memman.o os/kernel/pagetbl.o	\
			os/kernel/elf.o os/kernel/file.o os/kernel/exec.o os/kernel/fork.o os/kernel/pthread.o \
			os/kernel/ktest.o os/kernel/testfunc.o os/kernel/fs.o os/kernel/hd.o \
			os/kernel/spinlock.o os/kernel/fat32.o os/kernel/base.o os/kernel/assist.o os/kernel/vfs.o \
			os/kernel/keyboard.o os/kernel/tty.o os/kernel/shell.o os/kernel/console.o \
			os/kernel/wait.o os/kernel/exit.o \
			os/kernel/signal.o \
			os/kernel/buddy.o os/kernel/kmalloc.o \
			os/kernel/syscallc.o \
			os/kernel/pthread_mutex.o os/kernel/pthread_cond.o \
			os/ipc/shm.o os/ipc/msg.o \
			os/klib/klib.a os/kernel/semaphore.o os/kernel/ahci.o\
			
#added by mingxuan 2021-2-28

OBJSKLIB 	= os/kernel/syscall.o os/klib/klib.o os/klib/kliba.o os/klib/stringc.o os/klib/stringasm.o os/klib/string.o

GDBBIN = kernel.gdb.bin

.PHONY : everything final image clean_os realclean_os disasm all buildimg #modified by mingxuan 2019-5-19


#everything : $(ORANGESBOOT) $(ORANGESKERNEL) $(ORANGESINIT) $(GDBBIN) # modified by mingxuan 2021-2-7
everything : $(ORANGESBOOT) $(ORANGESKERNEL) $(GDBBIN) # modified by mingxuan 2021-4-6

all : realclean_os everything

#image : realclean_os everything # modified by mingxuan 2020-12-8
image : realclean_os everything clean_os # modified by mingxuan 2021-8-20

#clean_os :
#	rm -f $(OBJS) $(OBJSINIT) $(OBJSKLIB)

clean_os :
	rm -f $(OBJS) $(OBJSKLIB)

realclean_os :
	rm -f $(OBJS) $(ORANGESBOOT) $(ORANGESKERNEL) $(OBJSKLIB) $(GDBBIN) # modified by mingxuan 2021-2-7

disasm :
	$(DASM) $(DASMFLAGS) $(ORANGESKERNEL) > $(DASMOUTPUT)

# added by mingxuan 2019-5-17
os/boot/floppy/boot.bin : os/boot/floppy/boot.asm os/boot/floppy/include/load.inc os/boot/floppy/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

# added by mingxuan 2019-5-17
os/boot/floppy/loader.bin : os/boot/floppy/loader.asm os/boot/floppy/include/load.inc os/boot/floppy/include/fat12hdr.inc os/boot/floppy/include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

# added by mingxuan 2019-5-17
os/boot/mbr/mbr.bin : os/boot/mbr/mbr.asm
	$(ASM) $(ASMBFLAGS_mbr) -o $@ $<

# added by mingxuan 2019-5-17
os/boot/mbr/boot.bin : os/boot/mbr/boot.asm
	$(ASM) $(ASMBFLAGS_mbr) -o $@ $<

# added by mingxuan 2019-5-17
os/boot/mbr/loader.bin : os/boot/mbr/loader.asm os/boot/mbr/include/loader.inc os/boot/mbr/include/fat32.inc os/boot/mbr/include/pm.inc
	$(ASM) $(ASMBFLAGS_mbr) -o $@ $<

$(ORANGESKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS_kernel) -o $(ORANGESKERNEL) $(OBJS)

os/kernel/kernel.o : os/kernel/kernel.asm os/include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<


kernel.gdb.bin : $(OBJS)
	$(LD) $(LDFLAGS_kernel_gdb) -o $@ $(OBJS)

#user/init/init.gdb.bin : $(OBJSINIT)
#	$(LD) $(LDFLAGS_init_gdb) -o $@ $(OBJSINIT)



os/kernel/syscall.o : os/kernel/syscall.asm os/include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

os/kernel/start.o: os/kernel/start.c os/include/type.h os/include/const.h os/include/protect.h os/include/string.h os/include/proc.h os/include/proto.h \
			os/include/global.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/main.o: os/kernel/main.c os/include/type.h os/include/const.h os/include/protect.h os/include/string.h os/include/proc.h os/include/proto.h \
			os/include/global.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/clock.o: os/kernel/clock.c
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/i8259.o: os/kernel/i8259.c os/include/type.h os/include/const.h os/include/protect.h os/include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/global.o: os/kernel/global.c os/include/type.h os/include/const.h os/include/protect.h os/include/proc.h \
			os/include/global.h os/include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/protect.o: os/kernel/protect.c os/include/type.h os/include/const.h os/include/protect.h os/include/proc.h os/include/proto.h \
			os/include/global.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/proc.o: os/kernel/proc.c
	$(CC) $(CFLAGS) -o $@ $<





os/klib/klib.o: os/klib/klib.c os/include/type.h os/include/const.h os/include/protect.h os/include/string.h os/include/proc.h os/include/proto.h \
			os/include/global.h
	$(CC) $(CFLAGS) -o $@ $<

os/klib/kliba.o : os/klib/kliba.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

# stringasm.o	added by mingxuan 2019-5-19
os/klib/stringasm.o: os/klib/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

# stringc.o	added by mingxuan 2019-5-19
os/klib/stringc.o:	os/klib/string.c
	$(CC) $(CFLAGS) -o $@ $<

# string.o	added by mingxuan 2019-5-19
os/klib/string.o : os/klib/stringasm.o os/klib/stringc.o
	$(LD) -m elf_i386 -r -o $@ $<

os/kernel/syscallc.o: os/kernel/syscallc.c os/include/type.h os/include/const.h os/include/protect.h os/include/proto.h \
			os/include/string.h os/include/proc.h os/include/global.h  os/include/semaphore.h
	$(CC) $(CFLAGS) -o $@ $<

# printf.o	added by mingxuan 2019-5-19
os/klib/printf.o: os/klib/printf.c
	$(CC) $(CFLAGS) -o $@ $<

# vsprintf.o	added by mingxuan 2019-5-19
os/klib/vsprintf.o: os/klib/vsprintf.c
	$(CC) $(CFLAGS) -o $@ $<

# scanf.o	added by mingxuan 2019-5-19
os/klib/scanf.o: os/klib/scanf.c
	$(CC) $(CFLAGS) -o $@ $<

os/klib/malloc.o: os/klib/malloc.c os/include/malloc.h os/include/type.h
	$(CC) $(CFLAGS) -o $@ $<



# os/kernel/memman.o: os/kernel/memman.c os/include/memman.h os/include/type.h os/include/const.h os/include/protect.h \
 			os/include/proto.h os/include/string.h os/include/proc.h os/include/global.h
os/kernel/memman.o: os/kernel/memman.c os/include/type.h os/include/const.h os/include/protect.h \
 			os/include/proto.h os/include/string.h os/include/proc.h os/include/global.h	# modified by mingxuan 2021-8-14
	$(CC) $(CFLAGS) -o $@ $<


os/kernel/buddy.o: os/kernel/buddy.c os/include/buddy.h os/include/type.h os/include/string.h
#                        os/include/memman.h	# deleted by mingxuan 2021-8-14
	$(CC) $(CFLAGS) -o $@ $<


os/kernel/kmalloc.o: os/kernel/kmalloc.c os/include/type.h os/include/string.h \
                        os/include/kmalloc.h os/include/buddy.h
	$(CC) $(CFLAGS) -o $@ $<



os/kernel/pagetbl.o: os/kernel/pagetbl.c os/include/type.h os/include/const.h os/include/protect.h os/include/proto.h os/include/string.h \
			os/include/proc.h os/include/global.h
	$(CC) $(CFLAGS) -o $@ $<

os/klib/klib.a:  $(OBJSKLIB)
	$(AR) $(ARFLAGS) -o $@  $(OBJSKLIB)





os/kernel/elf.o: os/kernel/elf.c os/include/type.h os/include/const.h os/include/protect.h \
			os/include/proto.h os/include/string.h os/include/proc.h os/include/global.h os/include/elf.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/file.o: os/kernel/file.c os/include/type.h os/include/const.h os/include/protect.h \
			os/include/proto.h os/include/string.h os/include/proc.h os/include/global.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/exec.o: os/kernel/exec.c os/include/type.h os/include/const.h os/include/protect.h \
			os/include/proto.h os/include/string.h os/include/proc.h os/include/global.h os/include/elf.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/fork.o: os/kernel/fork.c os/include/type.h os/include/const.h os/include/protect.h \
			os/include/proto.h os/include/string.h os/include/proc.h os/include/global.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/pthread.o: os/kernel/pthread.c os/include/type.h os/include/const.h os/include/protect.h \
			os/include/proto.h os/include/string.h os/include/proc.h os/include/global.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/ktest.o: os/kernel/ktest.c os/include/type.h os/include/const.h os/include/protect.h os/include/string.h os/include/proc.h \
			os/include/proto.h os/include/global.h
#	$(CC) $(CFLAGS) -o $@ $<	#modified by mingxuan 2019-5-19
#	$(CC) $(CFLAGS) \
		-D INSTALL_FILENAME='"$(INSTALL_FILENAME)"' \
		-D $(INSTALL_TYPE) \
		-o $@ $<
# modified by mingxuan 2021-4-2
	$(CC) $(CFLAGS) \
		-D INSTALL_FILENAME='"$(INSTALL_FILENAME)"' \
		-o $@ $<


os/kernel/testfunc.o: os/kernel/testfunc.c os/include/type.h os/include/const.h os/include/protect.h os/include/string.h os/include/proc.h \
			os/include/proto.h os/include/global.h
	$(CC) $(CFLAGS) -o $@ $<

os/kernel/hd.o: os/kernel/hd.c os/include/type.h os/include/const.h os/include/protect.h os/include/string.h os/include/proc.h \
			os/include/proto.h os/include/global.h os/include/fs_const.h os/include/fs.h os/include/hd.h os/include/fs_misc.h
	$(CC) $(CFLAGS) -o $@ $<

# fs.o and hd.o; added by xw, 18/5/25
os/kernel/fs.o: os/kernel/fs.c os/include/type.h os/include/const.h os/include/protect.h os/include/string.h os/include/proc.h \
			os/include/proto.h os/include/global.h os/include/fs_const.h os/include/fs.h os/include/hd.h os/include/fs_misc.h
#	$(CC) $(CFLAGS) -o $@ $<	#modified by mingxuan 2019-5-19
#	$(CC) $(CFLAGS) \
		-D INSTALL_FILENAME='"$(INSTALL_FILENAME)"' \
		-D INSTALL_NR_SECTORS=$(INSTALL_NR_SECTORS) \
		-D INSTALL_START_SECTOR=$(INSTALL_START_SECTOR) \
		-D $(INSTALL_TYPE) \
		-o $@ $<
# modified by mingxuan 2021-4-2
	$(CC) $(CFLAGS) \
		-D INSTALL_FILENAME='"$(INSTALL_FILENAME)"' \
		-D INSTALL_NR_SECTORS=$(INSTALL_NR_SECTORS) \
		-D INSTALL_START_SECTOR=$(INSTALL_START_SECTOR) \
		-o $@ $<



# spinlock.o	added by mingxuan 2019-1-5
os/kernel/spinlock.o: os/kernel/spinlock.c os/include/spinlock.h
	$(CC) -masm=intel $(CFLAGS) -o $@ $<

# pthread_mutex.o	#added by ZengHao & MaLinhan 2021-12-23
os/kernel/pthread_mutex.o: os/kernel/pthread_mutex.c os/include/spinlock.h
	$(CC)  $(CFLAGS) -o $@ $<
# pthread_cond.o	#added by ZengHao & MaLinhan 2021-12-23
os/kernel/pthread_cond.o: os/kernel/pthread_cond.c os/include/spinlock.h
	$(CC)  $(CFLAGS) -o $@ $<

# fat32.o	added by mingxuan 2019-5-17
os/kernel/fat32.o: os/kernel/fat32.c os/include/type.h os/include/const.h os/include/protect.h os/include/string.h os/include/proc.h \
			os/include/proto.h os/include/global.h os/include/fs_const.h os/include/fs.h os/include/hd.h os/include/fs_misc.h \
			os/include/fat32.h
	$(CC) $(CFLAGS) -o $@ $<

# base.o	added by mingxuan 2019-5-17
os/kernel/base.o: os/kernel/base.c os/include/type.h os/include/const.h os/include/protect.h os/include/string.h os/include/proc.h \
			os/include/proto.h os/include/global.h os/include/fs_const.h os/include/fs.h os/include/hd.h os/include/fs_misc.h \
			os/include/fat32.h
	$(CC) $(CFLAGS) -o $@ $<

# assist.o	added by mingxuan 2019-5-17
os/kernel/assist.o: os/kernel/assist.c os/include/type.h os/include/const.h os/include/protect.h os/include/string.h os/include/proc.h \
			os/include/proto.h os/include/global.h os/include/fs_const.h os/include/fs.h os/include/hd.h os/include/fs_misc.h \
			os/include/fat32.h
	$(CC) $(CFLAGS) -o $@ $<

# vfs.o	added by mingxuan 2019-5-17
os/kernel/vfs.o: os/kernel/vfs.c
	$(CC) $(CFLAGS) -o $@ $<


# keyboard.o	added by mingxuan 2019-5-19
os/kernel/kerboard.o: os/kernel/keyboard.c
	$(CC) $(CFLAGS) -o $@ $<

# console.o	added by mingxuan 2019-5-19
os/kernel/console.o: os/kernel/console.c
	$(CC) $(CFLAGS) -o $@ $<

# tty.o	added by mingxuan 2019-5-19
os/kernel/tty.o: os/kernel/tty.c
	$(CC) $(CFLAGS) -o $@ $<

# shell.o	added by mingxuan 2019-5-19
os/kernel/shell.o: os/kernel/shell.c
	$(CC) $(CFLAGS) -o $@ $<


# wait.o	added by mingxuan 2021-1-6
os/kernel/wait.o: os/kernel/wait.c os/include/type.h os/include/const.h os/include/protect.h \
			os/include/proto.h os/include/string.h os/include/proc.h os/include/global.h os/include/fs_misc.h os/include/spinlock.h
	$(CC) $(CFLAGS) -o $@ $<   #added by Jane 2020/12/09

# exit.o	added by mingxuan 2021-1-6
os/kernel/exit.o: os/kernel/exit.c os/include/type.h os/include/const.h os/include/protect.h \
			os/include/proto.h os/include/string.h os/include/proc.h os/include/global.h os/include/fs_misc.h os/include/spinlock.h
	$(CC) $(CFLAGS) -o $@ $<   #added by Jane 2020/12/09

# signal.o	added by mingxuan 2021-2-28
os/kernel/signal.o: os/kernel/signal.c
	$(CC) $(CFLAGS) -o $@ $<

os/ipc/shm.o: os/ipc/shm.c os/include/shm.h os/include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

os/ipc/msg.o: os/ipc/msg.c os/include/msg.h os/include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

# semaphore.o	added by mingxuan 2019-1-5
os/kernel/semaphore.o: os/kernel/semaphore.c os/include/semaphore.h os/include/spinlock.h
	$(CC) -masm=intel $(CFLAGS) -o $@ $<

os/kernel/ahci.o: os/kernel/ahci.c os/include/ahci.h
	$(CC) $(CFLAGS) -o $@ $<