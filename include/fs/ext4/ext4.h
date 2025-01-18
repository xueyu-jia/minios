#pragma once

#include <fs/fs.h>
#include <klib/stdint.h>

#define EXT4_N_BLOCKS 15

#define EXT_ROOT_INO 2

#define EXT4_MIN_BLOCK_SIZE 1024
#define EXT4_LINK_MAX 65536

#define EXT4_EXTENTS_FL 0x80000

typedef u16 ext4_group_t;
typedef u32 ext4_lblk_t;

// TODO： 其他必要常量的定义 需要用到时可以在这里定义

// ext4 超级块私有数据
// 先定义这些 手册里面成员太多了 有些成员不确定能用上 用上的时候再补吧
// 理论上靠超级块之后的 GDT 就可以获得每个块组的信息了
struct ext4_sb_info {
    u32 s_inodes_count;         // 文件系统中的总inode数量
    u32 s_blocks_count_lo;      // 文件系统总块数
    u32 s_r_blocks_count_lo;    // 只有超级用户可以分配的块数 通常用于保留块
    u32 s_free_blocks_count_lo; // 文件系统剩余的块数
    u32 s_free_inodes_count;    // 文件系统剩余的inode数量
    u32 s_first_data_block;     // 第一个数据块块号 4KB 的块 一般为0
    u32 s_log_block_size;       // 块大小的对数值 2 ^ (10 + s_log_block_size).
    u32 s_log_cluster_size;     // 没有启用bigalloc特性 那么值和s_log_block_size一样
    u32 s_blocks_per_group;     // 每个块组的块数
    u32 s_clusters_per_group; // 每个组的簇数 没有启用bigalloc 值和s_blocks_per_group 一样
    u32 s_inodes_per_group; // 每个块组的 inode 数
    u32 s_mtime;            // 文件系统的挂载时间  从1970年1月1日的秒数
    u32 s_wtime;            // 文件系统最后写入的时间

    u16 s_inode_size; // inode 的大小
    u16 s_desc_size;  // 块组描述符的大小

    u16 s_mnt_count;     // 自上次文件系统检查fsck以来的挂载次数
    u16 s_max_mnt_count; // 需要进行文件系统检查的最大挂载次数
    /*0x38*/
    u16 s_magic; // 魔数 用于识别文件系统 有效值为0xEF53
    u16 s_block_group_nr;
    u32 s_first_ino; // 文件系统中第一个非保留 inode 的编号
};

// ext4 inode 定义
struct ext4_inode {
    /* data */
    u16 i_mode;        /* 文件模式 */
    u16 i_uid;         /* 所有者 UID 的低 16 位 */
    u32 i_size_lo;     /* 文件大小（字节） */
    u32 i_atime;       /* 访问时间 */
    u32 i_ctime;       /* inode 修改时间 */
    u32 i_mtime;       /* 修改时间 */
    u32 i_dtime;       /* 删除时间 */
    u16 i_gid;         /* 用户组 ID 的低 16 位 */
    u16 i_links_count; /* 链接计数 */
    u32 i_blocks_lo;   /* 块计数 */
    u32 i_flags;       /* 文件标志 */
    union {
        struct {
            u32 l_i_version; /* Linux 版本 */
        } linux1;
        struct {
            u32 h_i_translator; /* Hurd 1 转换器 */
        } hurd1;
        struct {
            u32 m_i_reserved1; /* Masix 1 保留字段 */
        } masix1;
    } osd1;                     /* 操作系统依赖的字段 1 */
    u32 i_block[EXT4_N_BLOCKS]; /* 块指针 */
    u32 i_generation;           /* 文件版本（用于 NFS） */
    u32 i_file_acl_lo;          /* 文件 ACL */
    u32 i_size_high;            /* 文件大小高位 */
    u32 i_obso_faddr;           /* 废弃的碎片地址 */
    union {
        struct {
            u16 l_i_blocks_high;   /* 高位块数 */
            u16 l_i_file_acl_high; /* 高位文件 ACL */
            u16 l_i_uid_high;      /* UID 高位 */
            u16 l_i_gid_high;      /* GID 高位 */
            u32 l_i_reserved2;     /* 保留字段 */
        } linux2;
        struct {
            u16 h_i_reserved1; /* 已废弃的碎片字段 */
            u16 h_i_mode_high; /* 高位文件模式 */
            u16 h_i_uid_high;  /* UID 高位 */
            u16 h_i_gid_high;  /* GID 高位 */
            u32 h_i_author;    /* Hurd 作者 */
        } hurd2;
        struct {
            u16 h_i_reserved1;     /* 已废弃的碎片字段 */
            u16 m_i_file_acl_high; /* 高位文件 ACL */
            u32 m_i_reserved2[2];  /* Masix 2 保留字段 */
        } masix2;
    } osd2;             /* 操作系统依赖的字段 2 */
    u16 i_extra_isize;  /* 额外的 inode 大小 */
    u16 i_pad1;         /* 填充字段 */
    u32 i_ctime_extra;  /* 额外的改变时间（nsec << 2 | epoch） */
    u32 i_mtime_extra;  /* 额外的修改时间（nsec << 2 | epoch） */
    u32 i_atime_extra;  /* 额外的访问时间（nsec << 2 | epoch） */
    u32 i_crtime;       /* 文件创建时间 */
    u32 i_crtime_extra; /* 额外的创建时间（nsec << 2 | epoch） */
    u32 i_version_hi;   /* 64 位版本的高 32 位 */
};

