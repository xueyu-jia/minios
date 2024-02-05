#########################
# Makefile for Orange'S #
#########################
SHELL := /bin/bash
# added by mingxuan 2020-9-12
# Offset of os_boot in hd
# 活动分区所在的扇区号
# OSBOOT_SEC = 4096
# 活动分区所在的扇区号对应的字节数
# OSBOOT_OFFSET = $(OSBOOT_SEC)*512
#OSBOOT_OFFSET = 1048576


# added by mingxuan 2020-10-29
# Offset of fs in hd
# 文件系统标志所在扇区号 = 文件系统所在分区的第1个扇区 + 1
# ORANGE_FS_SEC = 6144 + 1 = 6145
# 文件系统标志所在扇区 = $(ORANGE_FS_SEC)*512
#ORANGE_FS_START_OFFSET = 3146240
# FAT32_FS_SEC = 53248 + 1 = 53249
# 文件系统标志所在扇区 = $(ORANGE_FS_SEC)*512
#FAT32_FS_START_OFFSET = 27263488


# Programs, flags, etc.
ASM		= nasm
DASM	= ndisasm
CC		= gcc
LD		= ld
AR		= ar

#added by sundong 2023.10.28
########################用户可以输入的变量##########################
#选择OS的启动环境，virtual 代表虚机（qemu），real 代表真机
MACHINE_TYPE = virtual
#安装的硬盘，例如真机启动时该变量可能为 /dev/sda；虚机启动无需设置此变量
INS_DEV=
#启动分区的分区号，数字类型，例如: 1
BOOT_PART_NUM=1
#根文件系统所在的分区号，数字类型，例如：2
ROOT_FS_PART_NUM=2
#用于区分是使用grub chainloader
#可选值为 true  false
USING_GRUB_CHAINLOADER = false
#grub安装的分区,数字类型，例如：5
GRUB_PART_NUM=5
#选择启动分区的文件系统格式，目前仅支持fat32和orangefs
BOOT_PART_FS_TYPE= fat32
ROOT_PART_FS_TYPE= orangefs
#grub的配置文件,提供了一个默认的grub配置文件，配置为从第1块硬盘分区1引导
GRUB_CONFIG=boot_from_part1.cfg
#使用虚拟机时虚拟镜像的名称，该虚拟镜像应该放在hd/文件夹下
BOOT_IMG=virtual_disk.img
###################################################################


#added by sundong 写镜像用,用losetup -f查看
ifeq ($(MACHINE_TYPE), virtual)
	FREE_LOOP:=$(shell sudo losetup -f)
	INS_DEV=$(FREE_LOOP)
	BOOT_PART=$(INS_DEV)p$(BOOT_PART_NUM)
	ROOT_FS_PART=$(INS_DEV)p$(ROOT_FS_PART_NUM)
	GRUB_INSTALL_PART=$(INS_DEV)p$(GRUB_PART_NUM)
	WRITE_DISK=b.img
else
#	FREE_LOOP=/dev/sda
	BOOT_PART=$(INS_DEV)$(BOOT_PART_NUM)
	ROOT_FS_PART=$(INS_DEV)$(ROOT_FS_PART_NUM)
	GRUB_INSTALL_PART=$(INS_DEV)$(GRUB_PART_NUM)
	WRITE_DISK=$(INS_DEV)
endif

ORANGE_CP=./o_copy

ifeq ($(BOOT_PART_FS_TYPE),fat32)
	BOOT_PART_FS_MAKER = mkfs.vfat
	BOOT_PART_FS_MAKE_FLAG = -F 32 -s8
	CP = cp
	CP_FLAG = -fv
	BOOT =boot.bin
	BOOT_SIZE = 420
# FAT32规范规定os_boot的前89个字节是FAT32的配置信息
	BOOT_SEEK=90
	BOOT_PART_MOUNTPOINT = iso

else ifeq ($(BOOT_PART_FS_TYPE),orangefs)
	BOOT_PART_FS_MAKER = ./o_mkfs
	BOOT_PART_FS_MAKE_FLAG = 
	CP = $(ORANGE_CP)
	BOOT=orangefs_boot.bin
	BOOT_SIZE =512
# orangefs boot直接放在分区的0号扇区
	BOOT_SEEK=0
	BOOT_PART_MOUNTPOINT =$(BOOT_PART)
endif

