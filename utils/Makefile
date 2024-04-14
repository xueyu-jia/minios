CC	=	gcc
FUSE_CFLAGS = -Wall  -I/usr/include/fuse3 -lfuse3
# if you cant build normally, please ensure fuse3 installed and check flags by `pkg-config fuse3 --cflags --libs --static`
ORANGEFS_BASE = orangefs.c orangefs_disk.c
ORANGEFS_UTILS = orangefs_mount orangefs_mkfs

DISINFO =  $(shell cat /etc/os-release)
# detect distribution
ifneq ($(findstring debian , $(DISINFO)),)
    PKGFIND=dpkg -l
    INSTALLER=apt install
    PKG=libfuse3-dev
else ifneq ($(findstring arch , $(DISINFO)),)
    PKGFIND=pacman -Q
    INSTALLER=pacman -S
    PKG=fuse3
endif


.PHONY: all 

all: utils_dep $(ORANGEFS_UTILS)

utils_dep:
ifneq ($(PKG),)
ifeq ($(shell $(PKGFIND) $(PKG)),)
	$(info utils needed lib not found, installing...)
	sudo $(INSTALLER) $(PKG)
else
	$(info check lib fuse3 ok)
endif
else
	$(warning unknown distribution, please ensure lib fuse3 installed)
endif

orangefs_mount: orangefs_mount.c $(ORANGEFS_BASE)
	$(CC) $^ $(FUSE_CFLAGS) -o $@

orangefs_mkfs: orangefs_mkfs.c $(ORANGEFS_BASE)
	$(CC) $^ -o $@
