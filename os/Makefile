# Entry point of kernel
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
ASMKFLAGS	= -f elf
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)
ARFLAGS		= rcs
DEF_FLAGS 	= -DROOT_FSTYPE=\"$(ROOT_PART_FS_TYPE)\" -DROOT_PART=$(ROOT_FS_PART_NUM) # 指定根文件系统的分区和类型
OS_CFLAGS		= -m32 -mno-sse -mno-mmx -c -fno-builtin -fno-stack-protector -Wall -Wextra -g -std=gnu99 -fdump-rtl-expand
KFLAGS		= $(OS_CFLAGS) $(DEF_FLAGS)

#modified by jiangfeng 2024-03-10 add definitions option for root

LDFLAGS_kernel	= -m elf_i386 -s -Ttext $(ENTRYPOINT) #modified by mingxuan 2021-3-29
#LDFLAGS_init	= -m elf_i386 -s -T../user/init/lds

LDFLAGS_kernel_gdb	= -m elf_i386 -Ttext $(ENTRYPOINT)
#LDFLAGS_init_gdb	= -m elf_i386

# This Program
ORANGESBOOT	= os/boot/floppy/boot.bin os/boot/floppy/loader.bin \
			  os/boot/mbr/mbr.bin os/boot/mbr/boot.bin os/boot/mbr/orangefs_boot.bin os/boot/mbr/loader.bin #modified by mingxuan 2019-5-17


ORANGESKERNEL	= kernel.bin

OBJS =  os/kernel/kernel.o \
	$(subst .c,.o,$(wildcard os/kernel/*.c) $(wildcard os/ipc/*.c) $(wildcard os/fs/*.c) \
	$(wildcard os/klib/*.c)) $(subst .nasm,.o,$(wildcard os/gdbstub/*.nasm))


GDBBIN = kernel.gdb.bin

include os/boot/floppy/Makefile
include os/boot/mbr/Makefile

.PHONY : everything clean_os realclean_os disasm #modified by mingxuan 2019-5-19

everything : $(ORANGESBOOT) $(ORANGESKERNEL) $(GDBBIN) # modified by mingxuan 2021-4-6

clean_os :
	rm -f $(OBJS) $(OBJSKLIB)

realclean_os :
#	rm -f $(OBJS) $(ORANGESBOOT) $(ORANGESKERNEL) $(OBJSKLIB) $(GDBBIN) $(MBR_LOADER_OBJS)
	rm -f $(OBJS) $(ORANGESBOOT) $(ORANGESKERNEL) $(GDBBIN) $(MBR_LOADER_OBJS) *.expand

disasm :
	$(DASM) $(DASMFLAGS) $(ORANGESKERNEL) > $(DASMOUTPUT)

$(ORANGESKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS_kernel) -o $(ORANGESKERNEL) $(OBJS)

os/kernel/kernel.o : os/kernel/kernel.asm os/include/sconst.inc
	$(ASM) -I os/include/ $(ASMKFLAGS) -o $@ $<


kernel.gdb.bin : $(OBJS)
	$(LD) $(LDFLAGS_kernel_gdb) -o $@ $(OBJS)

os/kernel/%.o : os/kernel/%.c
	$(CC)  -I os/include/ $(KFLAGS) -o $@ $<
	mkdir -p rtl && mv "os/kernel/$*.c.245r.expand" rtl/;
# $(shell mkdir -p rtl && mv os/kernel/*.expand rtl/)

os/ipc/%.o : os/ipc/%.c
	$(CC)  -I os/include/ $(KFLAGS) -o $@ $<
	mkdir -p rtl && mv "os/ipc/$*.c.245r.expand" rtl/;

os/fs/%.o : os/fs/%.c
	$(CC)  -I os/include/ $(KFLAGS) -o $@ $<
	mkdir -p rtl && mv "os/fs/$*.c.245r.expand" rtl/;

os/gdbstub/gdbstub_x86_int.o: os/gdbstub/gdbstub_x86_int.nasm
	$(ASM) -I os/gdbstub/ -f elf -o $@ $<

os/klib/%.o: os/klib/%.c
	$(CC)  -I os/include/ $(OS_CFLAGS) -o $@ $<
	mkdir -p rtl && mv "os/klib/$*.c.245r.expand" rtl/;
