#pragma once

#define MAX_PATH 256

#define O_APPEND 32
#define O_TRUNC 16
#define O_DIRECTORY 8
#define O_CREAT 4
#define O_ACC 3
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2

#define S_IROTH 0x4
#define S_IWOTH 0x2
#define S_IXOTH 0x1
#define S_IXGRP 0x8
#define S_IWGRP 0x10
#define S_IRGRP 0x20
#define S_IXUSR 0x40
#define S_IWUSR 0x80
#define S_IRUSR 0x100

#define S_IFREG 0x8000
#define S_IFDIR 0x4000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define I_R 1
#define I_W 2
#define I_X 4
#define I_RW ((I_R) | (I_W))
#define I_RWX ((I_R) | (I_W) | (I_X))

#define I_TYPE_MASK 0170000
#define I_REGULAR 0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY 0040000
#define I_CHAR_SPECIAL 0020000
#define I_NAMED_PIPE 0010000
#define I_MOUNTPOINT 0110000
