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
DASM	= ndisasm
CC		= gcc
LD		= ld
AR		= ar

ASMBFLAGS	= -I os/boot/floppy/include/	#added by mingxuan 2019-5-17
ASMBFLAGS_mbr   = -I os/boot/mbr/include/	#added by mingxuan 2019-5-17
ASMKFLAGS	= -I os/include/ -f elf
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)
ARFLAGS		= rcs
DEF_FLAGS 	= -DROOT_FSTYPE=\"$(ROOT_PART_FS_TYPE)\" -DROOT_PART=$(ROOT_FS_PART_NUM) # 指定根文件系统的分区和类型
CFLAGS		= -m32 -mno-sse -mno-mmx -c -fno-builtin -fno-stack-protector -Wall -Wextra -g -std=gnu99
KFLAGS		= $(CFLAGS) $(DEF_FLAGS)

#modified by jiangfeng 2024-03-10 add definitions option for root

LDFLAGS_kernel	= -m elf_i386 -s -Ttext $(ENTRYPOINT) #modified by mingxuan 2021-3-29
#LDFLAGS_init	= -m elf_i386 -s -T../user/init/lds

LDFLAGS_kernel_gdb	= -m elf_i386 -Ttext $(ENTRYPOINT)
#LDFLAGS_init_gdb	= -m elf_i386

# This Program
ORANGESBOOT	= os/boot/floppy/boot.bin os/boot/floppy/loader.bin \
			  os/boot/mbr/mbr.bin os/boot/mbr/boot.bin os/boot/mbr/orangefs_boot.bin os/boot/mbr/loader.bin #modified by mingxuan 2019-5-17


ORANGESKERNEL	= kernel.bin

# OBJS		= os/kernel/kernel.o os/kernel/start.o os/kernel/main.o os/kernel/clock.o\
# 			os/kernel/i8259.o os/kernel/global.o os/kernel/protect.o os/kernel/proc.o\
# 			os/kernel/memman.o os/kernel/pagetbl.o	\
# 			os/kernel/elf.o os/kernel/file.o os/kernel/exec.o os/kernel/fork.o os/kernel/pthread.o \
# 			os/kernel/ktest.o os/kernel/testfunc.o os/kernel/fs.o os/kernel/hd.o \
# 			os/kernel/spinlock.o os/kernel/fat32.o os/kernel/base.o os/kernel/assist.o os/kernel/vfs.o \
# 			os/kernel/keyboard.o os/kernel/tty.o os/kernel/shell.o os/kernel/console.o \
# 			os/kernel/wait.o os/kernel/exit.o \
# 			os/kernel/signal.o \
# 			os/kernel/buddy.o os/kernel/kmalloc.o \
# 			os/kernel/syscallc.o \
# 			os/kernel/pthread_mutex.o os/kernel/pthread_cond.o \
# 			os/ipc/shm.o os/ipc/msg.o \
# 			os/klib/klib.a os/kernel/semaphore.o os/kernel/ahci.o\
# 			os/kernel/mount.o \
# 			os/kernel/buffer.o os/kernel/slab.o \
# 			os/gdbstub/gdbstub_x86_int.o

OBJS =  os/kernel/kernel.o \
	$(subst .c,.o,$(wildcard os/kernel/*.c) $(wildcard os/ipc/*.c)) \
	os/klib/klib.a os/gdbstub/gdbstub_x86_int.o 
			
#added by mingxuan 2021-2-28

OBJSKLIB 	= os/kernel/syscall.o os/klib/klib.o os/klib/kliba.o os/klib/string.o

GDBBIN = kernel.gdb.bin

include os/boot/mbr/Makefile

.PHONY : everything clean_os realclean_os disasm #modified by mingxuan 2019-5-19

everything : $(ORANGESBOOT) $(ORANGESKERNEL) $(GDBBIN) # modified by mingxuan 2021-4-6

clean_os :
	rm -f $(OBJS) $(OBJSKLIB)

realclean_os :
	rm -f $(OBJS) $(ORANGESBOOT) $(ORANGESKERNEL) $(OBJSKLIB) $(GDBBIN) $(MBR_LOADER_OBJS) os/klib/stringasm.o os/klib/stringc.o # modified by mingxuan 2021-2-7

disasm :
	$(DASM) $(DASMFLAGS) $(ORANGESKERNEL) > $(DASMOUTPUT)

# added by mingxuan 2019-5-17
os/boot/floppy/boot.bin : os/boot/floppy/boot.asm os/boot/floppy/include/load.inc os/boot/floppy/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

# added by mingxuan 2019-5-17
os/boot/floppy/loader.bin : os/boot/floppy/loader.asm os/boot/floppy/include/load.inc os/boot/floppy/include/fat12hdr.inc os/boot/floppy/include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

# added by mingxuan 2019-5-17
#os/boot/mbr/mbr.bin : os/boot/mbr/mbr.asm
#	$(ASM) $(ASMBFLAGS_mbr) -o $@ $<

# added by mingxuan 2019-5-17
#os/boot/mbr/boot.bin : os/boot/mbr/boot.asm
#	$(ASM) $(ASMBFLAGS_mbr) -o $@ $<

# added by mingxuan 2019-5-17
#os/boot/mbr/loader.bin : os/boot/mbr/loader.asm os/boot/mbr/include/loader.inc os/boot/mbr/include/fat32.inc os/boot/mbr/include/pm.inc
#	$(ASM) $(ASMBFLAGS_mbr) -o $@ $<

$(ORANGESKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS_kernel) -o $(ORANGESKERNEL) $(OBJS)

os/kernel/kernel.o : os/kernel/kernel.asm os/include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<


kernel.gdb.bin : $(OBJS)
	$(LD) $(LDFLAGS_kernel_gdb) -o $@ $(OBJS)

#user/init/init.gdb.bin : $(OBJSINIT)
#	$(LD) $(LDFLAGS_init_gdb) -o $@ $(OBJSINIT)

# spinlock.o	added by mingxuan 2019-1-5
os/kernel/spinlock.o: os/kernel/spinlock.c os/include/spinlock.h
	$(CC) -masm=intel  -I os/include/ $(KFLAGS) -o $@ $<

os/kernel/%.o : os/kernel/%.c
	$(CC)  -I os/include/ $(KFLAGS) -o $@ $<

os/ipc/%.o : os/ipc/%.c
	$(CC)  -I os/include/ $(KFLAGS) -o $@ $<

os/gdbstub/gdbstub_x86_int.o: os/gdbstub/gdbstub_x86_int.nasm
	nasm -I os/gdbstub/ -f elf -o $@ $<

# =================== klib ==========================
os/klib/klib.a:  $(OBJSKLIB)
	$(AR) $(ARFLAGS) -o $@  $(OBJSKLIB)

os/klib/kliba.o : os/klib/kliba.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

os/klib/%.o: os/klib/%.c
	$(CC)  -I os/include/ $(CFLAGS) -o $@ $<