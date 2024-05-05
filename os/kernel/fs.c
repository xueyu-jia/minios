#include "hd.h"
#include "string.h"
#include "vfs.h"
#include "buffer.h"
#include "tty.h"
#include "hd.h"
#include "memman.h"

PUBLIC struct super_block super_blocks[NR_SUPER_BLOCK]; //added by mingxuan 2020-10-30
PUBLIC struct fs_type fstype_table[NR_FS_TYPE];

PUBLIC int get_free_superblock()
{
	int sb_index = 0;
	for (sb_index = 0; sb_index < NR_SUPER_BLOCK; sb_index++)
	{
		if (super_blocks[sb_index].used == 0)
		{
			break;
		}
	}
	if (sb_index == NR_SUPER_BLOCK)
	{
		disp_str("there is no free superblock in array\n");
		return -1;
	}
	return sb_index;
}
#define MAX_DEV_PATH	16
int init_block_dev()
{
	for (int i = 0; i < 12; i++)
	{
		if (hd_infos[i].open_cnt > 0)
		{
			for (int j = 0; j < 16; j++)
			{
				if (hd_infos[i].part[j].size > 0)
				{
					int major = i,minor=j;
					register_device(MAKE_DEV(DEV_HD_BASE+i, minor), DEV_BLOCK_TYPE, &blk_file_ops);
				}
			}
		}
	}
	return 0;
}

// add by sundong 2023.5.19 
//在根文件系统下创建tty字符设备文件，设备文件分别是/dev/tty0、/dev/tty1、/dev/tty2
int init_char_dev()
{
	// struct super_block *sb = get_super_block(drive);
	//real_createdir(sb, "dev");
	// char ttypath[MAX_DEV_PATH] = {"/dev/tty0"};
	for (int i = 0; i < NR_CONSOLES; ++i)
	{
		// ttypath[strlen(ttypath) - 1] = '0' + i;
		// kern_vfs_mknod(ttypath, I_CHAR_SPECIAL|I_R|I_W, MAKE_DEV(DEV_CHAR_TTY, i));
		register_device(MAKE_DEV(DEV_CHAR_TTY, i), DEV_CHAR_TYPE, &tty_file_ops);
	}
	return 0;
}

int generic_file_readpage(struct address_space* file_address, page* target) {

	return 0;
}

int generic_file_read(struct file_desc* file, unsigned int count, char* buf) {
	return 0;
}