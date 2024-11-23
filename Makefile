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
BOOT_PART_FS_TYPE=orangefs
ROOT_PART_FS_TYPE=fat32
#grub的配置文件,提供了一个默认的grub配置文件，配置为从第1块硬盘分区1引导
GRUB_CONFIG=boot_from_part1.cfg
#从虚拟机机启动时使用的虚拟镜像，一般由由 makefile 生成
BOOT_IMG=hd/fat32_boot.img
###################################################################

# 生成的内核文件
ORANGESKERNEL = kernel.bin

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

ORANGE_MOUNT=utils/orangefs_mount
ORANGE_MAKER=utils/orangefs_mkfs

ifeq ($(BOOT_PART_FS_TYPE),fat32)
	BOOT_PART_FS_MAKER = mkfs.vfat
	BOOT_PART_FS_MAKE_FLAG = -F 32 -s8
	BOOT_MOUNT = mount
	BOOT =boot.bin
	BOOT_SIZE = 420
# FAT32规范规定os_boot的前89个字节是FAT32的配置信息
	BOOT_SEEK=90
else ifeq ($(BOOT_PART_FS_TYPE),orangefs)
	BOOT_PART_FS_MAKER = $(ORANGE_MAKER)
	BOOT_PART_FS_MAKE_FLAG =
	BOOT_MOUNT = $(ORANGE_MOUNT)
	BOOT=orangefs_boot.bin
	BOOT_SIZE =512
# orangefs boot直接放在分区的0号扇区
	BOOT_SEEK=0
endif

# config root fs param
ifeq ($(ROOT_PART_FS_TYPE),fat32)
	ROOT_MOUNT = mount
	ROOT_FS_MAKER = mkfs.vfat
	ROOT_FS_MAKE_FLAG = -F 32 -s8
else ifeq ($(ROOT_PART_FS_TYPE),orangefs)
	ROOT_MOUNT = $(ORANGE_MOUNT)
	ROOT_FS_MAKER = $(ORANGE_MAKER)
	ROOT_FS_MAKE_FLAG =
endif
ROOT_MOUNTPOINT = root



# All Phony Targets
.PHONY : all clean install

all : everything
clean : realclean_os realclean_user clean_user clean-rtl
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


#检查BOOT_PART_FS_TYPE是否输入正确(仅支持fat32,orangefs)
ifeq ($(BOOT_PART_NUM),$(ROOT_FS_PART_NUM))
ifneq ($(BOOT_PART_FS_TYPE),$(ROOT_PART_FS_TYPE))
	$(error different fstype in same part)
endif
endif
# ifeq ($(BOOT_PART_FS_TYPE),fat32)
# #检查fat32作为启动分区文件系统时，启动分区是否和根文件系统分区重合 此条限制已去除
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

ifneq ($(findstring orangefs,$(BOOT_PART_FS_TYPE) $(ROOT_PART_FS_TYPE)),)
ifneq ($(wildcard $(ORANGE_MAKER) $(ORANGE_MOUNT)), $(ORANGE_MAKER) $(ORANGE_MOUNT))
#$(error orangefs needed but orangefs_mount and orangefs_mkfs not ready, please cd into "utils" and execute make)
	cd utils && make
endif
endif

# copy_user <mount_point> <src> <dst>
define copy_user
	sudo mkdir -p $(1)/$(dir $(3)); \
	sudo cp -f $(2) $(1)/$(3);      \
	echo "$(2) => /$(3)"
endef

build_img:
ifeq ($(MACHINE_TYPE),virtual)
	@mkdir -p $(dir $(BOOT_IMG))
	@\
	if [ ! -e "$(BOOT_IMG)" ]; then                         \
		dd if=/dev/zero of=$(BOOT_IMG) bs=512 count=204800; \
		sfdisk $(BOOT_IMG) < part.sfdisk;                   \
	fi
	@cp -f $(BOOT_IMG) $(WRITE_DISK)
endif

# added by mingxuan 2020-10-22
build_grub:
ifeq ($(MACHINE_TYPE),virtual)
	sudo losetup -P $(INS_DEV) $(WRITE_DISK)
endif
	sudo mkfs.vfat -F 32 $(GRUB_INSTALL_PART)
ifeq ($(USING_GRUB_CHAINLOADER),true)
	@\
	mount_point=$$(mktemp -d); \
	sudo mount $(GRUB_INSTALL_PART) $$mount_point && \
	sudo grub-install --target=i386-pc --boot-directory=./iso  --modules="part_msdos" $(INS_DEV) && \
	sudo cp os/boot/mbr/grub/$(GRUB_CONFIG) iso/grub/grub.cfg && \
	sudo umount $$mount_point
endif
ifeq ($(MACHINE_TYPE),virtual)
	sudo losetup -d $(INS_DEV)
endif

# added by mingxuan 2019-5-17
build_mbr:
ifneq ($(USING_GRUB_CHAINLOADER),true)
	sudo dd if=os/boot/mbr/mbr.bin of=$(WRITE_DISK) bs=1 count=446 conv=notrunc
endif

build_fs:
ifeq ($(MACHINE_TYPE),virtual)
	sudo losetup -D $(INS_DEV)
	sudo losetup -P $(INS_DEV) $(WRITE_DISK)
endif

	sudo $(BOOT_PART_FS_MAKER) $(BOOT_PART_FS_MAKE_FLAG) $(BOOT_PART)
	sudo dd if=os/boot/mbr/$(BOOT) of=$(BOOT_PART) bs=1 count=$(BOOT_SIZE) seek=$(BOOT_SEEK) conv=notrunc

	@\
	mount_point=$$(mktemp -d);                                 \
	sudo $(BOOT_MOUNT) $(BOOT_PART) $$mount_point;             \
	sudo cp os/boot/mbr/loader.bin $${mount_point}/loader.bin; \
	sudo cp $(ORANGESKERNEL) $$mount_point/kernel.bin;         \
	sudo umount $$mount_point

	sudo $(ROOT_FS_MAKER) $(ROOT_FS_MAKE_FLAG) $(ROOT_FS_PART)

	@\
	mount_point=$$(mktemp -d);                        \
	sudo $(ROOT_MOUNT) $(ROOT_FS_PART) $$mount_point; \
	$(foreach FILE,$(USER_IMG),$(call copy_user,$$mount_point,$(FILE),$(subst $(USER_SRC_DIR)/,,$(subst .bin,,$(FILE))));) \
	sudo umount $$mount_point

ifeq ($(MACHINE_TYPE),virtual)
	sudo losetup -d $(INS_DEV)
endif


# buildimg :
# 	dd if=os/boot/floppy/boot.bin of=a.img bs=512 count=1 conv=notrunc	# modified by mingxuan 2019-5-17
# 	sudo mount -o loop a.img /mnt/floppy/
# 	sudo cp -fv os/boot/floppy/loader.bin /mnt/floppy/ # modified by mingxuan 2019-5-17
# 	sudo cp -fv kernel.bin /mnt/floppy
# 	sudo cp -fv os/init/init.bin /mnt/floppy
# 	sudo umount /mnt/floppy

# generate tags file. added by xw, 19/1/2
tags :
	ctags -R

archive :
	git archive --prefix=

collect-rtl: $(ORANGESKERNEL)
	@mkdir -p rtl
	@find . -path ./rtl -prune -o -regex '.*\.[0-9]+r\.expand$$' -exec mv -f {} rtl \;

clean-rtl:
	@rm -rf rtl
