#########################
# Makefile for Orange'S #
#########################
MACHINE_TYPE = virtual
SHELL := /bin/bash
# added by mingxuan 2020-9-12
# Offset of os_boot in hd
# 活动分区所在的扇区号
# OSBOOT_SEC = 4096
# 活动分区所在的扇区号对应的字节数
# OSBOOT_OFFSET = $(OSBOOT_SEC)*512
OSBOOT_OFFSET = 1048576


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

#added by sundong 写镜像用,用losetup -f查看
ifeq ($(MACHINE_TYPE), virtual)
	FREE_LOOP:=$(shell sudo losetup -f)
	ROOT_FS_PART=$(FREE_LOOP)p2
	GRUB_INSTALL_PART=$(FREE_LOOP)p5
	WRITE_DISK=b.img
else
	FREE_LOOP=/dev/sda
	ROOT_FS_PART=$(FREE_LOOP)2
	GRUB_INSTALL_PART=$(FREE_LOOP)5
	WRITE_DISK=/dev/sda
endif

#added by sundong 2023.3.21
#用于区分是使用grub chainloader
#可选值为 true  false
USING_GRUB_CHAINLOADER = false
#选择启动分区的文件系统格式，目前仅支持fat32和orangfs
BOOT_PART_FS_TYPE= fat32

ifeq ($(BOOT_PART_FS_TYPE),fat32)
	BOOT_PART_FS_MAKER = mkfs.vfat
	BOOT_PART_FS_MAKE_FLAG = -F 32 -s8
	CP = cp
	CP_FLAG = -fv
	BOOT =boot.bin
	BOOT_SIZE = 420
# FAT32规范规定os_boot的前89个字节是FAT32的配置信息
	BOOT_SEEK=90
	ifeq ($(MACHINE_TYPE), virtual)
		BOOT_PART=$(FREE_LOOP)p1
	else
		BOOT_PART=$(FREE_LOOP)1
	endif
	BOOT_PART_MOUNTPOINT = iso
	BOOT_IMG = fat32_boot.img
	GRUB_CONFIG=fat32_grub.cfg
	BOOT_FLAGS= -DFAT32_BOOT
else ifeq ($(BOOT_PART_FS_TYPE),orangefs)
	BOOT_PART_FS_MAKER = ./o_mkfs
	BOOT_PART_FS_MAKE_FLAG = 
	CP = ./o_copy
	BOOT=orangefs_boot.bin
	BOOT_SIZE =512
# orangefs boot直接放在分区的0号扇区
	BOOT_SEEK=0
#p2分区格式化为orangefs
	ifeq ($(MACHINE_TYPE), virtual)
		BOOT_PART=$(FREE_LOOP)p2
		BOOT_PART_MOUNTPOINT =$(FREE_LOOP)p2
	else
		BOOT_PART=$(FREE_LOOP)2
		BOOT_PART_MOUNTPOINT =$(FREE_LOOP)2
	endif
	
#镜像；不用文件系统作为启动分区时镜像是不同的
	BOOT_IMG=orangefs_boot.img
	GRUB_CONFIG=orangefs_grub.cfg
	BOOT_FLAGS= -DORANGE_BOOT
endif

# All Phony Targets
.PHONY : all clean install

all : everything
clean : realclean_os realclean_user clean_user
install : build_img build_grub build_mbr build_fs

include ./os/Makefile
include	./user/Makefile

build_img:
	@if [[ "$(MACHINE_TYPE)" == "virtual" ]]; then \
		rm -f $(WRITE_DISK) && \
		cp ./hd/$(BOOT_IMG) ./$(WRITE_DISK); \
	fi

# added by mingxuan 2020-10-22
build_grub:
	@if [[ "$(MACHINE_TYPE)" == "virtual" ]]; then \
		sudo losetup -P $(FREE_LOOP) $(WRITE_DISK) && \
		sudo mkfs.vfat -F 32 $(GRUB_INSTALL_PART); \
	else \
		sudo mkfs.vfat -F 32 $(GRUB_INSTALL_PART); \
	fi
	
	@if [[ "$(USING_GRUB_CHAINLOADER)" == "true" ]]; then \
		sudo mount  $(GRUB_INSTALL_PART) iso && \
		sudo grub-install --boot-directory=./iso  --modules="part_msdos"  $(FREE_LOOP) &&\
		sudo cp os/boot/mbr/grub/$(GRUB_CONFIG) iso/grub/grub.cfg &&\
		sudo umount iso ;\
	fi

	@if [[ "$(MACHINE_TYPE)" == "virtual" ]]; then \
		sudo losetup -d $(FREE_LOOP); \
	fi

# added by mingxuan 2019-5-17
build_mbr:
	@if [[ "$(USING_GRUB_CHAINLOADER)" != "true" ]]; then \
		dd if=os/boot/mbr/mbr.bin of=$(WRITE_DISK) bs=1 count=446 conv=notrunc ; \
	fi

build_fs:
	@if [[ "$(MACHINE_TYPE)" == "virtual" ]]; then \
		sudo losetup -d $(FREE_LOOP); \
		sudo losetup -P $(FREE_LOOP) $(WRITE_DISK); \
	fi

	sudo $(BOOT_PART_FS_MAKER) $(BOOT_PART_FS_MAKE_FLAG) $(BOOT_PART)

# FAT322规范规定第90~512个字节(共423个字节)是引导程序 # added by mingxuan 2020-10-5
	sudo dd if=os/boot/mbr/$(BOOT) of=$(BOOT_PART) bs=1 count=$(BOOT_SIZE) seek=$(BOOT_SEEK) conv=notrunc

