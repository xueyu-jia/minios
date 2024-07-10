#########################
# Makefile for Orange'S #
#########################

# Programs, flags, etc.
ASM		= nasm
DASM	= ndisasm
CC		= gcc
LD		= ld
AR		= ar

ASMKFLAGS	= -f elf
ARFLAGS		= rcs

CFLAGS		= -m32 -mno-sse -mno-mmx -c -fno-builtin -fno-stack-protector -Wall -Wextra -g -std=gnu99 
LDFLAGS		= -m elf_i386 -s -Tuser/user/lds

LDFLAGS_user_gdb	= -m elf_i386					#added by mingxuan 2021-8-21

USER_SRC_DIR=user/user
USER_INIT_DIR=user/init
USER_ULIB_DIR=user/ulib

USER_FILES = $(shell find "$(USER_SRC_DIR)" -type f) #user/user下所有文件
USER_OBJS = $(subst .c,.o,$(filter %.c, $(USER_FILES))) #.c文件需要编译
USER_BIN = $(subst .o,.bin,$(USER_OBJS))
USER_EXECUTE = $(USER_BIN) $(filter %.sh,$(USER_FILES))

#默认加入user/user下的所有.sh文件和.c编译得到的.bin文件(为了便于命令使用复制时其目标文件名略去.bin文件的后缀扩展名)
#如需要添加.sh和elf文件之外的文件
#此处USER_EXTEND_FILES中添加相对"user/user/"下的文件路径名即可
USER_EXTEND_FILES = 

#最终的复制文件
USER_IMG = $(USER_EXECUTE) $(addprefix $(USER_SRC_DIR)/,$(USER_EXTEND_FILES))

OBJSULIB = $(subst .c,.o, $(wildcard $(USER_ULIB_DIR)/*.c ))
# OBJSULIB 	= user/ulib/syscall.o user/ulib/string.o user/ulib/printf.o user/ulib/vsprintf.o user/ulib/scanf.o\
# 			user/ulib/signal.o user/ulib/malloc.o user/ulib/ushm.o

#added by mingxuan 2021-8-21
GDBBINUSER =


GDBBIN = user/user/bin/init.gdb.bin	#added by mingxuan 2021-4-6

# All Phony Targets
.PHONY : everything clean_user realclean_user # install_elf  modified by mingxuan 2019-5-19 modified by sundong 2023.5.18

everything : $(OBJSULIB) $(GDBBIN) $(USER_BIN) $(GDBBINUSER) #install_elf modified by mingxuan 2021-8-21 modified by sundong 2023.5.18

clean_user :
	rm -f $(USER_OBJS) $(OBJSULIB)

realclean_user :
	rm -f $(USER_BIN) $(OBJSULIB) $(USER_OBJS) $(GDBBIN) $(GDBBINUSER) user/user/userstart.o

#--------------------------------------ulib-------------------------------------------------
user/ulib/ulib.a:  $(OBJSULIB)
	$(AR) $(ARFLAGS) -o $@  $(OBJSULIB)

user/ulib/%.o: user/ulib/%.c
	$(CC) -I user/include/ $(CFLAGS) -o $@ $<

#--------------------------------------user-------------------------------------------------
user/user/userstart.o: user/user/userstart.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

# added by mingxuan 2021-2-7
$(USER_SRC_DIR)/%.o: $(USER_SRC_DIR)/%.c
	$(CC) -I user/include/ $(CFLAGS) -o $@ $<

$(USER_SRC_DIR)/%.gdb.bin : $(USER_SRC_DIR)/%.o user/user/userstart.o $(OBJSULIB)
	$(LD) $(LDFLAGS_user_gdb) -o $@ $^

$(USER_SRC_DIR)/%.bin : $(USER_SRC_DIR)/%.o user/user/userstart.o $(OBJSULIB)
	$(LD) $(LDFLAGS) -o $@ $^
