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

# added by mingxuan 2020-10-22
MKFS = fs_flags/orange_flag.bin fs_flags/fat32_flag.bin

#added by sundong 写镜像用,用losetup -f查看
FREE_LOOP =$(shell sudo losetup -f)
#added by sundong 2023.4.25
ROOT_FS_PART=$(FREE_LOOP)p2
GRUB_INSTALL_PART=$(FREE_LOOP)p5
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
# OSBOOT_START_OFFSET = OSBOOT_OFFSET + 90
OSBOOT_START_OFFSET = 1048666 # 2048*512+90
#OSBOOT_START_OFFSET=2097242 # for test12.img
BOOT_PART=$(FREE_LOOP)p1
BOOT_PART_MOUNTPOINT = iso
endif

ifeq ($(BOOT_PART_FS_TYPE),orangefs)
BOOT_PART_FS_MAKER = ./format
BOOT_PART_FS_MAKE_FLAG = 
CP = ./o_copy
CP_FLAG = 
BOOT=orangefs_boot.bin
BOOT_SIZE =512
# orangefs boot直接放在分区的0号扇区
# OSBOOT_START_OFFSET = OSBOOT_OFFSET 
OSBOOT_START_OFFSET =2097152 #4096*512
#p2分区格式化为orangefs
BOOT_PART=$(FREE_LOOP)p2
BOOT_PART_MOUNTPOINT =$(FREE_LOOP)p2
endif
 
include ./os/Makefile
include	./user/Makefile

# All Phony Targets

.PHONY : everything image all buildimg

# Default starting position
nop :
	@echo "why not \`make image' huh? :)"

#everything : $(MKFS)

all : everything

image : everything build_fs buildimg_mbr # 调整了下build_fs和buildimg_mbr的执行顺序 modified by sundong 2023.4.25 

# added by mingxuan 2020-10-22
build_fs :
#	dd if=fs_flags/orange_flag.bin of=b.img bs=1 count=1 seek=$(ORANGE_FS_START_OFFSET) conv=notrunc
#	dd if=os/boot/mbr/orangefs_boot.bin of=b.img bs=1 count=512 seek=$(OSBOOT_OFFSET) conv=notrunc

	sudo losetup -P $(FREE_LOOP) b.img


	sudo mkfs.vfat -F 32 $(FREE_LOOP)p5;

	@if [[ "$(USING_GRUB_CHAINLOADER)" == "true" ]]; then \
		sudo mount  $(GRUB_INSTALL_PART) iso && \
		sudo grub-install --boot-directory=./iso  --modules="part_msdos"  $(FREE_LOOP) &&\
		sudo cp os/boot/mbr/grub/grub.cfg iso/grub &&\
		sudo umount iso ;\
	fi


#	sudo ./o_list $(FREE_LOOP)p1
	sudo losetup -d $(FREE_LOOP)

#	cp ./b.img ./user/user/b.img	# for debug, added by mingxuan 2021-8-8

buildimg :
	dd if=os/boot/floppy/boot.bin of=a.img bs=512 count=1 conv=notrunc	# modified by mingxuan 2019-5-17

	sudo mount -o loop a.img /mnt/floppy/

	sudo cp -fv os/boot/floppy/loader.bin /mnt/floppy/ # modified by mingxuan 2019-5-17
	sudo cp -fv kernel.bin /mnt/floppy
	sudo cp -fv os/init/init.bin /mnt/floppy

	sudo umount /mnt/floppy

# added by mingxuan 2019-5-17
buildimg_mbr:
	rm -f b.img 				# added by mingxuan 2020-10-5
	cp ./hd/test2.img ./b.img	# added by mingxuan 2020-10-5
	@if [[ "$(USING_GRUB_CHAINLOADER)" != "true" ]]; then \
		dd if=os/boot/mbr/mbr.bin of=b.img bs=1 count=446 conv=notrunc ; \
	fi
#	dd if=os/boot/mbr/orangefs_boot.bin of=b.img bs=1 count=512 seek=$(OSBOOT_OFFSET) conv=notrunc

	sudo losetup -P $(FREE_LOOP) b.img
	sudo $(BOOT_PART_FS_MAKER) $(BOOT_PART_FS_MAKE_FLAG) $(BOOT_PART)  #$(FREE_LOOP)p2	# modified by mingxuan 2021-2-28

	# FAT322规范规定第90~512个字节(共423个字节)是引导程序 # added by mingxuan 2020-10-5
	dd if=os/boot/mbr/$(BOOT) of=b.img bs=1 count=$(BOOT_SIZE) seek=$(OSBOOT_START_OFFSET) conv=notrunc
#	dd if=os/boot/mbr/orangefs_boot.bin of=b.img bs=1 count=512 seek=$(OSBOOT_OFFSET) conv=notrunc
	#初始化根文件系统
	sudo ./format $(ROOT_FS_PART)
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

	# added by yingchi 2022.01.05
#	 sudo $(CP) $(CP_FLAG) user/user/myTest.bin $(BOOT_PART_MOUNTPOINT)/myTest.bin
	
	# added by mingxuan 2021-2-28
	sudo $(CP) $(CP_FLAG) user/user/sig_0.bin $(BOOT_PART_MOUNTPOINT)/sig_0.bin
	sudo $(CP) $(CP_FLAG) user/user/sig_1.bin $(BOOT_PART_MOUNTPOINT)/sig_1.bin

	@if [[ "$(BOOT_PART_FS_TYPE)" != "orangefs" ]]; then \
		sudo umount $(BOOT_PART_MOUNTPOINT) ; \
	fi

	sudo losetup -d $(FREE_LOOP)
#	dd if=os/boot/mbr/orangefs_boot.bin of=b.img bs=1 count=512 seek=$(OSBOOT_OFFSET) conv=notrunc



# mkfs_orange
#fs_flags/orange_flag.bin : fs_flags/orange_flag.asm
#	$(ASM) -o $@ $<

# mkfs_fat32
#fs_flags/fat32_flag.bin : fs_flags/fat32_flag.asm
#	$(ASM) -o $@ $<

# generate tags file. added by xw, 19/1/2
tags :
	# ctags -R

archive :
	git archive --prefix=