# config root fs param
ifeq ($(ROOT_PART_FS_TYPE),fat32)
	ROOT_CP = cp
	ROOT_MOUNTPOINT = root
	ROOT_FS_MAKER = mkfs.vfat
	ROOT_FS_MAKE_FLAG = -F 32 -s8
else ifeq ($(ROOT_PART_FS_TYPE),orangefs)
	ROOT_CP = $(ORANGE_CP)
	ROOT_MOUNTPOINT = $(ROOT_FS_PART)
	ROOT_FS_MAKER = ./o_mkfs
	ROOT_FS_MAKE_FLAG =
endif



# All Phony Targets
.PHONY : all clean install

all : everything
clean : realclean_os realclean_user clean_user
install : check_para build_img build_grub build_mbr build_fs

include ./os/Makefile
include	./user/Makefile
check_para:
#检查MACHINE_TYPE输入是否正确(仅支持real,virtual)
ifeq ($(MACHINE_TYPE),real) 
#检查在真机启动情况下是否输入了INS_DEV
ifndef INS_DEV
	$(error INS_DEV is null! The INS_DEV needs a vlaue when using real machine.)
endif 
else ifeq ($(MACHINE_TYPE),virtual) 
	$(info MACHINE_TYPE=$(MACHINE_TYPE))
ifeq ($(BOOT_PART_NUM), $(ROOT_FS_PART_NUM))
	$(info BOOT_PART_NUM= $(BOOT_PART_NUM))
	$(info ROOT_FS_PART_NUM= $(ROOT_FS_PART_NUM))
endif 
else 
	$(error MACHINE_TYPE input error! Correct vlaue: virtual or real ,your input:$(MACHINE_TYPE) )
endif 


# #检查BOOT_PART_FS_TYPE是否输入正确(仅支持fat32,orangefs)
# ifeq ($(BOOT_PART_FS_TYPE),fat32)
# #检查fat32作为启动分区文件系统时，启动分区是否和根文件系统分区重合
# ifeq ($(BOOT_PART_NUM),$(ROOT_FS_PART_NUM))
# 	$(error  BOOT_PART_NUM equals ROOT_FS_PART_NUM! This is not allowed when using $(BOOT_PART_FS_TYPE) as boot part file system )
# endif
# else ifeq ($(BOOT_PART_FS_TYPE),orangefs)
# #检查orangefs作为启动分区文件系统时 启动分区是否等于根文件系统分区
# ifneq ($(BOOT_PART_NUM),$(ROOT_FS_PART_NUM))
# 	$(error  BOOT_PART_NUM not equals ROOT_FS_PART_NUM! This is not allowed when using $(BOOT_PART_FS_TYPE) as boot part file system )
# endif
# else 
# 	$(error BOOT_PART_FS_TYPE input error! Correct vlaue: fat32 or orangefs , your input: $(BOOT_PART_FS_TYPE) )
# endif 


#检查USING_GRUB_CHAINLOADER是否输入正确(仅支持true,false)
ifeq ($(USING_GRUB_CHAINLOADER),true)
ifeq ($(GRUB_PART_NUM),$(ROOT_FS_PART_NUM))
	$(error  GRUB_PART_NUM equals ROOT_FS_PART_NUM! This is not allowed when using GRUB)
endif
else ifeq ($(USING_GRUB_CHAINLOADER),false)
else
	$(error USING_GRUB_CHAINLOADER input error! Correct vlaue: true or false ,your input:$(USING_GRUB_CHAINLOADER) )
endif 
#输出关键参数
	$(info MACHINE_TYPE=$(MACHINE_TYPE))
	$(info INS_DEV=$(INS_DEV))
	$(info BOOT_PART=$(BOOT_PART))
	$(info ROOT_FS_PART=$(ROOT_FS_PART))
	$(info BOOT_PART_FS_TYPE=$(BOOT_PART_FS_TYPE))
	$(info ROOT_PART_FS_TYPE=$(ROOT_PART_FS_TYPE))
ifeq ($(USING_GRUB_CHAINLOADER),true)
	$(info GRUB_INSTALL_PART=$(GRUB_INSTALL_PART))
	$(info GRUB_CONFIG=$(GRUB_CONFIG))
endif


build_img:
	@if [[ "$(MACHINE_TYPE)" == "virtual" ]]; then \
		rm -f $(WRITE_DISK) && \
		cp ./hd/$(BOOT_IMG) ./$(WRITE_DISK); \
	fi

