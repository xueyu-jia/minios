#ifndef _ORANGEFS_H
#define _ORANGEFS_H
#include "type.h"

#define	MAX_FILENAME_LEN	12
#define	SUPER_BLOCK_SIZE	64		
/**
 * @def   INODE_SIZE
 * @brief The size of i-node stored \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define	INODE_SIZE	32
/**
 * @def   BLOCK_SIZE
 * @brief The size of BLOCK. It must be same as the block size in orange fs format programe.
 *
 */
 #define BLOCK_SIZE 4096
 /**
 * @def   SECT_SIZE
 * @brief The size of sector.
 */
 #define SECT_SIZE 512
 /**
 * @def   DIR_ENTRY_SIZE
 * @brief The size of directory entry in the device.
 *
 * It is as same as the size in memory.
 */

#define	DIR_ENTRY_SIZE	sizeof(struct dir_entry)

/**
 * @def   MAGIC_V1
 * @brief Magic number of FS v1.0
 */
#define	MAGIC_V1	0x111

/**
 * @struct super_block fs.h "include/fs.h"
 * @brief  The 2nd sector of the FS
 *
 * Remember to change SUPER_BLOCK_SIZE if the members are changed.
 */
typedef struct super_block {
	union {
    	struct {
			u32	magic;		  /**< Magic number */
			u32	nr_inodes;	  /**< How many inodes */
			u32	nr_sects;	  /**< How many sectors */
			u32	nr_imap_sects;	  /**< How many inode-map sectors */
			u32	nr_smap_sects;	  /**< How many sector-map sectors */
			u32	n_1st_sect;	  /**< Number of the 1st data sector */
			u32	nr_inode_sects;   /**< How many inode sectors */
			u32	root_inode;       /**< Inode nr of root directory */
			u32	inode_size;       /**< INODE_SIZE */
			u32	inode_isize_off;  /**< Offset of `struct inode::i_size' */
			u32	inode_start_off;  /**< Offset of `struct inode::i_start_sect' */
			u32	dir_ent_size;     /**< DIR_ENTRY_SIZE */
			u32	dir_ent_inode_off;/**< Offset of `struct dir_entry::inode_nr' */
			u32	dir_ent_fname_off;/**< Offset of `struct dir_entry::name' */
    	};
    	struct {
			u32 TotalSectors;//总扇区数，当载入磁盘时，才从DBR中读取。
			u16  Bytes_Per_Sector;//每个扇区的字节数，当载入磁盘时，才从DBR中读取。
			u8  Sectors_Per_Cluster;//每个簇的扇区数，当载入磁盘时，才从DBR中读取。
			u16  Reserved_Sector;//保留扇区数，当载入磁盘时，才从DBR中读取。
			u32 Sectors_Per_FAT;//每个FAT所占的扇区数，当载入磁盘时，才从DBR中读取。
			u32 Position_Of_RootDir;//根目录的位置。
			u32 Position_Of_FAT1;//FAT1的位置。
			u32 Position_Of_FAT2;//FAT2的位置。
    	};
	};

  /*
   * the following item(s) are only present in memory
   */
	int	sb_dev; 	/**< the super block's home device */
	int fs_type;	//added by mingxuan 2020-10-30
	int used;
	//SPIN_LOCK lock;
}super_block;

typedef struct inode {
	u32	i_mode;		/**< Accsess mode */
	u32	i_size;		/**< File size */
	u32	i_start_sect;	/**< The first sector of the data */
	u32	i_nr_sects;	/**< How many sectors the file occupies */
	u8	_unused[16];	/**< Stuff for alignment */

	/* the following items are only present in memory */
	int	i_dev;
	int	i_cnt;		/**< How many procs share this inode  */
	int	i_num;		/**< inode nr.  */
} inode;


/**
 * @struct dir_entry
 * @brief  Directory Entry
 */
typedef struct dir_entry {
	int	inode_nr;		/**< inode nr. */
	char name[MAX_FILENAME_LEN];	/**< Filename */
} dir_entry;
extern super_block sb;


void orangefs_init();
int orangefs_read_file(char *filename,void *dst);
#endif