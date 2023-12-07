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

CFLAGS		= -m32 -c -fno-builtin -fno-stack-protector -Wall -Wextra -g -std=gnu99 
LDFLAGS		= -m elf_i386 -s -Tuser/user/lds

LDFLAGS_init	= -m elf_i386 -s -Tuser/init/lds	#added by mingxuan 2021-4-6
LDFLAGS_init_gdb	= -m elf_i386					#added by mingxuan 2021-4-6
LDFLAGS_user_gdb	= -m elf_i386					#added by mingxuan 2021-8-21

USER_SRC_DIR=user/user
USER_INIT_DIR=user/init
USER_ULIB_DIR=user/ulib

ORANGESINIT	= user/init/init.bin	#added by mingxuan 2021-4-6

SRCUSER = $(wildcard $(USER_SRC_DIR)/*.c)
OBJSUSER = $(subst .c,.o, $(SRCUSER))
# OBJSUSER = user/user/shell_0.o user/user/shell_1.o user/user/shell_2.o \
# 		user/user/test_0.o user/user/test_1.o user/user/test_2.o user/user/test_3.o user/user/test_4.o\
# 		user/user/test_5.o user/user/test_6.o user/user/test_7.o user/user/sig_0.o user/user/sig_1.o user/user/userstart.o user/user/myTest.o\
# 		user/user/fortest1.o user/user/fortest2.o user/user/fortest3.o user/user/fortest4.o user/user/fortest5.o \
# 		user/user/fortest6.o user/user/fortest7.o user/user/fortest8.o user/user/fortest9.o user/user/fortest10.o \
# 		user/user/fortest11.o user/user/fortest12.o user/user/fortest13.o user/user/t_exit01.o \
# 		user/user/t_pthr01.o user/user/t_pthr02.o user/user/t_pthr03.o user/user/sema1.o \
# 		user/user/fstest.o user/user/rwtest1.o

ORANGESUSER = $(subst .o,.bin, $(OBJSUSER))
# ORANGESUSER = user/user/shell_0.bin user/user/shell_1.bin user/user/shell_2.bin \
# 			user/user/test_0.bin user/user/test_1.bin user/user/test_2.bin user/user/test_3.bin user/user/test_4.bin\
# 			user/user/test_5.bin user/user/test_6.bin user/user/test_7.bin user/user/sig_0.bin user/user/sig_1.bin user/user/myTest.bin \
# 			user/user/ptest1.bin user/user/ptest2.bin user/user/ptest3.bin user/user/ptest4.bin user/user/ptest5.bin \
# 			user/user/ptest6.bin user/user/ptest7.bin user/user/ptest8.bin user/user/ptest9.bin user/user/ptest10.bin \
# 			user/user/ptest11.bin user/user/ptest12.bin user/user/ptest13.bin user/user/ptest13.gdb.bin  \
# 			user/user/t_exit01.bin user/user/t_pthr01.bin user/user/t_pthr02.bin user/user/t_pthr03.bin user/user/sema1.bin \
# 			user/user/fstest.bin user/user/rwtest1.bin

#added by mingxuan 2021-8-21
GDBBINUSER = user/user/shell_0.gdb.bin user/user/test_1.gdb.bin

OBJSULIB = $(subst .c,.o, $(wildcard $(USER_ULIB_DIR)/*.c ))
# OBJSULIB 	= user/ulib/syscall.o user/ulib/string.o user/ulib/printf.o user/ulib/vsprintf.o user/ulib/scanf.o\
# 			user/ulib/signal.o user/ulib/malloc.o user/ulib/ushm.o

OBJSINIT	= user/init/init.o user/init/util.o user/init/initstart.o	#added by mingxuan 2021-4-6

GDBBIN = user/init/init.gdb.bin	#added by mingxuan 2021-4-6

include user/init/makefile	 #added by mingxuan 2019-5-19

# All Phony Targets
.PHONY : everything clean_user realclean_user # install_elf  modified by mingxuan 2019-5-19 modified by sundong 2023.5.18

# everything : realclean_user $(OBJSULIB) $(ORANGESINIT) $(ORANGESUSER) install_elf clean_user # modified by mingxuan 2021-2-7
# everything : realclean_user $(OBJSULIB) $(ORANGESINIT) $(GDBBIN) $(ORANGESUSER) install_elf clean_user # modified by mingxuan 2021-8-8
everything : $(OBJSULIB) $(ORANGESINIT) $(GDBBIN) $(ORANGESUSER) $(GDBBINUSER) #install_elf modified by mingxuan 2021-8-21 modified by sundong 2023.5.18

clean_user :
	rm -f $(OBJSINIT) $(OBJSUSER) $(OBJSULIB)

realclean_user :
	rm -f $(ORANGESINIT) $(ORANGESUSER) $(OBJSULIB) $(OBJSINIT) $(OBJSUSER) $(GDBBIN) $(GDBBINUSER) user/ulib/*.o user/user/userstart.o

#install_elf:
#	cd user/init && \
#	make


#--------------------------------------init-------------------------------------------------
$(GDBBIN) : $(OBJSINIT)
#	$(LD) $(LDFLAGS_init_gdb) -o $@ $(OBJSINIT)
	$(LD) $(LDFLAGS_init_gdb) -o $(GDBBIN) $(OBJSINIT) $(OBJSULIB)	# modified by mingxuan 2021-8-8

$(ORANGESINIT) : $(OBJSINIT)
	$(LD) $(LDFLAGS_init) -o $(ORANGESINIT) $(OBJSINIT) $(OBJSULIB)

user/init/util.o: user/init/util.c user/init/util.h user/include/stdio.h
	$(CC) -I user/include/ $(CFLAGS) -o $@ $<

user/init/init.o: user/init/init.c user/include/stdio.h
	$(CC) -I user/include/ $(CFLAGS) -o $@ $<

user/init/initstart.o: user/init/initstart.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<


#--------------------------------------ulib-------------------------------------------------
user/ulib/ulib.a:  $(OBJSULIB)
	$(AR) $(ARFLAGS) -o $@  $(OBJSULIB)

# stringasm.o	added by mingxuan 2019-5-19
user/ulib/stringasm.o: user/ulib/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

# stringc.o	added by mingxuan 2019-5-19
user/ulib/stringc.o:	user/ulib/string.c
	$(CC) -I user/include/ $(CFLAGS) -o $@ $<

# string.o	added by mingxuan 2019-5-19
user/ulib/string.o : user/ulib/stringasm.o user/ulib/stringc.o
	$(LD) -m elf_i386 -r -o $@ $<

user/ulib/%.o: user/ulib/%.c
	$(CC) -I user/include/ $(CFLAGS) -o $@ $<

#--------------------------------------user-------------------------------------------------
user/user/userstart.o: user/user/userstart.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

# added by mingxuan 2021-2-7
user/user/%.o: user/user/%.c
	$(CC) -I user/include/ $(CFLAGS) -o $@ $<

user/user/%.bin : user/user/%.o user/user/userstart.o $(OBJSULIB)
	$(LD) $(LDFLAGS) -o $@ $^

user/user/%.gdb.bin : user/user/%.o user/user/userstart.o $(OBJSULIB)
	$(LD) $(LDFLAGS_user_gdb) -o $@ $^
#ulib/syscall.o : ulib/syscall.asm include/sconst.inc
# user/ulib/syscall.o : user/ulib/syscall.asm
# 	$(ASM) $(ASMKFLAGS) -o $@ $<		# deleted by zhenhao 2023.3.5
# user/ulib/syscall.o : user/ulib/syscall.c user/include/syscall.h
# 	$(CC) $(CFLAGS) -o $@ $<			# added by zhenhao 2023.3.5
# user/ulib/malloc.o: user/ulib/malloc.c user/include/malloc.h user/include/type.h
# 	$(CC) $(CFLAGS) -o $@ $<

# # printf.o	added by mingxuan 2019-5-19
# user/ulib/printf.o: user/ulib/printf.c
# 	$(CC) $(CFLAGS) -o $@ $<

# # vsprintf.o	added by mingxuan 2019-5-19
# user/ulib/vsprintf.o: user/ulib/vsprintf.c
# 	$(CC) $(CFLAGS) -o $@ $<

# # scanf.o	added by mingxuan 2019-5-19
# user/ulib/scanf.o: user/ulib/scanf.c
# 	$(CC) $(CFLAGS) -o $@ $<

# user/ulib/signal.o: user/ulib/signal.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/ulib/ushm.o: user/ulib/ushm.c
# 	$(CC) $(CFLAGS) -o $@ $<

#--------------------------------------user-------------------------------------------------
# user/user/userstart.o: user/user/userstart.asm
# 	$(ASM) $(ASMKFLAGS) -o $@ $<

# # added by mingxuan 2021-2-7
# user/user/shell_0.o: user/user/shell_0.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/shell_0.bin : user/user/shell_0.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# #added by mingxuan 2021-8-21
# user/user/shell_0.gdb.bin : user/user/shell_0.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS_user_gdb) -o $@ $^

# user/user/shell_1.o: user/user/shell_1.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/shell_1.bin : user/user/shell_1.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^


# user/user/shell_2.o: user/user/shell_2.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/shell_2.bin : user/user/shell_2.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^



# user/user/test_0.o: user/user/test_0.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/test_0.bin : user/user/test_0.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^


# user/user/test_1.o: user/user/test_1.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/test_1.bin : user/user/test_1.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# # added by mingxuan 2021-8-21
# user/user/test_1.gdb.bin : user/user/test_1.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS_user_gdb) -o $@ $^

# user/user/test_2.o: user/user/test_2.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/test_2.bin : user/user/test_2.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/test_3.o: user/user/test_3.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/test_3.bin : user/user/test_3.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/test_4.o: user/user/test_4.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/test_4.bin : user/user/test_4.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/test_5.o: user/user/test_5.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/test_5.bin : user/user/test_5.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/test_6.o: user/user/test_6.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/test_6.bin : user/user/test_6.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/test_7.o: user/user/test_7.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/test_7.bin : user/user/test_7.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/sig_0.o: user/user/sig_0.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/sig_0.bin : user/user/sig_0.o user/ulib/signal.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/sig_1.o: user/user/sig_1.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/sig_1.bin : user/user/sig_1.o user/ulib/signal.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# # added by yingchi 2022.01.05
# user/user/myTest.o: user/user/myTest.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/myTest.bin : user/user/myTest.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

	
# user/user/fortest1.o: user/user/fortest1.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest1.bin : user/user/fortest1.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest2.o: user/user/fortest2.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest2.bin : user/user/fortest2.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest3.o: user/user/fortest3.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest3.bin : user/user/fortest3.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest4.o: user/user/fortest4.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest4.bin : user/user/fortest4.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest5.o: user/user/fortest5.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest5.bin : user/user/fortest5.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest6.o: user/user/fortest6.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest6.bin : user/user/fortest6.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest7.o: user/user/fortest7.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest7.bin : user/user/fortest7.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest8.o: user/user/fortest8.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest8.bin : user/user/fortest8.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest9.o: user/user/fortest9.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest9.bin : user/user/fortest9.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest10.o: user/user/fortest10.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest10.bin : user/user/fortest10.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest11.o: user/user/fortest11.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest11.bin : user/user/fortest11.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest12.o: user/user/fortest12.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest12.bin : user/user/fortest12.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/fortest13.o: user/user/fortest13.c user/include/stdio.h 
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/ptest13.bin : user/user/fortest13.o  user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/ptest13.gdb.bin : user/user/fortest13.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS_user_gdb) -o $@ $^

# user/user/fstest.o:user/user/fstest.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<
# user/user/fstest.bin :user/user/fstest.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^





# user/user/t_exit01.o: user/user/t_exit01.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/t_exit01.bin : user/user/t_exit01.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/t_pthr01.o: user/user/t_pthr01.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/t_pthr01.bin : user/user/t_pthr01.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/t_pthr02.o: user/user/t_pthr02.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/t_pthr02.bin : user/user/t_pthr02.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/t_pthr03.o: user/user/t_pthr03.c user/include/stdio.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/t_pthr03.bin : user/user/t_pthr03.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/sema1.o: user/user/sema1.c user/include/stdio.h user/include/pthread.h user/include/syscall.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/sema1.bin : user/user/sema1.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

# user/user/rwtest1.o: user/user/rwtest1.c user/include/stdio.h user/include/syscall.h
# 	$(CC) $(CFLAGS) -o $@ $<

# user/user/rwtest1.bin : user/user/rwtest1.o user/user/userstart.o $(OBJSULIB)
# 	$(LD) $(LDFLAGS) -o $@ $^

