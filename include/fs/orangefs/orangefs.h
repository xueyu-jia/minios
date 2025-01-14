#pragma once

#include <fs/fs.h>
#include <klib/stdint.h>

/**
 * @def   MAGIC_V1
 * @brief Magic number of FS v1.0
 */
#define MAGIC_V1 0x111
// #define	NR_DEFAULT_FILE_SECTS	2048 /* 2048 * 512 = 1MB */
// add by sundong 2023.5.26
#define NR_DEFAULT_FILE_BLOCKS 1024

// mark 路径最大长度
// #define	MAX_PATH	128 common macro in fs_const.h
#define MAX_FILENAME_LEN 12

/* Error types of dir option*/
#define DIR_PATH_INEXISTE -1
#define DIR_PATH_REPEATED -2
#define DIR_FILE_OCCUPIYED -3

struct orange_sb_info {
    u32 magic;             /**< Magic number */
    u32 nr_inodes;         /**< How many inodes */
    u32 nr_blocks;         /**< How many blocks */
    u32 nr_imap_blocks;    /**< How many inode-map blocks */
    u32 nr_smap_blocks;    /**< How many sector-map blocks */
    u32 n_1st_block;       /**< Number of the 1st data block */
    u32 nr_inode_blocks;   /**< How many inode blocks */
    u32 root_inode;        /**< Inode nr of root directory */
    u32 inode_size;        /**< INODE_SIZE */
    u32 inode_isize_off;   /**< Offset of `struct inode::i_size' */
    u32 inode_start_off;   /**< Offset of `struct inode::i_start_sect' */
    u32 dir_ent_size;      /**< DIR_ENTRY_SIZE */
    u32 dir_ent_inode_off; /**< Offset of `struct dir_entry::inode_nr' */
    u32 dir_ent_fname_off; /**< Offset of `struct dir_entry::name' */
};

/**
 * @def   SUPER_BLOCK_SIZE
 * @brief The size of super block \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define SUPER_BLOCK_SIZE 64 // modified by mingxuan 2020-10-30
#define INVALID_INODE 0
#define ROOT_INODE 1
#define NR_ORANGE_INODE 64 /* FIXME */
/**
 * @struct inode
 * @brief  i-node
 *
 * The \c start_sect and\c nr_sects locate the file in the device,
 * and the size show how many bytes is used.
 * If <tt> size < (nr_sects * SECTOR_SIZE) </tt>, the rest bytes
 * are wasted and reserved for later writing.
 *
 * \b NOTE: Remember to change INODE_SIZE if the members are changed
 */
struct orange_inode {
    u32 i_mode;        /**< Accsess mode */
    u32 i_size;        /**< File size */
    u32 i_start_block; /**< The first block of the data */
    u32 i_nr_blocks;   /**< How many blocks the file occupies */
    u8 _unused[15];    /**< Stuff for alignment */
    u8 i_mnt_index;    /**the index in mnt_table when the inode is mountpoint*/
    /* the following items are only present in memory */
    int i_dev;
    int i_cnt; /**< How many procs share this inode  */
    int i_num; /**< inode nr.  */
};
extern struct superblock_operations orange_sb_ops;
extern struct inode_operations orange_inode_ops;
extern struct file_operations orange_file_ops;

struct orange_inode_info {
    u32 i_start_block; /**< The first block of the data */
    u32 i_nr_blocks;   /**< How many blocks the file occupies */
};

#define ORANGE_SB(sb) ((struct orange_sb_info*)((sb)->sb_private))
#define ORANGE_INODE(inode) ((struct orange_inode_info*)((inode)->i_private))

/**
 * @def   INODE_SIZE
 * @brief The size of i-node stored \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define INODE_SIZE 32

/**
 * @struct dir_entry
 * @brief  Directory Entry
 */
struct dir_entry {
    int inode_nr;                /**< inode nr. */
    char name[MAX_FILENAME_LEN]; /**< Filename */
};

/**
 * @def   DIR_ENTRY_SIZE
 * @brief The size of directory entry in the device.
 *
 * It is as same as the size in memory.
 */
#define DIR_ENTRY_SIZE sizeof(struct dir_entry)
