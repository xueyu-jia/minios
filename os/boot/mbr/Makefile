#add by sundong 2023.3.26
#构建mbr boot和loader的makefile
MBR_LOADER_DIR=os/boot/mbr
MBR_LOADER_INC_DIR=$(MBR_LOADER_DIR)/include

MBR_LOADER_SRC :=$(wildcard $(MBR_LOADER_DIR)/*.c )
MBR_LOADER_SRC += $(MBR_LOADER_DIR)/loader.asm

#构建loader时所有的.o文件
MBR_LOADER_OBJS:=$(patsubst %.c,%.o,$(MBR_LOADER_SRC))
MBR_LOADER_OBJS:=$(patsubst %.asm,%.o,$(MBR_LOADER_OBJS))

MBR_LOADER_LINKER=$(MBR_LOADER_DIR)/linker.ld

MBR_LOADER_CFLAGS= -g -I $(MBR_LOADER_INC_DIR) $(CFLAGS)
MBR_LOADER_ASMBFLAGS =-f elf  -g -F dwarf -I $(MBR_LOADER_INC_DIR) 
#loader为二进制文件并非elf
MBR_LOADER_LDFLAGS = -m elf_i386 -nostdlib -s -T $(MBR_LOADER_LINKER) --oformat binary 

$(MBR_LOADER_DIR)/mbr.bin : $(MBR_LOADER_DIR)/mbr.asm
	$(ASM) $(ASMBFLAGS_mbr) -o $@ $<
$(MBR_LOADER_DIR)/boot.bin : $(MBR_LOADER_DIR)/boot.asm
	$(ASM) $(ASMBFLAGS_mbr) -o $@ $<
	
#编译出所有的.o文件
$(MBR_LOADER_DIR)/%.o:$(MBR_LOADER_DIR)/%.c
	$(CC)  $(MBR_LOADER_CFLAGS) -o $@ $<
$(MBR_LOADER_DIR)/loader.o:$(MBR_LOADER_DIR)/loader.asm
	$(ASM)  $(MBR_LOADER_ASMBFLAGS) -o $@ $<
#链接loader.bin
$(MBR_LOADER_DIR)/loader.bin:$(MBR_LOADER_OBJS)
	$(LD) $(MBR_LOADER_LDFLAGS) -o $@ $^ 
	$(LD) -g -m elf_i386 -nostdlib -s -T $(MBR_LOADER_LINKER) -o $@.gdb $^ 