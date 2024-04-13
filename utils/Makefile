CC	=	gcc
FUSE_CFLAGS = -Wall  -I/usr/include/fuse3 -lfuse3
# if you cant build normally, please ensure fuse3 installed and check flags by `pkg-config fuse3 --cflags --libs --static`
ORANGEFS_BASE = orangefs.c orangefs_disk.c

.PHONY: all 

all:orangefs_mount orangefs_mkfs

orangefs_mount: orangefs_mount.c $(ORANGEFS_BASE)
	$(CC) $^ $(FUSE_CFLAGS) -o $@

orangefs_mkfs: orangefs_mkfs.c $(ORANGEFS_BASE)
	$(CC) $^ -o $@