// ext4 inode 私有数据
struct ext4_inode_info {
    u32 i_block[EXT4_N_BLOCKS]; /* 块指针 */
    u32 i_bg_block;             // 所在的块组号
    u32 i_dtime;                /* 删除时间 */
    u32 i_blocks_lo;            /* 块计数 */
    u32 i_size;        /* ext4 inode 中的 i_size， 可能与 vfs inode 中的 i_size 不同 */
    u16 i_links_count; /* 链接计数 */
};

// 组描述符结构体
struct ext4_group_desc {
    u32 bg_block_bitmap_lo;      // 0x0: 块位图位置的低 32 位
    u32 bg_inode_bitmap_lo;      // 0x4: inode 位图位置的低 32 位
    u32 bg_inode_table_lo;       // 0x8: inode 表位置的低 32 位
    u16 bg_free_blocks_count_lo; // 0xC: 空闲块计数的低 16 位
    u16 bg_free_inodes_count_lo; // 0xE: 空闲 inode 计数的低 16 位
    u16 bg_used_dirs_count_lo;   // 0x10: 目录计数的低 16 位
    u16 bg_flags;                // 0x12: 块组标志
    u32 bg_exclude_bitmap_lo;    // 0x14: 快照排除位图位置的低 32 位
    u16 bg_block_bitmap_csum_lo; // 0x18: 块位图校验和的低 16 位
    u16 bg_inode_bitmap_csum_lo; // 0x1A: inode 位图校验和的低 16 位
    u16 bg_itable_unused_lo;     // 0x1C: 未使用 inode 数量的低 16 位
    u16 bg_checksum;             // 0x1E: 组描述符校验和
    u32 bg_block_bitmap_hi;      // 0x20: 块位图位置的高 32 位
    u32 bg_inode_bitmap_hi;      // 0x24: inode 位图位置的高 32 位
    u32 bg_inode_table_hi;       // 0x28: inode 表位置的高 32 位
    u16 bg_free_blocks_count_hi; // 0x2C: 空闲块计数的高 16 位
    u16 bg_free_inodes_count_hi; // 0x2E: 空闲 inode 计数的高 16 位
    u16 bg_used_dirs_count_hi;   // 0x30: 目录计数的高 16 位
    u16 bg_itable_unused_hi;     // 0x32: 未使用 inode 数量的高 16 位
    u32 bg_exclude_bitmap_hi;    // 0x34: 快照排除位图位置的高 32 位
    u16 bg_block_bitmap_csum_hi; // 0x38: 块位图校验和的高 16 位
    u16 bg_inode_bitmap_csum_hi; // 0x3A: inode 位图校验和的高 16 位
    u32 bg_reserved;             // 0x3C: 填充，确保结构体大小为 64 字节
};

