#pragma once

#include <fs/blk_types.h>
#include <uapi/minios/fs.h>

enum {
    FS_TYPE_DEV,
    FS_TYPE_ORANGE,
    FS_TYPE_FAT32,
    FS_TYPE_EXT4,
    NR_FS_TYPE,
    FS_TYPE_NONE = 0xffffffff,
};

#define MAX_DNAME_LEN 32

#define NR_FILE_DESC 128
#define NR_INODE 512
#define NR_SUPER_BLOCK 16

#define is_special(m) \
    ((((m) & I_TYPE_MASK) == I_BLOCK_SPECIAL) || (((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))
#define inode_allow_lseek(type) ((type) != I_CHAR_SPECIAL && (type) != I_NAMED_PIPE)
