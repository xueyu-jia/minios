#########################
# Makefile for Orange'S #
#########################

# added by mingxuan 2020-9-12
# Offset of os_boot in hd
# 活动分区所在的扇区号
# OSBOOT_SEC = 4096
# 活动分区所在的扇区号对应的字节数
# OSBOOT_OFFSET = $(OSBOOT_SEC)*512
OSBOOT_OFFSET = 1048576
# FAT32规范规定os_boot的前89个字节是FAT32的配置信息
# OSBOOT_START_OFFSET = OSBOOT_OFFSET + 90
OSBOOT_START_OFFSET = 1048666 # for test12.img

# added by mingxuan 2020-10-29
# Offset of fs in hd
# 文件系统标志所在扇区号 = 文件系统所在分区的第1个扇区 + 1
# ORANGE_FS_SEC = 6144 + 1 = 6145
# 文件系统标志所在扇区 = $(ORANGE_FS_SEC)*512
ORANGE_FS_START_OFFSET = 3146240
# FAT32_FS_SEC = 53248 + 1 = 53249
# 文件系统标志所在扇区 = $(ORANGE_FS_SEC)*512
FAT32_FS_START_OFFSET = 27263488


# Programs, flags, etc.
ASM		= nasm
DASM	= ndisasm
CC		= gcc
LD		= ld
AR		= ar

# added by mingxuan 2020-10-22
MKFS = fs_flags/orange_flag.bin fs_flags/fat32_flag.bin

include ./os/Makefile
include	./user/Makefile

# All Phony Targets

.PHONY : everything image all buildimg

# Default starting position
nop :
	@echo "why not \`make image' huh? :)"

everything : $(MKFS)

all : everything

image : everything buildimg_mbr build_fs # modified by mingxuan 2020-12-8

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
	cp ./hd/test1.img ./b.img	# added by mingxuan 2020-10-5

	dd if=os/boot/mbr/mbr.bin of=b.img bs=1 count=446 conv=notrunc

	sudo losetup -P /dev/loop0 b.img

	sudo mkfs.vfat -F 32 -s8 /dev/loop0p1	# modified by mingxuan 2021-2-28

	# FAT322规范规定第90~512个字节(共423个字节)是引导程序 # added by mingxuan 2020-10-5
	dd if=os/boot/mbr/boot.bin of=b.img bs=1 count=420 seek=$(OSBOOT_START_OFFSET) conv=notrunc

	sudo mount /dev/loop0p1 iso/

	sudo cp -fv os/boot/mbr/loader.bin iso/
	sudo cp -fv kernel.bin iso/

	# 在启动盘放置init.bin启动文件
	sudo cp -fv user/init/init.bin iso/

	# 在此处添加用户程序的文件
	sudo cp -fv user/user/shell_0.bin iso/
	sudo cp -fv user/user/shell_1.bin iso/
	sudo cp -fv user/user/shell_2.bin iso/

	sudo cp -fv user/user/test_0.bin iso/
	sudo cp -fv user/user/test_1.bin iso/
	sudo cp -fv user/user/test_2.bin iso/	# added by mingxuan 2021-2-28
	sudo cp -fv user/user/test_3.bin iso/
	sudo cp -fv user/user/test_4.bin iso/
	sudo cp -fv user/user/test_5.bin iso/

	# added by yingchi 2022.01.05
	# sudo cp -fv user/user/myTest.bin iso/
	
	# added by mingxuan 2021-2-28
	sudo cp -fv user/user/sig_0.bin iso/
	sudo cp -fv user/user/sig_1.bin iso/


	sudo umount iso/

	sudo losetup -d /dev/loop0

# added by mingxuan 2020-10-22
build_fs:
	dd if=fs_flags/orange_flag.bin of=b.img bs=1 count=1 seek=$(ORANGE_FS_START_OFFSET) conv=notrunc

	sudo losetup -P /dev/loop0 b.img
	sudo mkfs.vfat -F 32 /dev/loop0p6
	sudo losetup -d /dev/loop0

#	cp ./b.img ./user/user/b.img	# for debug, added by mingxuan 2021-8-8

# mkfs_orange
fs_flags/orange_flag.bin : fs_flags/orange_flag.asm
	$(ASM) -o $@ $<

# mkfs_fat32
fs_flags/fat32_flag.bin : fs_flags/fat32_flag.asm
	$(ASM) -o $@ $<

# generate tags file. added by xw, 19/1/2
tags :
	ctags -R

archive :
	git archive --prefix=






