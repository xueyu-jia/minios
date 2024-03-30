#include "hd.h"
#include "string.h"
#include "vfs.h"
#include "fs.h"

PUBLIC struct super_block super_blocks[NR_SUPER_BLOCK]; //added by mingxuan 2020-10-30
PUBLIC struct fs_type fstype_table[NR_FS_TYPE];

int get_fs_dev(int drive, u32 fs_type)
{
	int i;
	for (i = 1; i < NR_PRIM_PER_DRIVE; i++) // 跳过第1个主分区，因为第1个分区是启动分区 comment added by ran
	{
		if (hd_infos[drive].part[i].fs_type == fs_type)
			return ((drive << MAJOR_SHIFT) | i);
	}

	// added by mingxuan 2020-10-29
	for (i = NR_PRIM_PER_DRIVE; i < NR_PRIM_PER_DRIVE + NR_SUB_PER_PART; i++)
	{
		if (hd_infos[drive].part[i].fs_type == fs_type)
			return ((drive << MAJOR_SHIFT) | i);
	}
}

int get_fs_part_dev(int drive, int part, u32 fs_type){
	if( hd_infos[drive].part[part].fs_type == fs_type){
		return MAKE_DEV(drive, part);
	}
	disp_str("fatal: FSTYPE provided incorrect");
	return -1;
}

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

int kern_init_block_dev()
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
					char dev_pathname[MAX_PATH];
					memset(dev_pathname, 0, sizeof(dev_pathname));

					if (major < SATA_BASE)
					{
						strcpy(dev_pathname, "/dev/hd");
						dev_pathname[strlen(dev_pathname)] = 'a' + major - IDE_BASE;
					}
					else if (major < SCSI_BASE)
					{
						strcpy(dev_pathname, "/dev/sd");
						dev_pathname[strlen(dev_pathname)] = 'a' + major - SATA_BASE;
					}
					else if (major < 12)
					{
						disp_str("SCSI devices are temporarily not supported\n");
						return -1;
					}
					else
					{
						disp_str("major num out of limit\n");
						return -1;
					}

					if (minor != 0)
					{
						itoa(minor, dev_pathname + strlen(dev_pathname), 10);
					}
					kern_vfs_mknod(dev_pathname, I_BLOCK_SPECIAL|I_R|I_W, MAKE_DEV(major,minor));
					// 根据一般的语义,重复创建应该报错,此处不判断
					// todo: judge by err code, allow EEXIST
				}
			}
		}
	}
	return 0;
}

// int do_init_block_dev(int drive)
// {
// 	return kern_init_block_dev(drive);
// }

// int sys_init_block_dev()
// {
// 	return do_init_block_dev(get_arg(1));
// }
// add by sundong 2023.5.19 
//在根文件系统下创建tty字符设备文件，设备文件分别是/dev/tty0、/dev/tty1、/dev/tty2
int kern_init_char_dev()
{
	// struct super_block *sb = get_super_block(drive);
	//real_createdir(sb, "dev");
	char ttypath[MAX_PATH] = {"/dev/tty0"};
	for (int i = 0; i < NR_CONSOLES; ++i)
	{
		ttypath[strlen(ttypath) - 1] = '0' + i;
		kern_vfs_mknod(ttypath, I_CHAR_SPECIAL|I_R|I_W, MAKE_DEV(DEV_CHAR_TTY, i));
	}
}
// add by sundong 2023.5.19
// int do_init_char_dev(int drive)
// {
// 	return kern_init_char_dev(drive);
// }
// // add by sundong 2023.5.19
// int sys_init_char_dev()
// {
// 	return do_init_char_dev(get_arg(1));
// }