// extent 头部结构体的定义
struct ext4_extent_header {
    u16 eh_magic;   /* f30a */
    u16 eh_entries; // 目前有效的 ext4_extent_idx或ext4_extent个数
    u16 eh_max;     // 最多能保存的 ext4_extent_idx或ext4_extent个数
    u16 eh_depth;   // eh_depth = 0 则后面的都是 ext4_extent
    u32 eh_generation;
};

// ext4_extent_idx 定义
struct ext4_extent_idx {
    u32 ei_block; // 起始逻辑块地址
    // 由ei_leaf_lo和ei_leaf_hi组成起始逻辑块地址对应的物理块地址
    u32 ei_leaf_lo;
    u16 ei_leaf_hi;
    u16 ei_unused;
};

// ext4_extent定义
struct ext4_extent {
    u32 ee_block; // extent的起始逻辑块地址
    u16 ee_len;   // extent的长度

    u16 ee_start_hi; // 物理块号高16位
    u32 ee_start_lo; // 物理块号低32位
};

// ext4_dir_entry_2 定义
struct ext4_dir_entry_2 {
    u32 inode;    // 该目录项的inode的值
    u16 rec_len;  // 该目录项的长度
    u8 name_len;  // 名字的长度
    u8 file_type; // 文件类型
    char* name;
};

/*
 * Hints to ext4_read_dirblock regarding whether we expect a directory
 * block being read to be an index block, or a block containing
 * directory entries (and if the latter, whether it was found via a
 * logical block in an htree index block).  This is used to control
 * what sort of sanity checkinig ext4_read_dirblock() will do on the
 * directory block read from the storage device.  EITHER will means
 * the caller doesn't know what kind of directory block will be read,
 * so no specific verification will be done.
 */
typedef enum { EITHER, INDEX, DIRENT, DIRENT_HTREE } dirblock_type_t;

#define EXT_SB(sb) ((struct ext4_sb_info*)((sb)->sb_private))
#define EXT_INODE(inode) ((struct ext4_inode_info*)((inode)->i_private))

// #ifdef __KERNEL__
// # define EXT4_BLOCK_SIZE(s)		((s)->s_blocksize)
// #else
#define EXT4_BLOCK_SIZE(s) (EXT4_MIN_BLOCK_SIZE << EXT_SB(s)->s_log_block_size)
// #endif

#define EXT4_MIN_DESC_SIZE 32
#define EXT4_MIN_DESC_SIZE_64BIT 64
#define EXT4_MAX_DESC_SIZE EXT4_MIN_BLOCK_SIZE
#define EXT4_DESC_SIZE(s) (EXT_SB(s)->s_desc_size)
// #ifdef __KERNEL__
// # define EXT4_BLOCKS_PER_GROUP(s)	(EXT_SB(s)->s_blocks_per_group)
// # define EXT4_CLUSTERS_PER_GROUP(s)	(EXT_SB(s)->s_clusters_per_group)
// # define EXT4_DESC_PER_BLOCK(s)		(EXT_SB(s)->s_desc_per_block)
// # define EXT4_INODES_PER_GROUP(s)	(EXT_SB(s)->s_inodes_per_group)
// # define EXT4_DESC_PER_BLOCK_BITS(s)	(EXT_SB(s)->s_desc_per_block_bits)
// #else
#define EXT4_BLOCKS_PER_GROUP(s) (EXT_SB(s)->s_blocks_per_group)
#define EXT4_DESC_PER_BLOCK(s) (EXT4_BLOCK_SIZE(s) / EXT4_DESC_SIZE(s))
#define EXT4_INODES_PER_GROUP(s) (EXT_SB(s)->s_inodes_per_group)
// #endif
#define EXT4_TOTAL_GROUP_COUNT(s) ((EXT_SB(s)->s_blocks_count_lo / EXT4_BLOCKS_PER_GROUP(s)) + 1)
#define EXT4_NEXT_DIR_ENTRY_2(de) ((struct ext4_dir_entry_2*)(((char*)(de)) + (de)->rec_len))

extern struct superblock_operations ext4_sb_ops;
extern struct inode_operations ext4_inode_ops;
extern struct dentry_operations ext4_dentry_ops;
extern struct file_operations ext4_file_ops;