#初始化根文件系统
	sudo ./o_mkfs $(ROOT_FS_PART)
#orangefs有专用的cp 不需要挂载到linux目录下，其他如fat32需要挂载
	@if [[ "$(BOOT_PART_FS_TYPE)" != "orangefs" ]]; then \
		sudo mount $(BOOT_PART) $(BOOT_PART_MOUNTPOINT) ; \
	fi

		sudo $(CP) $(CP_FLAG) os/boot/mbr/loader.bin $(BOOT_PART_MOUNTPOINT)/loader.bin
	sudo $(CP) $(CP_FLAG) kernel.bin $(BOOT_PART_MOUNTPOINT)/kernel.bin

# 在启动盘放置init.bin启动文件
	sudo $(CP) $(CP_FLAG) user/init/init.bin $(BOOT_PART_MOUNTPOINT)/init.bin

# 在此处添加用户程序的文件
	sudo $(CP) $(CP_FLAG) user/user/shell_0.bin $(BOOT_PART_MOUNTPOINT)/shell_0.bin
	sudo $(CP) $(CP_FLAG) user/user/shell_1.bin $(BOOT_PART_MOUNTPOINT)/shell_1.bin
	sudo $(CP) $(CP_FLAG) user/user/shell_2.bin $(BOOT_PART_MOUNTPOINT)/shell_2.bin

	sudo $(CP) $(CP_FLAG) user/user/test_0.bin $(BOOT_PART_MOUNTPOINT)/test_0.bin
	sudo $(CP) $(CP_FLAG) user/user/test_1.bin $(BOOT_PART_MOUNTPOINT)/test_1.bin
	sudo $(CP) $(CP_FLAG) user/user/test_2.bin $(BOOT_PART_MOUNTPOINT)/test_2.bin	# added by mingxuan 2021-2-28
	sudo $(CP) $(CP_FLAG) user/user/test_3.bin $(BOOT_PART_MOUNTPOINT)/test_3.bin
	sudo $(CP) $(CP_FLAG) user/user/test_4.bin $(BOOT_PART_MOUNTPOINT)/test_4.bin
	sudo $(CP) $(CP_FLAG) user/user/test_5.bin $(BOOT_PART_MOUNTPOINT)/test_5.bin
	sudo $(CP) $(CP_FLAG) user/user/test_6.bin $(BOOT_PART_MOUNTPOINT)/test_6.bin
	sudo $(CP) $(CP_FLAG) user/user/test_7.bin $(BOOT_PART_MOUNTPOINT)/test_7.bin

	sudo $(CP) $(CP_FLAG) user/user/ptest1.bin $(BOOT_PART_MOUNTPOINT)/ptest1.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest2.bin $(BOOT_PART_MOUNTPOINT)/ptest2.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest3.bin $(BOOT_PART_MOUNTPOINT)/ptest3.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest4.bin $(BOOT_PART_MOUNTPOINT)/ptest4.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest5.bin $(BOOT_PART_MOUNTPOINT)/ptest5.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest6.bin $(BOOT_PART_MOUNTPOINT)/ptest6.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest7.bin $(BOOT_PART_MOUNTPOINT)/ptest7.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest8.bin $(BOOT_PART_MOUNTPOINT)/ptest8.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest9.bin $(BOOT_PART_MOUNTPOINT)/ptest9.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest10.bin $(BOOT_PART_MOUNTPOINT)/ptest10.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest11.bin $(BOOT_PART_MOUNTPOINT)/ptest11.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest12.bin $(BOOT_PART_MOUNTPOINT)/ptest12.bin
	sudo $(CP) $(CP_FLAG) user/user/ptest13.bin $(BOOT_PART_MOUNTPOINT)/ptest13.bin
	sudo $(CP) $(CP_FLAG) user/user/fstest.bin $(BOOT_PART_MOUNTPOINT)/fstest.bin
# added by dzq 2023-4-12
	sudo $(CP) $(CP_FLAG) user/user/t_exit01.bin $(BOOT_PART_MOUNTPOINT)/t_exit01.bin


# added by dzq
	sudo $(CP) $(CP_FLAG) user/user/t_pthr01.bin $(BOOT_PART_MOUNTPOINT)/t_pthr01.bin
	sudo $(CP) $(CP_FLAG) user/user/t_pthr02.bin $(BOOT_PART_MOUNTPOINT)/t_pthr02.bin
	sudo $(CP) $(CP_FLAG) user/user/t_pthr03.bin $(BOOT_PART_MOUNTPOINT)/t_pthr03.bin

# added by yingchi 2022.01.05
#	 sudo $(CP) $(CP_FLAG) user/user/myTest.bin $(BOOT_PART_MOUNTPOINT)/myTest.bin
	
	# added by mingxuan 2021-2-28
	sudo $(CP) $(CP_FLAG) user/user/sig_0.bin $(BOOT_PART_MOUNTPOINT)/sig_0.bin
	sudo $(CP) $(CP_FLAG) user/user/sig_1.bin $(BOOT_PART_MOUNTPOINT)/sig_1.bin

	sudo $(CP) $(CP_FLAG) user/user/sema1.bin $(BOOT_PART_MOUNTPOINT)/sema1.bin
	sudo $(CP) $(CP_FLAG) user/user/rwtest1.bin $(BOOT_PART_MOUNTPOINT)/rwtest1.bin

	@if [[ "$(BOOT_PART_FS_TYPE)" != "orangefs" ]]; then \
		sudo umount $(BOOT_PART_MOUNTPOINT) ; \
	fi

	@if [[ "$(MACHINE_TYPE)" == "virtual" ]]; then \
		sudo losetup -d $(FREE_LOOP); \
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
