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

LDFLAGS_init	= -m elf_i386 -s -Tuser/init/lds	#added by mingxuan 2021-4-6
LDFLAGS_init_gdb	= -m elf_i386					#added by mingxuan 2021-4-6
LDFLAGS_user_gdb	= -m elf_i386					#added by mingxuan 2021-8-21

USER_SRC_DIR=user/user
USER_INIT_DIR=user/init
USER_ULIB_DIR=user/ulib
USER_SYS_DIR=user/system

ORANGESINIT	= user/init/init.bin	#added by mingxuan 2021-4-6

USER_SRC =  t_xv6.c  twait.c test_0.c test_1.c fstest.c t_vfspth.c t_pthr01.c t_pthr02.c t_pthr03.c 
# user/user/test10.c  user/user/test_2.c  user/user/test4.c   user/user/test_7.c  \
user/user/test_0.c   user/user/test_1.c  user/user/test_4.c  user/user/test6.c  \
user/user/t_exit01.c  user/user/twait.c	user/user/tfork.c  user/user/fstest.c  \
# user/user/myTest.c   user/user/shell_2.c  user/user/test11.c  user/user/test2.c   user/user/test_5.c  user/user/test7.c     user/user/t_pthr01.c \
# user/user/rwtest1.c  user/user/sig_0.c    user/user/test12.c  user/user/test_3.c  user/user/test5.c   user/user/test8.c     user/user/t_pthr02.c \
# user/user/sema1.c    user/user/sig_1.c    user/user/test13.c  user/user/test3.c   user/user/test_6.c  user/user/test9.c     user/user/t_pthr03.c \
#$(wildcard $(USER_SRC_DIR)/*.c)
OBJSUSER = $(subst .c,.o,$(addprefix $(USER_SRC_DIR)/, $(USER_SRC)))
OBJSSYS = $(subst .c,.o,$(wildcard $(USER_SYS_DIR)/*.c))
OBJSULIB = $(subst .c,.o, $(wildcard $(USER_ULIB_DIR)/*.c ))
# OBJSULIB 	= user/ulib/syscall.o user/ulib/string.o user/ulib/printf.o user/ulib/vsprintf.o user/ulib/scanf.o\
# 			user/ulib/signal.o user/ulib/malloc.o user/ulib/ushm.o

OBJSINIT	= user/init/init.o user/init/initstart.o	#added by mingxuan 2021-4-6

ORANGESBIN = $(subst .o,.bin, $(OBJSUSER) $(OBJSSYS)) $(ORANGESINIT)

ORANGESRAW = $(wildcard user/raw/*)
#added by mingxuan 2021-8-21
GDBBINUSER =


GDBBIN = user/init/init.gdb.bin	#added by mingxuan 2021-4-6


# All Phony Targets
.PHONY : everything clean_user realclean_user # install_elf  modified by mingxuan 2019-5-19 modified by sundong 2023.5.18

# everything : realclean_user $(OBJSULIB) $(ORANGESINIT) $(ORANGESBIN) install_elf clean_user # modified by mingxuan 2021-2-7
# everything : realclean_user $(OBJSULIB) $(ORANGESINIT) $(GDBBIN) $(ORANGESBIN) install_elf clean_user # modified by mingxuan 2021-8-8
everything : $(OBJSULIB) $(ORANGESINIT) $(GDBBIN) $(ORANGESBIN) $(GDBBINUSER) #install_elf modified by mingxuan 2021-8-21 modified by sundong 2023.5.18

clean_user :
	rm -f $(OBJSINIT) $(OBJSUSER) $(OBJSULIB)

realclean_user :
	rm -f $(ORANGESINIT) $(ORANGESBIN) $(OBJSULIB) $(OBJSINIT) $(OBJSUSER) $(GDBBIN) $(GDBBINUSER) user/ulib/*.o user/user/userstart.o

#install_elf:
#	cd user/init && \
#	make


#--------------------------------------init-------------------------------------------------
$(GDBBIN) : $(OBJSINIT)
#	$(LD) $(LDFLAGS_init_gdb) -o $@ $(OBJSINIT)
	$(LD) $(LDFLAGS_init_gdb) -o $(GDBBIN) $(OBJSINIT) $(OBJSULIB)	# modified by mingxuan 2021-8-8

$(ORANGESINIT) : $(OBJSINIT)
	$(LD) $(LDFLAGS_init) -o $(ORANGESINIT) $(OBJSINIT) $(OBJSULIB)

user/init/init.o: user/init/init.c user/include/stdio.h
	$(CC) -I user/include/ $(CFLAGS) -o $@ $<

user/init/initstart.o: user/init/initstart.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<


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

$(USER_SRC_DIR)/%.bin : $(USER_SRC_DIR)/%.o user/user/userstart.o $(OBJSULIB)
	$(LD) $(LDFLAGS) -o $@ $^

user/system/%.o: $(USER_SYS_DIR)/%.c
	$(CC) -I user/include/ $(CFLAGS) -o $@ $<
# $$(USER_SYS_DIR)
user/system/%.bin : $(USER_SYS_DIR)/%.o user/user/userstart.o $(OBJSULIB)
	$(LD) $(LDFLAGS) -o $@ $^

user/user/%.gdb.bin : user/user/%.o user/user/userstart.o $(OBJSULIB)
	$(LD) $(LDFLAGS_user_gdb) -o $@ $^