# added by mingxuan 2020-10-22
build_grub:
	@if [[ "$(MACHINE_TYPE)" == "virtual" ]]; then \
		sudo losetup -P $(INS_DEV) $(WRITE_DISK) && \
		sudo mkfs.vfat -F 32 $(GRUB_INSTALL_PART); \
	else \
		sudo mkfs.vfat -F 32 $(GRUB_INSTALL_PART); \
	fi
	
	@if [[ "$(USING_GRUB_CHAINLOADER)" == "true" ]]; then \
		sudo mount  $(GRUB_INSTALL_PART) iso && \
		sudo grub-install --boot-directory=./iso  --modules="part_msdos"  $(INS_DEV) &&\
		sudo cp os/boot/mbr/grub/$(GRUB_CONFIG) iso/grub/grub.cfg &&\
		sudo umount iso ;\
	fi

	@if [[ "$(MACHINE_TYPE)" == "virtual" ]]; then \
		sudo losetup -d $(INS_DEV); \
	fi

# added by mingxuan 2019-5-17
build_mbr:
	@if [[ "$(USING_GRUB_CHAINLOADER)" != "true" ]]; then \
		sudo dd if=os/boot/mbr/mbr.bin of=$(WRITE_DISK) bs=1 count=446 conv=notrunc ; \
	fi

build_fs:
	@if [[ "$(MACHINE_TYPE)" == "virtual" ]]; then \
		sudo losetup -D $(INS_DEV); \
		sudo losetup -P $(INS_DEV) $(WRITE_DISK); \
	fi

	sudo $(BOOT_PART_FS_MAKER) $(BOOT_PART_FS_MAKE_FLAG) $(BOOT_PART)

# FAT322规范规定第90~512个字节(共423个字节)是引导程序 # added by mingxuan 2020-10-5
	sudo dd if=os/boot/mbr/$(BOOT) of=$(BOOT_PART) bs=1 count=$(BOOT_SIZE) seek=$(BOOT_SEEK) conv=notrunc

#orangefs有专用的cp 不需要挂载到linux目录下，其他如fat32需要挂载
	@if [[ "$(BOOT_PART_FS_TYPE)" != "orangefs" ]]; then \
		sudo mount $(BOOT_PART) $(BOOT_PART_MOUNTPOINT) ; \
	fi

	sudo $(CP) $(CP_FLAG) os/boot/mbr/loader.bin $(BOOT_PART_MOUNTPOINT)/loader.bin
	sudo $(CP) $(CP_FLAG) kernel.bin $(BOOT_PART_MOUNTPOINT)/kernel.bin

#初始化根文件系统
	sudo $(ROOT_FS_MAKER) $(ROOT_FS_MAKE_FLAG) $(ROOT_FS_PART)
	@if [[ "$(ROOT_PART_FS_TYPE)" != "orangefs" ]]; then \
		sudo mount $(ROOT_FS_PART) $(ROOT_MOUNTPOINT);	\
	fi
# 在启动盘放置init.bin启动文件
	sudo $(ROOT_CP)  user/init/init.bin $(ROOT_MOUNTPOINT)/init.bin

# 在此处添加用户程序的文件
	$(foreach USER_TEST,$(ORANGESUSER),\
		sudo $(ROOT_CP)  $(USER_TEST) $(ROOT_MOUNTPOINT)/$(notdir $(USER_TEST));\
	)
# 	sudo $(ORANGE_CP)  user/user/shell_0.bin $(ROOT_FS_PART)/shell_0.bin
# 	sudo $(ORANGE_CP)  user/user/shell_1.bin $(ROOT_FS_PART)/shell_1.bin
# 	sudo $(ORANGE_CP)  user/user/shell_2.bin $(ROOT_FS_PART)/shell_2.bin

# 	sudo $(ORANGE_CP)  user/user/test_0.bin $(ROOT_FS_PART)/test_0.bin
# 	sudo $(ORANGE_CP)  user/user/test_1.bin $(ROOT_FS_PART)/test_1.bin
# 	sudo $(ORANGE_CP)  user/user/test_2.bin $(ROOT_FS_PART)/test_2.bin	# added by mingxuan 2021-2-28
# 	sudo $(ORANGE_CP)  user/user/test_3.bin $(ROOT_FS_PART)/test_3.bin
# 	sudo $(ORANGE_CP)  user/user/test_4.bin $(ROOT_FS_PART)/test_4.bin
# 	sudo $(ORANGE_CP)  user/user/test_5.bin $(ROOT_FS_PART)/test_5.bin
# 	sudo $(ORANGE_CP)  user/user/test_6.bin $(ROOT_FS_PART)/test_6.bin
# 	sudo $(ORANGE_CP)  user/user/test_7.bin $(ROOT_FS_PART)/test_7.bin

# 	sudo $(ORANGE_CP)  user/user/ptest1.bin $(ROOT_FS_PART)/ptest1.bin
# 	sudo $(ORANGE_CP)  user/user/ptest2.bin $(ROOT_FS_PART)/ptest2.bin
# 	sudo $(ORANGE_CP)  user/user/ptest3.bin $(ROOT_FS_PART)/ptest3.bin
# 	sudo $(ORANGE_CP)  user/user/ptest4.bin $(ROOT_FS_PART)/ptest4.bin
# 	sudo $(ORANGE_CP)  user/user/ptest5.bin $(ROOT_FS_PART)/ptest5.bin
# 	sudo $(ORANGE_CP)  user/user/ptest6.bin $(ROOT_FS_PART)/ptest6.bin
# 	sudo $(ORANGE_CP)  user/user/ptest7.bin $(ROOT_FS_PART)/ptest7.bin
# 	sudo $(ORANGE_CP)  user/user/ptest8.bin $(ROOT_FS_PART)/ptest8.bin
# 	sudo $(ORANGE_CP)  user/user/ptest9.bin $(ROOT_FS_PART)/ptest9.bin
# 	sudo $(ORANGE_CP)  user/user/ptest10.bin $(ROOT_FS_PART)/ptest10.bin
# 	sudo $(ORANGE_CP)  user/user/ptest11.bin $(ROOT_FS_PART)/ptest11.bin
# 	sudo $(ORANGE_CP)  user/user/ptest12.bin $(ROOT_FS_PART)/ptest12.bin
# 	sudo $(ORANGE_CP)  user/user/ptest13.bin $(ROOT_FS_PART)/ptest13.bin
# 	sudo $(ORANGE_CP)  user/user/fstest.bin $(ROOT_FS_PART)/fstest.bin
# # added by dzq 2023-4-12
# 	sudo $(ORANGE_CP)  user/user/t_exit01.bin $(ROOT_FS_PART)/t_exit01.bin


# # added by dzq
# 	sudo $(ORANGE_CP)  user/user/t_pthr01.bin $(ROOT_FS_PART)/t_pthr01.bin
# 	sudo $(ORANGE_CP)  user/user/t_pthr02.bin $(ROOT_FS_PART)/t_pthr02.bin
# 	sudo $(ORANGE_CP)  user/user/t_pthr03.bin $(ROOT_FS_PART)/t_pthr03.bin

# # added by yingchi 2022.01.05
# #	 sudo $(CP) $(CP_FLAG) user/user/myTest.bin $(BOOT_PART_MOUNTPOINT)/myTest.bin
	
# 	# added by mingxuan 2021-2-28
# 	sudo $(ORANGE_CP)  user/user/sig_0.bin $(ROOT_FS_PART)/sig_0.bin
# 	sudo $(ORANGE_CP)  user/user/sig_1.bin $(ROOT_FS_PART)/sig_1.bin

# 	sudo $(ORANGE_CP)  user/user/sema1.bin $(ROOT_FS_PART)/sema1.bin
# 	sudo $(ORANGE_CP)  user/user/rwtest1.bin $(ROOT_FS_PART)/rwtest1.bin
	@if [[ "$(ROOT_PART_FS_TYPE)" != "orangefs" ]]; then \
		sudo umount $(ROOT_MOUNTPOINT) ; \
	fi

	@if [[ "$(BOOT_PART_FS_TYPE)" != "orangefs" ]]; then \
		sudo umount $(BOOT_PART_MOUNTPOINT) ; \
	fi

	@if [[ "$(MACHINE_TYPE)" == "virtual" ]]; then \
		sudo losetup -d $(INS_DEV); \
	fi


# buildimg :
# 	dd if=os/boot/floppy/boot.bin of=a.img bs=512 count=1 conv=notrunc	# modified by mingxuan 2019-5-17
# 	sudo mount -o loop a.img /mnt/floppy/
# 	sudo cp -fv os/boot/floppy/loader.bin /mnt/floppy/ # modified by mingxuan 2019-5-17
# 	sudo cp -fv kernel.bin /mnt/floppy
# 	sudo cp -fv os/init/init.bin /mnt/floppy
# 	sudo umount /mnt/floppy

# generate tags file. added by xw, 19/1/2
tags :
	# ctags -R

archive :
	git archive --prefix=
