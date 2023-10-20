/// zcr copy from chapter9/d fs/main.c and modified it.
/*
#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "fs_misc.h"
*/
#include "../include/type.h"
#include "../include/const.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/proc.h"
#include "../include/global.h"
#include "../include/proto.h"
#include "../include/fs_const.h"
#include "../include/hd.h"
#include "../include/fs.h"
#include "../include/fs_misc.h"
#include "../include/mount.h"
#include "../include/buffer.h" 

/* FSBUF_SIZE is defined as macro in fs_const.h.
 * The physical address space 6MB~7MB is used as fs buffer in Orange's, but we can't use this
 * space directly in minios. We allocate the fs buffer space in kernel initialization stage.
 * modified by xw, 18/6/15
 */
// PUBLIC const int	FSBUF_SIZE	= 0x100000;
// PUBLIC u8 * fsbuf = (u8*)0x600000;

// MESSAGE	fs_msg;		//deleted by xw, 18/8/27
// PROCESS* pcaller;		//deleted by xw, 18/8/27

/* fsbuf is a global memory area, could cause data race when multiple processes access
 * disk by using read()/write() concurrently. so delete it and use local variables instead.
 * added by xw, 18/12/27
 */
// PRIVATE u8* fsbuf;	//added by mingxuan 2019-5-20

// added by xw, 18/8/28
/* data */
PRIVATE struct inode *root_inode;

// PRIVATE struct file_desc f_desc_table[NR_FILE_DESC];	//deleted by mingxuan 2020-10-30
extern struct file_desc f_desc_table[NR_FILE_DESC]; // modified by mingxuan 2020-10-30

PRIVATE struct inode inode_table[NR_INODE];

// PRIVATE struct super_block super_block[NR_SUPER_BLOCK];	//deleted by mingxuan 2020-10-30
extern struct super_block super_block[NR_SUPER_BLOCK]; // modified by mingxuan 2020-10-30

/* functions */
PRIVATE void mkfs(int device);
// PRIVATE void read_super_block(int dev);
PUBLIC void read_super_block(int dev); // modified by mingxuan 2020-10-30
// PRIVATE struct super_block* get_super_block(int dev);
PUBLIC struct super_block *get_super_block(int dev); // modified by mingxuan 2020-10-30

// PRIVATE int do_open(MESSAGE *fs_msg);
PRIVATE int ora_open(MESSAGE *fs_msg); // modified by mingxuan 2021-8-20
// PRIVATE int do_close(int fd);
PRIVATE int ora_close(int fd); // modified by mingxuan 2021-8-20
// PRIVATE int do_lseek(MESSAGE *fs_msg);
PRIVATE int ora_lseek(MESSAGE *fs_msg); // modified by mingxuan 2021-8-20
// PRIVATE int do_rdwt(MESSAGE *fs_msg);
PRIVATE int ora_rdwt(MESSAGE *fs_msg); // modified by mingxuan 2021-8-20
// PRIVATE int do_unlink(MESSAGE *fs_msg);
PRIVATE int ora_unlink(MESSAGE *fs_msg); // modified by mingxuan 2021-8-20

// PRIVATE int real_open(const char *pathname, int flags);//deleted by mingxuan 2019-5-17
PUBLIC int real_open(struct super_block *sb, const char *pathname, int flags); // modified by mingxuan 2019-5-17

// PRIVATE int real_close(int fd);	//deleted by mingxuan 2019-5-17
PUBLIC int real_close(int fd); // modified by mingxuan 2019-5-17

// PRIVATE int real_read(int fd, void *buf, int count);	//deleted by mingxuan 2019-5-17
PUBLIC int real_read(int fd, char *buf, int count); // 注意:buf的类型被修改成char //modified by mingxuan 2019-5-17

// PRIVATE int real_write(int fd, const void *buf, int count);	//deleted by mingxuan 2019-5-17
PUBLIC int real_write(int fd, const char *buf, int count); // 注意:buf的类型被修改成char //modified by mingxuan 2019-5-17

// PRIVATE int real_unlink(const char *pathname);//deleted by mingxuan 2019-5-17
PUBLIC int real_unlink(struct super_block *sb, const char *pathname); // modified by mingxuan 2019-5-17

// PRIVATE int real_lseek(int fd, int offset, int whence); //deleted by mingxuan 2019-5-17
PUBLIC int real_lseek(int fd, int offset, int whence); // modified by mingxuan 2019-5-17


PRIVATE int strip_path(char * filename, const char * pathname, struct inode** ppinode);
PRIVATE int search_file(char *path);
PRIVATE struct inode *create_file(char *path, int imod);
PRIVATE struct inode *get_inode(int dev, int num);
PRIVATE struct inode *get_inode_sched(int dev, int num); 
PRIVATE struct inode *new_inode(int dev, int inode_nr, int start_sect, int i_mod,int nr_sects);
PRIVATE void put_inode(struct inode *pinode);
PRIVATE void sync_inode(struct inode *p);
PRIVATE void new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename);
PRIVATE int alloc_imap_bit(int dev);
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc);

PRIVATE int memcmp(const void *s1, const void *s2, int n);
PRIVATE int strcmp(const char *s1, const char *s2);

PRIVATE int create_tty_file(char *path,int tty_dev);//add by sundong 2023.5.18
PRIVATE int create_blockdev_file(char *path, int block_dev_id);//add by sundong 2023.5.28
PRIVATE int create_devfile(int drive, int major, int minor);

/*
OrangeFS新增目录树所需的一些函数 ported by sundong 2023.5.5
*/
PRIVATE int find_dir_inode_cur_dir(struct inode **ppinode, char *dir_name); /*add by xkx 2023-1-3*/
PRIVATE int find_dir_inode(struct inode **ppinode, char *dir_name);			/*add by xkx 2023-1-3*/
PRIVATE int search_file_in_dir(char *filename, struct inode *dir_inode);	/*added by gfx 2023-2-18*/
static int ora_createdir(MESSAGE *fs_msg);									/*add by xkx 2023-1-3*/
static int ora_opendir(MESSAGE *fs_msg);										/*add by xkx 2023-1-3*/
static int ora_deletedir(MESSAGE *fs_msg);									/*add by xkx 2023-1-3*/
static int do_deletecheck(MESSAGE *fs_msg);									/*add by gfx 2023-1-3*/
static int ora_showdir(MESSAGE *fs_msg);										/*add by gfx 2023-2-13*/
static int free_smap_bit(int dev, int start_sect_nr, int nr_sects_to_free); /*add by xkx 2023-1-3*/
static int free_imap_bit(int dev, int inode_nr);							/*add by xkx 2023-1-3*/
static int find_max_i_cnt(struct inode *dir_inode);							/*added by gfx 2023-1-12*/
static void set_dir_entry(struct inode *new_inode, int father_inode_nr);	/*add by xkx 2023-1-3*/

// PUBLIC int sys_init_block_dev(int);

// added by mingxuan 2020-10-27

// int get_fs_dev(int drive, int fs_type)
// {
// 	int i=0;
// 	for(i=0; i < NR_PRIM_PER_DRIVE; i++)
// 	{
// 		if(hd_info[drive].primary[i].fs_type == fs_type)
// 		return ((DEV_HD << MAJOR_SHIFT) | i);
// 	}

// 	//added by mingxuan 2020-10-29
// 	for(i=0; i < NR_SUB_PER_DRIVE; i++)
// 	{
// 		if(hd_info[drive].logical[i].fs_type == fs_type)
// 		return ((DEV_HD << MAJOR_SHIFT) | (i + MINOR_hd1a)); // logic的下标i加上hd1a才是该逻辑分区的次设备号
// 	}
// }

// 整型转字符串
PRIVATE char *itoa(int num, char *str, int radix)
{
	char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // 索引表
	unsigned unum;										   // 存放要转换的整数的绝对值,转换的整数可能是负数
	int i = 0, j, k;									   // i用来指示设置字符串相应位，转换之后i其实就是字符串的长度；转换后顺序是逆序的，有正负的情况，k用来指示调整顺序的开始位置;j用来指示调整顺序时的交换。

	// 获取要转换的整数的绝对值
	if (radix == 10 && num < 0) // 要转换成十进制数并且是负数
	{
		unum = (unsigned)-num; // 将num的绝对值赋给unum
		str[i++] = '-';		   // 在字符串最前面设置为'-'号，并且索引加1
	}
	else
		unum = (unsigned)num; // 若是num为正，直接赋值给unum

	// 转换部分，注意转换后是逆序的
	do
	{
		str[i++] = index[unum % (unsigned)radix]; // 取unum的最后一位，并设置为str对应位，指示索引加1
		unum /= radix;							  // unum去掉最后一位

	} while (unum); // 直至unum为0退出循环

	str[i] = '\0'; // 在字符串最后添加'\0'字符，c语言字符串以'\0'结束。

	// 将顺序调整过来
	if (str[0] == '-')
		k = 1; // 如果是负数，符号不用调整，从符号后面开始调整
	else
		k = 0; // 不是负数，全部都要调整

	char temp;						   // 临时变量，交换两个值时用到
	for (j = k; j <= (i - 1) / 2; j++) // 头尾一一对称交换，i其实就是字符串的长度，索引最大值比长度少1
	{
		temp = str[j];				 // 头部赋值给临时变量
		str[j] = str[i - 1 + k - j]; // 尾部赋值给头部
		str[i - 1 + k - j] = temp;	 // 将临时变量的值(其实就是之前的头部值)赋给尾部
	}

	return str; // 返回转换后的字符串
}
//add by sundong 2023.5.28
/*****************************************************************************
 *                                get_blockfile_dev
 *****************************************************************************/
/**
 * 寻找块设备类型文件，并且返回该块设备文件的设备号
 *
 * @param[in] path   The full path of the block device file.
 *
 * @return           dev_id if success; -1 if error .
*/
PUBLIC int get_blockfile_dev(char *path){
	int inode_nr = search_file(path);
	if(inode_nr < 1){
		disp_str("block device file path error!\n");
		return -1;

	}
	struct inode * p_inode = get_inode_sched(root_inode->i_dev,inode_nr);
	if(!p_inode){
		disp_str("error in get_blockfile_dev\n");
		return -1;
	}
	if(p_inode->i_mode!=I_BLOCK_SPECIAL){
		disp_str("error:the file is not a block device file,file path :");
		disp_str(path);
		return -1;

	}
	return p_inode->i_start_block;
}


int create_devfile(int drive, int major, int minor)
{
	char devname[10];
	memset(devname, 0, sizeof(devname));

	if (major < SATA_BASE)
	{
		strcpy(devname, "dev_hd");
		devname[strlen(devname)] = 'a' + major - IDE_BASE;
	}
	else if (major < SCSI_BASE)
	{
		strcpy(devname, "dev_sd");
		devname[strlen(devname)] = 'a' + major - SATA_BASE;
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
		itoa(minor, devname + strlen(devname), 10);
	}

	// int fd = real_open(devname, O_CREAT);
	// real_close(fd);

	int inode_nr = search_file(devname);

	if (inode_nr != 0)
	{
		int orange_dev = get_fs_dev(drive, ORANGE_TYPE);
		struct inode *dev_inode = get_inode_sched(orange_dev, inode_nr);

		if (dev_inode->i_mode == I_BLOCK_SPECIAL)
		{
			return 0;
		}
		else
		{
			disp_str("Conflicting file name with dev name");
			return -1;
		}
	}

	create_file(devname, I_BLOCK_SPECIAL);

	inode_nr = search_file(devname);
	int orange_dev = get_fs_dev(drive, ORANGE_TYPE);
	struct inode *dev_inode = get_inode_sched(orange_dev, inode_nr);

	dev_inode->i_mode = I_BLOCK_SPECIAL;
	sync_inode(dev_inode);
	return 0;
}

int kern_init_block_dev(int drive)
{
	for (int i = 0; i < 12; i++)
	{
		if (hd_info[i].open_cnt > 0)
		{
			for (int j = 0; j < 16; j++)
			{
				if (hd_info[i].part[j].size > 0)
				{
					int major = i,minor=j;
					char dev_pathname[MAX_PATH];
					memset(dev_pathname, 0, sizeof(dev_pathname));

					if (major < SATA_BASE)
					{
						strcpy(dev_pathname, "dev/hd");
						dev_pathname[strlen(dev_pathname)] = 'a' + major - IDE_BASE;
					}
					else if (major < SCSI_BASE)
					{
						strcpy(dev_pathname, "dev/sd");
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
					if(create_blockdev_file(dev_pathname,MAKE_DEV(major,minor))!=0){
						disp_str("\ninit_block_dev error!\n");
						return -1;
					}

					/* if (create_devfile(drive, i, j) != 0)
					{
						disp_str("\ninit_block_dev error!\n");
						return -1;
					} */
				}
			}
		}
	}
	return 0;
}

int do_init_block_dev(int drive)
{
	return kern_init_block_dev(drive);
}

int sys_init_block_dev()
{
	return do_init_block_dev(get_arg(1));
}
// add by sundong 2023.5.19 
//在根文件系统下创建tty字符设备文件，设备文件分别是/dev/tty0、/dev/tty1、/dev/tty2
PRIVATE int kern_init_char_dev(int drive)
{
	struct super_block *sb = get_super_block(drive);
	//real_createdir(sb, "dev");
	char ttypath[MAX_PATH] = {"dev/tty0"};
	for (int i = 0; i < NR_CONSOLES; ++i)
	{
		ttypath[strlen(ttypath) - 1] = '0' + i;
		if (create_tty_file(ttypath, i) != 0){
			disp_str("\ninit_char_dev error!\n");
			return -1;
		}
			
	}
}
// add by sundong 2023.5.19
int do_init_char_dev(int drive)
{
	return kern_init_char_dev(drive);
}
// add by sundong 2023.5.19
int sys_init_char_dev()
{
	return do_init_char_dev(get_arg(1));
}

/* void orangefs_test()
{
	char buf[512];
	memset(buf, 0, 512);
	real_showdir("dev", buf);
	disp_str("dirs:\n");
	disp_str(buf);

		real_createdir("test");
		real_createdir("test/dir1");
		int fd = real_open("test/f1",O_CREAT|O_RDWR);
		real_showdir("test",buf);
		disp_str("dirs:\n");
		disp_str(buf);
		real_deletedir("test/dir1");

		char buff[16] = {'a','b','c'};
		if(fd>=0){
			real_write(fd,buff,8);
			real_close(fd);
		}
		memset(buff,0,8);
		fd = real_open("test/f1",O_RDWR);
		int ret = real_deletedir("test");

		real_read(fd,buff,8);
		disp_str("file content:\n");
		disp_str(buff);
		real_close(fd);
		real_unlink("test/f1");
		memset(buf,0,512);
		real_showdir("test",buf);
		disp_str("dirs after delete:\n");
		disp_str(buf);
	
}
int sys_orangefs_dir_test()
{
	orangefs_test();
	return 0;
} */
// add by sundong 2023.5.14
/**
根据路径来判断该路径对应的文件所属的文件系统，将绝对路径转化为相对路径
 @param path  传入的是从根目录开始的绝对路径的指针，函数执行完后该指针指向的是具体文件系统内的相对路径
 @param fs_index   用于存储该路径对应的文件系统在vfs_table数组中索引
 @return 0 if success; -1 if having error
*/
PUBLIC int vfs_path_transfer(char *path, int *fs_index)
{
	*fs_index = 3;
	char *s = path;
	//	char * t = filename;
	//	if (s == 0)
	//		return -1;

	//	if (*s == '/')
	//		s++;

	//	while (*s) {		/* check each character */
	//		if (*s == '/')
	//			return -1;
	//		*t++ = *s++;
	/* if filename is too long, just truncate it */
	//		if (t - filename >= MAX_FILENAME_LEN)
	//			break;
	//	}
	//	*t = 0;

	//	*ppinode = root_inode;
	int path_len = strlen(path);
	char pre_dir_name[MAX_FILENAME_LEN];
	memset((void *)pre_dir_name, 0, MAX_FILENAME_LEN);
	char *p = pre_dir_name;
	struct inode * pinode;
	struct inode **ppinode = &pinode;
	*ppinode = root_inode;

	if (s == 0)
		return -1;
	else if (*s == '/')
	{
		// 忽略掉路径最开始的 '/'
		char tmp[path_len];
		memset(tmp,0,path_len);
		//str copy时去掉 ‘/’
		strcpy(tmp,s+1);
		memset(s,0,path_len+1);
		//重新copy到path中
		strcpy(s,tmp);
		path_len = strlen(path);
	}

	while (*s)
	{
		if (*s == '/')
		{
			if ((*ppinode)->i_mode != I_DIRECTORY) // 如果不为文件夹类型
			{
				goto fail;
			}
			if (!find_dir_inode_cur_dir(ppinode, pre_dir_name))
			{
				goto fail;
			}
			if ((*ppinode)->i_mode == I_MOUNTPOINT)
			{
				// 寻找挂载点挂载的文件系统在vfs_table中的下标
				*fs_index = get_fs_index((*ppinode)->i_mnt_index);
				if (*fs_index < 0)
					goto fail;
				// 去除path中挂载点的路径信息，将绝对路径转化为文件系统的相对路径
				char tmp[path_len + 1];
				memset(tmp, 0, path_len + 1);
				strcpy(tmp, s + 1);
				memset(path, 0, path_len);
				strcpy(path, tmp);
				goto success;
			}
			memset((void *)pre_dir_name, 0, MAX_FILENAME_LEN);
			p = pre_dir_name;
			s++;
			continue;
		}
		*p++ = *s++;
		if (p - pre_dir_name >= MAX_FILENAME_LEN)
			break;
	}
success:
	//kern_kfree(ppinode);
	return 0;
fail:
	//kern_kfree(ppinode);
	return -1;
}

PUBLIC void create_mountpoint(const char *pathname, u32 dev, u8 index_mnt_table)
{
	int reval_orange;

	int inode_nr = search_file(pathname);

	if (inode_nr == 0)
	{
		create_file(pathname, O_CREAT);
		inode_nr = search_file(pathname);
	}

	int orange_dev = dev;
	struct inode *dev_inode = get_inode_sched(orange_dev, inode_nr);

	dev_inode->i_mode = I_MOUNTPOINT;
	dev_inode->i_mnt_index = index_mnt_table;

	return;
}

PUBLIC void free_mountpoint(const char *pathname, u32 dev)
{
	int inode_nr = search_file(pathname);
	if (inode_nr == 0)
	{
		return;
	}

	int orange_dev = dev;
	struct inode *dev_inode = get_inode_sched(orange_dev, inode_nr);

	dev_inode->i_mode = I_DIRECTORY;

	return;
}

// modified by ran
int get_fs_dev(int drive, int fs_type)
{
	int i;
	for (i = 2; i < NR_PRIM_PER_DRIVE; i++) // 跳过第1个主分区，因为第1个分区是启动分区 comment added by ran
	{
		if (hd_info[drive].part[i].fs_type == fs_type)
			return ((drive << MAJOR_SHIFT) | i);
	}

	// added by mingxuan 2020-10-29
	for (i = NR_PRIM_PER_DRIVE; i < NR_PRIM_PER_DRIVE + NR_SUB_PER_PART; i++)
	{
		if (hd_info[drive].part[i].fs_type == fs_type)
			return ((drive << MAJOR_SHIFT) | i);
	}
}

PRIVATE int get_free_superblock()
{
	int sb_index = 0;
	for (sb_index = 0; sb_index < NR_SUPER_BLOCK; sb_index++)
	{
		if (super_block[sb_index].used == 0)
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

PUBLIC void read_super_block_to_target(int dev, struct super_block *target_sb)
{
	MESSAGE driver_msg;
	char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added by xw, 18/12/27

	driver_msg.type = DEV_READ;
	driver_msg.DEVICE = dev;
	driver_msg.POSITION = SECTOR_SIZE * 1;
	driver_msg.BUF = fsbuf;
	driver_msg.CNT = SECTOR_SIZE;
	driver_msg.PROC_NR = proc2pid(p_proc_current); /// TASK_A

	hd_rdwt(&driver_msg);

	struct super_block *psb = (struct super_block *)fsbuf;

	*target_sb = *psb;

	target_sb->sb_dev = dev;
}

PUBLIC int read_super_block_to_empty(int dev)
{
	int sb_index = 0;

	sb_index = get_free_superblock();

	if (sb_index == -1)
	{
		return -1;
	}

	read_super_block_to_target(dev, &super_block[sb_index]);

	return 0;
}

PUBLIC int init_orangefs(int device)
{
	disp_str("Initializing orange file system in device:");
	disp_int(device);

	if (read_super_block_to_empty(device) != 0)
	{
		return -1;
	}

	struct super_block *sb;

	sb = get_super_block(device);

	if (sb->magic != MAGIC_V1)
	{
		disp_color_str("Error:Please format the root file system before starting the kernel!!\n", 0x74);
		while (1);
	}

	return sb - super_block;
}

/// zcr added
PUBLIC void init_rootfs(int device)
{

	disp_str("Initializing root file system...  ");

	// allocate fs buffer. added by xw, 18/6/15
	// fsbuf = (u8*)K_PHY2LIN(sys_kmalloc(FSBUF_SIZE)); //deleted by xw, 18/12/27

	int i;
	// for (i = 0; i < NR_FILE_DESC; i++)						//deleted by mingxuan 2020-10-30
	//	memset(&f_desc_table[i], 0, sizeof(struct file_desc));	//deleted by mingxuan 2020-10-30

	for (i = 0; i < NR_INODE; i++)
		memset(&inode_table[i], 0, sizeof(struct inode));
	struct super_block *sb = super_block; // deleted by mingxuan 2020-10-30
	// for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)				//deleted by mingxuan 2020-10-30
	//	sb->sb_dev = NO_DEV;										//deleted by mingxuan 2020-10-30
	int orange_dev = get_fs_dev(device, ORANGE_TYPE); // added by mingxuan 2020-10-27

	/* load super block of ROOT */
	// read_super_block(ROOT_DEV);		// deleted by mingxuan 2020-10-27
	read_super_block(orange_dev); // modified by mingxuan 2020-10-27
	// sb = get_super_block(ROOT_DEV);	// deleted by mingxuan 2020-10-27
    disp_int(11);
	sb = get_super_block(orange_dev); // modified by mingxuan 2020-10-27

	disp_str("Superblock Address:");
	disp_int(sb);
	disp_str(" \n");

	if (sb->magic != MAGIC_V1)
	{ // deleted by mingxuan 2019-5-20
		// modified by sundong 2023.5.8
		// 如果根文件系统没有在kernel启动前格式化为orangefs就报错，系统无法继续下去
		disp_color_str("Error:Please format the root file system before starting the kernel!!\n", 0x74);
		while (1)
			;

		// mkfs(orange_dev);
		// disp_str("Make file system Done.\n");

		// deleted by mingxuan 2020-10-30
		// for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
		//	sb->sb_dev = NO_DEV;

		// read_super_block(ROOT_DEV);	// deleted by mingxuan 2020-10-27
		// read_super_block(orange_dev);	// modified by mingxuan 2020-10-27
	}

	// root_inode = get_inode(ROOT_DEV, ROOT_INODE);	// deleted by mingxuan 2020-10-27
	root_inode = get_inode(orange_dev, ROOT_INODE); // modified by xiaofeng 2020-10-27
}

/*****************************************************************************
 *                                mkfs
 *****************************************************************************/
/**
 * <Ring 1> Make a available Orange'S FS in the disk. It will
 *          - Write a super block to sector 1.
 *          - Create three special files: dev_tty0, dev_tty1, dev_tty2
 *          - Create the inode map
 *          - Create the sector map
 *          - Create the inodes of the files
 *          - Create `/', the root directory
 *****************************************************************************/
//PRIVATE void mkfs(int device)
//{
//	MESSAGE driver_msg;
//	int i, j;
//
//	int bits_per_sect = SECTOR_SIZE * 8; /* 8 bits per byte */
//
//	// local array, to substitute global fsbuf. added by xw, 18/12/27
//	char fsbuf[SECTOR_SIZE];
//
//	int orange_dev = device; // added by mingxuan 2020-10-27
//
//	/* get the geometry of ROOTDEV */
//	struct part_info geo;
//	driver_msg.type = DEV_IOCTL;
//	// driver_msg.DEVICE	= MINOR(ROOT_DEV);		// deleted by mingxuan 2020-10-27
//	driver_msg.DEVICE = orange_dev; // modified by mingxuan 2020-10-27
//	driver_msg.REQUEST = DIOCTL_GET_GEO;
//	driver_msg.BUF = &geo;
//	driver_msg.PROC_NR = proc2pid(p_proc_current);
//	// assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
//	// send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);
//	hd_ioctl(&driver_msg);
//
//	// printl("dev size: 0x%x sectors\n", geo.size);
//	disp_str(" dev size: ");
//	disp_int(geo.size);
//	disp_str(" sectors\n");
//
//	/************************/
//	/*      super block     */
//	/************************/
//	struct super_block sb;
//	sb.magic = MAGIC_V1;
//	sb.nr_inodes = bits_per_sect;
//	sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE;
//	sb.nr_sects = geo.size; /* partition size in sector */
//	sb.nr_imap_sects = 1;
//	sb.nr_smap_sects = sb.nr_sects / bits_per_sect + 1;
//	sb.n_1st_sect = 1 + 1 + /* boot sector & super block */
//					sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects;
//	sb.root_inode = ROOT_INODE;
//	sb.inode_size = INODE_SIZE;
//
//	struct inode x;
//	sb.inode_isize_off = (int)&x.i_size - (int)&x;
//	sb.inode_start_off = (int)&x.i_start_sect - (int)&x;
//	sb.dir_ent_size = DIR_ENTRY_SIZE;
//
//	struct dir_entry de;
//	sb.dir_ent_inode_off = (int)&de.inode_nr - (int)&de;
//	sb.dir_ent_fname_off = (int)&de.name - (int)&de;
//
//	sb.sb_dev = orange_dev;	  // added by mingxuan 2020-10-30
//	sb.fs_type = ORANGE_TYPE; // added by mingxuan 2020-10-30
//
//	memset(fsbuf, 0x90, SECTOR_SIZE);
//	memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);
//
//	/* write the super block */
//	// WR_SECT(ROOT_DEV, 1, fsbuf);	//added by xw, 18/12/27 // deleted by mingxuan 2020-10-27
//	WR_SECT(orange_dev, 1, fsbuf); // modified by mingxuan 2020-10-27
//
//	// printl("devbase:0x%x00, sb:0x%x00, imap:0x%x00, smap:0x%x00\n"
//	//        "        inodes:0x%x00, 1st_sector:0x%x00\n",
//	//        geo.base * 2,
//	//        (geo.base + 1) * 2,
//	//        (geo.base + 1 + 1) * 2,
//	//        (geo.base + 1 + 1 + sb.nr_imap_sects) * 2,
//	//        (geo.base + 1 + 1 + sb.nr_imap_sects + sb.nr_smap_sects) * 2,
//	//        (geo.base + sb.n_1st_sect) * 2);
//
//	disp_str("devbase:");
//	disp_int(geo.base * 2);
//	disp_str("00");
//
//	disp_str(" sb:");
//	disp_int((geo.base + 1) * 2);
//	disp_str("00");
//	disp_str(" imap:");
//
//	disp_int((geo.base + 1 + 1) * 2);
//	disp_str("00");
//	disp_str(" smap:");
//
//	disp_int((geo.base + 1 + 1 + sb.nr_imap_sects) * 2);
//	disp_str("00\n");
//
//	disp_str("        inodes:");
//	disp_int((geo.base + 1 + 1 + sb.nr_imap_sects + sb.nr_smap_sects) * 2);
//	disp_str("00");
//
//	disp_str(" 1st_sector:");
//	disp_int((geo.base + sb.n_1st_sect) * 2);
//	disp_str("00\n");
//
//	/************************/
//	/*       inode map      */
//	/************************/
//	memset(fsbuf, 0, SECTOR_SIZE);
//	// for (i = 0; i < (NR_CONSOLES + 2); i++) //deleted by mingxuan 2019-5-22
//	for (i = 0; i < (NR_CONSOLES + 3); i++) // modified by mingxuan 2019-5-22
//		fsbuf[0] |= 1 << i;
//
//	// WR_SECT(ROOT_DEV, 2);
//	// WR_SECT(ROOT_DEV, 2, fsbuf);	//modified by xw, 18/12/27 // deleted by mingxuan 2020-10-27
//	WR_SECT(orange_dev, 2, fsbuf); // modified by mingxuan 2020-10-27
//
//	/************************/
//	/*      secter map      */
//	/************************/
//	memset(fsbuf, 0, SECTOR_SIZE);
//	int nr_sects = NR_DEFAULT_FILE_SECTS + 1;
//	/*             ~~~~~~~~~~~~~~~~~~~|~   |
//	 *                                |    `--- bit 0 is reserved
//	 *                                `-------- for `/'
//	 */
//	for (i = 0; i < nr_sects / 8; i++)
//		fsbuf[i] = 0xFF;
//
//	for (j = 0; j < nr_sects % 8; j++)
//		fsbuf[i] |= (1 << j);
//
//	// WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects, fsbuf);	//modified by xw, 18/12/27 //deleted by mingxuan 2020-10-27
//	WR_SECT(orange_dev, 2 + sb.nr_imap_sects, fsbuf); // modified by mingxuan 2020-10-27
//
//	/* zeromemory the rest sector-map */
//	memset(fsbuf, 0, SECTOR_SIZE);
//	for (i = 1; i < sb.nr_smap_sects; i++)
//		// WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + i, fsbuf);	//modified by xw, 18/12/27	//deleted by mingxuan 2020-10-27
//		WR_SECT(orange_dev, 2 + sb.nr_imap_sects + i, fsbuf); // modified by mingxuan 2020-10-27
//
//	/* app.tar */
//	// added by mingxuan 2019-5-19
//	int bit_offset = INSTALL_START_SECTOR -
//					 sb.n_1st_sect + 1; /* sect M <-> bit (M - sb.n_1stsect + 1) */
//	int bit_off_in_sect = bit_offset % (SECTOR_SIZE * 8);
//	int bit_left = INSTALL_NR_SECTORS;
//	int cur_sect = bit_offset / (SECTOR_SIZE * 8);
//	// RD_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect);	//deleted by mingxuan 2019-5-19
//	// RD_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect, fsbuf);	//modified by mingxuan 2019-5-19	//deleted by mingxuan 2020-10-27
//	RD_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf); // modified by mingxuan 2020-10-27
//	while (bit_left)
//	{
//		int byte_off = bit_off_in_sect / 8;
//		/* this line is ineffecient in a loop, but I don't care */
//		fsbuf[byte_off] |= 1 << (bit_off_in_sect % 8);
//		bit_left--;
//		bit_off_in_sect++;
//		if (bit_off_in_sect == (SECTOR_SIZE * 8))
//		{
//			// WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect);	//deleted by mingxuan 2019-5-19
//			// WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect, fsbuf);	//modified by mingxuan 2019-5-19 	//deleted by mingxuan 2020-10-27
//			WR_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf); // modified by mingxuan 2020-10-27
//			cur_sect++;
//			// RD_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect);	//deleted by mingxuan 2019-5-19
//			// RD_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect, fsbuf);	//modified by mingxuan 2019-5-19	//deleted by mingxuan 2020-10-27
//			RD_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf); // modified by mingxuan 2020-10-27
//			bit_off_in_sect = 0;
//		}
//	}
//	// WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect);	//deleted by mingxuan 2019-5-19
//	// WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect, fsbuf);	//modified by mingxuan 2019-5-19	//deleted by mingxuan 2020-10-27
//	WR_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf); // modified by mingxuan 2020-10-27
//
//	/************************/
//	/*       inodes         */
//	/************************/
//	/* inode of `/' */
//	memset(fsbuf, 0, SECTOR_SIZE);
//	struct inode *pi = (struct inode *)fsbuf;
//	pi->i_mode = I_DIRECTORY;
//
//	// deleted by mingxuan 2019-5-21
//	// pi->i_size = DIR_ENTRY_SIZE * 4; //4 files: (预定义四个文件)
//	/* `.',
//	 * `dev_tty0', `dev_tty1', `dev_tty2',
//	 */
//
//	// modified by mingxuan 2019-5-21
//	pi->i_size = DIR_ENTRY_SIZE * 5; /* 5 files: (预定义5个文件)
//									  * `.',
//									  * `dev_tty0', `dev_tty1', `dev_tty2','app.tar'
//									  */
//
//	pi->i_start_sect = sb.n_1st_sect;
//	pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;
//
//	/* inode of `/dev_tty0~2' */
//	for (i = 0; i < NR_CONSOLES; i++)
//	{
//		pi = (struct inode *)(fsbuf + (INODE_SIZE * (i + 1)));
//		pi->i_mode = I_CHAR_SPECIAL;
//		pi->i_size = 0;
//		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
//		pi->i_nr_sects = 0;
//	}
//
//	/* inode of /app.tar */
//	// added by mingxuan 2019-5-19
//	pi = (struct inode *)(fsbuf + (INODE_SIZE * (NR_CONSOLES + 1)));
//	pi->i_mode = I_REGULAR;
//	pi->i_size = INSTALL_NR_SECTORS * SECTOR_SIZE;
//	pi->i_start_sect = INSTALL_START_SECTOR;
//	pi->i_nr_sects = INSTALL_NR_SECTORS;
//
//	// WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + sb.nr_smap_sects, fsbuf);	//modified by xw, 18/12/27	//deleted by mingxuan 2020-10-27
//	WR_SECT(orange_dev, 2 + sb.nr_imap_sects + sb.nr_smap_sects, fsbuf); // modified by mingxuan 2020-10-27
//
//	/************************/
//	/*          `/'         */
//	/************************/
//	memset(fsbuf, 0, SECTOR_SIZE);
//	struct dir_entry *pde = (struct dir_entry *)fsbuf;
//
//	pde->inode_nr = 1;
//	strcpy(pde->name, ".");
//
//	/* dir entries of `/dev_tty0~2' */
//	for (i = 0; i < NR_CONSOLES; i++)
//	{
//		pde++;
//		pde->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
//		// sprintf(pde->name, "dev_tty%d", i);
//		/// zcr added to replace the statement above
//		switch (i)
//		{
//		case 0:
//			strcpy(pde->name, "dev_tty0");
//			break;
//		case 1:
//			strcpy(pde->name, "dev_tty1");
//			break;
//		case 2:
//			strcpy(pde->name, "dev_tty2");
//			break;
//		}
//	}
//
//	(++pde)->inode_nr = NR_CONSOLES + 2; // added by mingxuan 2019-5-19
//	strcpy(pde->name, INSTALL_FILENAME); // added by mingxuan 2019-5-19
//
//	// WR_SECT(ROOT_DEV, sb.n_1st_sect, fsbuf);	//modified by xw, 18/12/27	//deleted by mingxuan 2020-10-27
//	WR_SECT(orange_dev, sb.n_1st_sect, fsbuf); // modified by mingxuan 2020-10-27
//
//}
//   
//	/************************/
//	/*       inodes         */
//	/************************/
//	/* inode of `/' */
//	memset(fsbuf, 0, SECTOR_SIZE);
//	struct inode * pi = (struct inode*)fsbuf;
//	pi->i_mode = I_DIRECTORY;
//
//	//deleted by mingxuan 2019-5-21
//	//pi->i_size = DIR_ENTRY_SIZE * 4; //4 files: (预定义四个文件)
//					  /* `.',
//					  * `dev_tty0', `dev_tty1', `dev_tty2',
//					  */
//
//	//modified by mingxuan 2019-5-21
//	pi->i_size = DIR_ENTRY_SIZE * 5; /* 5 files: (预定义5个文件)
//					  * `.',
//					  * `dev_tty0', `dev_tty1', `dev_tty2','app.tar'
//					  */
//
//	pi->i_start_sect = sb.n_1st_sect;
//	pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;
//
//	/* inode of `/dev_tty0~2' */
//	for (i = 0; i < NR_CONSOLES; i++) {
//		pi = (struct inode*)(fsbuf + (INODE_SIZE * (i + 1)));
//		pi->i_mode = I_CHAR_SPECIAL;
//		pi->i_size = 0;
//		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
//		pi->i_nr_sects = 0;
//	}
//
//	/* inode of /app.tar */
//	//added by mingxuan 2019-5-19
//	pi = (struct inode*)(fsbuf + (INODE_SIZE * (NR_CONSOLES + 1)));
//	pi->i_mode = I_REGULAR;
//	pi->i_size = INSTALL_NR_SECTORS * SECTOR_SIZE;
//	pi->i_start_sect = INSTALL_START_SECTOR;
//	pi->i_nr_sects = INSTALL_NR_SECTORS;
//
//	//WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + sb.nr_smap_sects, fsbuf);	//modified by xw, 18/12/27	//deleted by mingxuan 2020-10-27
//	WR_SECT(orange_dev, 2 + sb.nr_imap_sects + sb.nr_smap_sects, fsbuf);	//modified by mingxuan 2020-10-27
//
//	/************************/
//	/*          `/'         */
//	/************************/
//	memset(fsbuf, 0, SECTOR_SIZE);
//	struct dir_entry * pde = (struct dir_entry *)fsbuf;
//
//	pde->inode_nr = 1;
//	strcpy(pde->name, ".");
//
//	/* dir entries of `/dev_tty0~2' */
//	for (i = 0; i < NR_CONSOLES; i++) {
//		pde++;
//		pde->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
//		// sprintf(pde->name, "dev_tty%d", i);
//		/// zcr added to replace the statement above
//		switch(i) {
//			case 0:
//				strcpy(pde->name, "dev_tty0");
//				break;
//			case 1:
//				strcpy(pde->name, "dev_tty1");
//				break;
//			case 2:
//				strcpy(pde->name, "dev_tty2");
//				break;
//		}
//	}
//
//	(++pde)->inode_nr = NR_CONSOLES + 2; //added by mingxuan 2019-5-19
//	strcpy(pde->name, INSTALL_FILENAME); //added by mingxuan 2019-5-19
//
//	//WR_SECT(ROOT_DEV, sb.n_1st_sect, fsbuf);	//modified by xw, 18/12/27	//deleted by mingxuan 2020-10-27
//	WR_SECT(orange_dev, sb.n_1st_sect, fsbuf);	//modified by mingxuan 2020-10-27
//} 

/*****************************************************************************
 *                                rw_sector
 *****************************************************************************/
/**
 * <Ring 1> R/W a sector via messaging with the corresponding driver.
 *
 * @param io_type  DEV_READ or DEV_WRITE
 * @param dev      device nr
 * @param pos      Byte offset from/to where to r/w.
 * @param bytes    r/w count in bytes.
 * @param proc_nr  To whom the buffer belongs.
 * @param buf      r/w buffer.
 *
 * @return Zero if success.
 *****************************************************************************/
/* /// zcr: change the "u64 pos" to "int pos"
PRIVATE int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
{
	MESSAGE driver_msg;

	driver_msg.type		= io_type;
	// driver_msg.DEVICE	= MINOR(dev);
	driver_msg.DEVICE	= dev;
	//attention
	// driver_msg.POSITION	= (unsigned long long)pos;
	driver_msg.POSITION	= pos;
	driver_msg.CNT		= bytes;	/// hu is: 512
	driver_msg.PROC_NR	= proc_nr;
	driver_msg.BUF		= buf;
	// assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	// send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);

	/// replace the statement above.
	// disp_int(proc_nr);
	hd_rdwt(&driver_msg);
	return 0;
}

//added by xw, 18/8/27
PRIVATE int rw_sector_sched(int io_type, int dev, int pos, int bytes, int proc_nr, void* buf)
{
	MESSAGE driver_msg;

	driver_msg.type		= io_type;
	// driver_msg.DEVICE	= MINOR(dev);
	driver_msg.DEVICE	= dev;

	driver_msg.POSITION	= pos;
	driver_msg.CNT		= bytes;	/// hu is: 512
	driver_msg.PROC_NR	= proc_nr;
	driver_msg.BUF		= buf;

	hd_rdwt_sched(&driver_msg);
	return 0;
} */
//~xw

// added by zcr from chapter9/e/lib/open.c and modified it.

/*****************************************************************************
 *                                open
 *****************************************************************************/
/**
 * open/create a file.
 *
 * @param pathname  The full path of the file to be opened/created.
 * @param flags     O_CREAT, O_RDWR, etc.
 *
 * @return File descriptor if successful, otherwise -1.
 *****************************************************************************/
// open is a syscall interface now. added by xw, 18/6/18
//  PUBLIC int open(const char *pathname, int flags)
//  PRIVATE int real_open(const char *pathname, int flags)	//deleted by mingxuan 2019-5-17
PUBLIC int real_open(struct super_block *sb, const char *pathname, int flags) // modified by mingxuan 2019-5-17
{
	/* 	disp_str("\nreal_open pathname :");
		disp_str(pathname); */
		//deleted by sundong 2023.5.18
/* 	char orange_pathname[20];
	int i, j;
	int inode_nr = 0;

	for (i = 0; i < strlen(pathname); i++)
	{
		if (pathname[i] == '/')
		{
			j = i;
			for (j = 0; j < i; j++)
			{
				orange_pathname[j] = pathname[j];
			}
			orange_pathname[i] = '\0';
			 			//disp_str("\nreal_open orange_pathname :");
						//disp_str(orange_pathname); 
			inode_nr = search_file(orange_pathname);
			 			//disp_str("\n real_open inode_nr :");
						//char output[16];
						//itoa(output, inode_nr,10);
						//disp_str(output); 

			if (inode_nr == 0)
				break;

			int orange_dev = root_inode->i_dev;
			struct inode *mnt_node = get_inode_sched(orange_dev, inode_nr);

			if (mnt_node->i_mode == I_MOUNTPOINT)
			{
				int fd = mount_open(pathname, flags);
				return fd;
			}
			// 如果是目录文件
			else if (mnt_node->i_mode == I_DIRECTORY)
			{
				MESSAGE fs_msg;
				fs_msg.type = OPEN;
				fs_msg.PATHNAME = (void *)(pathname);
				fs_msg.FLAGS = flags;
				fs_msg.NAME_LEN = strlen(pathname);
				fs_msg.source = proc2pid(p_proc_current);
				int fd = ora_open(&fs_msg);
				return fd;
			}
			else
			{
				return -1;
			}
		}
	}

	// added by xw, 18/8/27
	// root fs
	for (i = 0; i < strlen(pathname); i++)
	{
		if (pathname[i] == '/')
		{
			break;
		}
	}
	if (i < strlen(pathname))
	{
		i = i + 1;
	}
	else
	{
		i = 0;
	} */
	MESSAGE fs_msg;

	fs_msg.type = OPEN;
	fs_msg.PATHNAME = (void *)(pathname);
	fs_msg.FLAGS = flags;
	fs_msg.NAME_LEN = strlen(pathname);
	fs_msg.source = proc2pid(p_proc_current);

	// int fd = do_open();	//modified by xw, 18/8/27
	// int fd = do_open(&fs_msg);
	int fd = ora_open(&fs_msg); // modified by mingxuan 2021-8-20

	// p_proc_current -> task.filp[fd] -> dev_index = root_inode->i_dev;

	// send_recv(BOTH, TASK_FS, &msg);
	// assert(msg.type == SYSCALL_RET);
	// return msg.FD;
	return fd;
}

/*****************************************************************************
 *                                do_open
 *****************************************************************************/
/**
 * Open a file and return the file descriptor.
 *
 * @return File descriptor if successful, otherwise a negative error code.
 *****************************************************************************/
/// zcr modified.
// PRIVATE int do_open(MESSAGE *fs_msg)
PRIVATE int ora_open(MESSAGE *fs_msg) // modified by mingxuan 2021-8-20
{
	/*caller_nr is the process number of the */
	int fd = -1; /* return value */

	char pathname[MAX_PATH];

	/* get parameters from the message */
	int flags = fs_msg->FLAGS;		 /* access mode */
	int name_len = fs_msg->NAME_LEN; /* length of filename */
	int src = fs_msg->source;		 /* caller proc nr. */

	/// zcr for debugging(the output is 0x1)
	// disp_str("src is: ");
	// disp_int(src);

	// assert(name_len < MAX_PATH);
	// phys_copy((void*)va2la(TASK_A, pathname), (void*)va2la(src, fs_msg.PATHNAME), name_len);
	phys_copy((void *)va2la(src, pathname), (void *)va2la(src, fs_msg->PATHNAME), name_len);
	pathname[name_len] = 0;

	/* find a free slot in PROCESS::filp[] */
	int i;
	// for (i = 0; i < NR_FILES; i++) {
	/* 0, 1, 2 are reserved for stdin, stdout, stderr. modified by xw, 18/8/28 */
	// for (i = 3; i < NR_FILES; i++) { //deleted by mingxuan 2019-5-20
	for (i = 0; i < NR_FILES; i++)
	{ // modified by mingxuan 2019-5-20
		if (p_proc_current->task.filp[i] == 0)
		{
			fd = i;
			break;
		}
	}
	if ((fd < 0) || (fd >= NR_FILES))
	{
		return -1;
		// panic("filp[] is full (PID:%d)", proc2pid(p_proc_current));
		disp_str("filp[] is full (PID:");
		disp_int(proc2pid(p_proc_current));
		disp_str(")\n");
	}

	/* find a free slot in f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++)
		// deleted by mingxuan 2019-5-17
		// if (f_desc_table[i].fd_inode == 0) {
		//	f_desc_table[i].fd_inode = -1;	//to decrease the chance of two process finding the same free slot
		// a lock should be used here in the future. added by xw, 18/12/28
		//	break;
		//}
		// modified by mingxuan 2019-5-17
		if (f_desc_table[i].flag == 0)
			break;
	if (i >= NR_FILE_DESC)
	{
		// panic("f_desc_table[] is full (PID:%d)", proc2pid(p_proc_current));
		disp_str("f_desc_table[] is full (PID:");
		disp_int(proc2pid(p_proc_current));
		disp_str(")\n");
	}

	int inode_nr = search_file(pathname);

	/// zcr debug (output is 0x0, right.)
	// disp_str("the inode_nr result of search_file(): ");
	// disp_int(inode_nr);
	// disp_str("\n");

	struct inode *pin = 0;
	if (flags & O_CREAT)
	{
		if (inode_nr)
		{
			// disp_str("file exists.\n");	/// zcr	//deleted by mingxuan 2019-5-22
			return -1;
		}
		else
		{
			pin = create_file(pathname, I_REGULAR);
			/// zcr debugging
			// disp_str("create a new file done.\n");
		}
	}
	else
	{
		// assert(flags & O_RDWR);

		char filename[MAX_PATH];
		struct inode *dir_inode;
		if (strip_path(filename, pathname, &dir_inode) != 0)
			return -1;
		pin = get_inode_sched(dir_inode->i_dev, inode_nr); // modified by xw, 18/8/28
																	   // pin = get_inode(dir_inode->i_dev, inode_nr); //modified by mingxuan 2019-5-20
																	   /// zcr
																	   // disp_str("get the i-node of a file already exists.\n");	//deleted by mingxuan 2019-5-22
	}
	/// zcr for debugging
	// disp_int(pin);

	if (pin)
	{
		/* connects proc with file_descriptor */
		p_proc_current->task.filp[fd] = &f_desc_table[i];

		f_desc_table[i].flag = 1; // added by mingxuan 2019-5-17

		/* connects file_descriptor with inode */
		// f_desc_table[i].fd_inode = pin;	//deleted by mingxuan 2019-5-17
		f_desc_table[i].fd_node.fd_inode = pin; // modified by mingxuan 2019-5-17

		f_desc_table[i].fd_mode = flags;
		/* f_desc_table[i].fd_cnt = 1; */
		f_desc_table[i].fd_pos = 0;

		pin->i_cnt++;

		int imode = pin->i_mode & I_TYPE_MASK;

		if (imode == I_CHAR_SPECIAL)
		{
			MESSAGE driver_msg;

			// driver_msg.type = DEV_OPEN;
			int dev = pin->i_start_block;
			p_proc_current->task.filp[fd]->dev_index = MINOR(dev);

			// driver_msg.DEVICE = MINOR(dev);
			// assert(MAJOR(dev) == 4);
			// assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);

			// send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);

			/// zcr added to replace the statement above
			// hd_open(MINOR(dev));	//deleted by mingxuan 2019-5-20
		}
		else if (imode == I_DIRECTORY||imode == I_REGULAR)
		{
			p_proc_current->task.filp[fd]->dev_index = -1;

			// assert(pin->i_num == ROOT_INODE);
			/// zcr
			//deleted by sundong 2023.5.18
			/* if (pin->i_num != ROOT_INODE)
			{
				disp_str("Panic: pin->i_num != ROOT_INODE");
			} */
		}else
		{
			// assert(pin->i_mode == I_REGULAR);
			// disp_str("REGULAR MODE  ");
			disp_str("Panic: pin->i_mode != I_REGULAR");

/* 			if (pin->i_mode != I_REGULAR)
			{
				/// zcr modified.
			} */
		}
	}
	else
	{
		return -1;
	}

	return fd;
}
/*****************************************************************************
 *                                create_blockdev_file
 *****************************************************************************/
/**
 * Create a block devcie file 
 *
 * @param[in] path   The full path of the new file
 * @param[in] block_dev 块设备的设备号
 *
 * @return           0 success; -1 error .
*/
PRIVATE int create_blockdev_file(char *path, int block_dev_id){
	char filename[MAX_FILENAME_LEN];
	int inode_nr;
	struct inode *dir_inode;
	inode_nr = search_file(path);
	// 如果文件不存在，则创建块设备文件
	if (inode_nr == 0)
	{
		if (strip_path(filename, path, &dir_inode) != 0)
			return -1;

		inode_nr = alloc_imap_bit(dir_inode->i_dev);
		struct inode *newino = new_inode(dir_inode->i_dev, inode_nr, block_dev_id, I_BLOCK_SPECIAL, 0);

		new_dir_entry(dir_inode, newino->i_num, filename);
		sync_inode(newino);
	}
	else
	{ // 如果文件存在
		if (strip_path(filename, path, &dir_inode) != 0)
			return -1;
		struct inode *p_inode = get_inode_sched(dir_inode->i_dev, inode_nr);
		// 检查是否为块设备类型
		if (p_inode == NULL || p_inode->i_mode != I_BLOCK_SPECIAL)
			return -1;
	}
	return 0;

}

/*****************************************************************************
 *                                create_tty_file
 *****************************************************************************/
/**
 * Create a tty file 
 *
 * @param[in] path   The full path of the new file
 * @param[in] tty_dev  tty设备在vfs_table中的索引，通常与tty设备的minor设备号一致
 *
 * @return           0 success; -1 error .
*/
PRIVATE int create_tty_file(char *path, int tty_dev)
{
	char filename[MAX_FILENAME_LEN];
	int inode_nr;
	struct inode *dir_inode;
	inode_nr = search_file(path);
	// 如果文件不存在，则创建字符设备文件
	if (inode_nr == 0)
	{
		if (strip_path(filename, path, &dir_inode) != 0)
			return -1;

		inode_nr = alloc_imap_bit(dir_inode->i_dev);
		struct inode *newino = new_inode(dir_inode->i_dev, inode_nr, MAKE_DEV(DEV_CHAR_TTY, tty_dev), I_CHAR_SPECIAL, 0);

		new_dir_entry(dir_inode, newino->i_num, filename);
		sync_inode(newino);
	}
	else
	{ // 如果文件存在
		if (strip_path(filename, path, &dir_inode) != 0)
			return -1;
		struct inode *p_inode = get_inode_sched(dir_inode->i_dev, inode_nr);
		// 检查是否为字符类型
		if (p_inode == NULL || p_inode->i_mode != I_CHAR_SPECIAL)
			return -1;
	}
	return 0;
}
/*****************************************************************************
 *                                create_file
 *****************************************************************************/
/**
 * Create a file and return it's inode ptr.
 *
 * @param[in] path   The full path of the new file
 * @param[in] imod  Attribiutes of the new file
 *
 * @return           Ptr to i-node of the new file if successful, otherwise 0.
 *
 * @see open()
 * @see do_open()
 *****************************************************************************/
PRIVATE struct inode *create_file(char *path, int imod)
{
	/// zcr debug
	// deleted by mingxuan 2019-5-22
	// disp_str("path: ");
	// disp_str(path);
	// disp_str("  ");

	char filename[MAX_FILENAME_LEN];
	struct inode *dir_inode;

	if (strip_path(filename, path, &dir_inode) != 0)
		return 0;

	int inode_nr = alloc_imap_bit(dir_inode->i_dev);
	/// zcr debug(output is 0x1,wrong! should be 0x5!)
	// deleted by mingxuan 2019-5-22
	// disp_str("inode_nr: ");
	// disp_int(inode_nr);
	// disp_str("    ");

	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev, NR_DEFAULT_FILE_BLOCKS);
	/// zcr debug(output is 0x10E)
	// disp_str("free_sect_nr: ");
	// disp_int(free_sect_nr);
	// disp_str("\n");
	struct inode *newino = new_inode(dir_inode->i_dev, inode_nr, free_sect_nr, imod,NR_DEFAULT_FILE_BLOCKS);

	new_dir_entry(dir_inode, newino->i_num, filename);

	return newino;
}

/// zcr copied from ch9/e/fs/misc.c

/*****************************************************************************
 *                                memcmp
 *****************************************************************************/
/**
 * Compare memory areas.
 *
 * @param s1  The 1st area.
 * @param s2  The 2nd area.
 * @param n   The first n bytes will be compared.
 *
 * @return  an integer less than, equal to, or greater than zero if the first
 *          n bytes of s1 is found, respectively, to be less than, to match,
 *          or  be greater than the first n bytes of s2.
 *****************************************************************************/
PRIVATE int memcmp(const void *s1, const void *s2, int n)
{
	if ((s1 == 0) || (s2 == 0))
	{ /* for robustness */
		return (s1 - s2);
	}

	const char *p1 = (const char *)s1;
	const char *p2 = (const char *)s2;
	int i;
	for (i = 0; i < n; i++, p1++, p2++)
	{
		if (*p1 != *p2)
		{
			return (*p1 - *p2);
		}
	}
	return 0;
}

/*****************************************************************************
 *                                strcmp
 *****************************************************************************/
/**
 * Compare two strings.
 *
 * @param s1  The 1st string.
 * @param s2  The 2nd string.
 *
 * @return  an integer less than, equal to, or greater than zero if s1 (or the
 *          first n bytes thereof) is  found,  respectively,  to  be less than,
 *          to match, or be greater than s2.
 *****************************************************************************/
PRIVATE int strcmp(const char *s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0))
	{ /* for robustness */
		return (s1 - s2);
	}

	const char *p1 = s1;
	const char *p2 = s2;

	for (; *p1 && *p2; p1++, p2++)
	{
		if (*p1 != *p2)
		{
			break;
		}
	}

	return (*p1 - *p2);
}

/*****************************************************************************
 *                                search_file
 *****************************************************************************/
/**
 * Search the file and return the inode_nr.
 *
 * @param[in] path The full path of the file to search.
 * @return         Ptr to the i-node of the file if successful, otherwise zero.
 *
 * @see open()
 * @see do_open()
 *****************************************************************************/
/*
 *	modified by gfx 2023-2-18 ported by sundong 2023.5.6
 *	搜索过程中增加了对目录树的支持，整个搜索过程不在是只在根目录下进行
 */
PRIVATE int search_file(char *path)
{
	int i, j;

	char filename[MAX_PATH];
	memset(filename, 0, MAX_FILENAME_LEN);
	struct inode *dir_inode;
	// 获取路径中的文件的文件名和父目录inode
	if (strip_path(filename, path, &dir_inode) != 0)
		return 0;
	return search_file_in_dir(filename, dir_inode);

	//	if (filename[0] == 0)	/* path: "/" */
	//		return dir_inode->i_num;

	/**
	 * Search the dir for the file.
	 */
	//	int dir_blk0_nr = dir_inode->i_start_sect;
	//	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	//	int nr_dir_entries =
	//	  dir_inode->i_size / DIR_ENTRY_SIZE; /**
	//					       * including unused slots
	//					       * (the file has been deleted
	//					       * but the slot is still there)
	//					       */
	//	int m = 0;
	//	struct dir_entry * pde;
	//	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27
	//	for (i = 0; i < nr_dir_blks; i++) {
	//		RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
	// RD_SECT(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by mingxuan 2019-5-20
	//		pde = (struct dir_entry *)fsbuf;
	//		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
	//			if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
	//				return pde->inode_nr;
	//			if (++m > nr_dir_entries)
	//				break;
	//		}
	//		if (m > nr_dir_entries) /* all entries have been iterated */
	//			break;
	//	}

	/* file not found */
	//	return 0;
}

//PRIVATE int search_file_sync(char *path)
//{
//	int i, j;
//
//	char filename[MAX_PATH];
//	memset(filename, 0, MAX_FILENAME_LEN);
//	struct inode *dir_inode;
//	if (strip_path(filename, path, &dir_inode) != 0)
//		return 0;
//
//	if (filename[0] == 0) /* path: "/" */
//		return dir_inode->i_num;
//
//	/**
//	 * Search the dir for the file.
//	 */
//	int dir_blk0_nr = dir_inode->i_start_sect;
//	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
//	int nr_dir_entries =
//		dir_inode->i_size / DIR_ENTRY_SIZE; /**
//											 * including unused slots
//											 * (the file has been deleted
//											 * but the slot is still there)
//											 */
//	int m = 0;
//	struct dir_entry *pde;
//	char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added by xw, 18/12/27
//	for (i = 0; i < nr_dir_blks; i++)
//	{
//		// RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
//		RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf); // modified by mingxuan 2019-5-20
//		pde = (struct dir_entry *)fsbuf;
//		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++)
//		{
//			if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
//				return pde->inode_nr;
//			if (++m > nr_dir_entries)
//				break;
//		}
//		if (m > nr_dir_entries) /* all entries have been iterated */
//			break;
//	}
//
//	/* file not found */
//	return 0;
//}
/*****************************************************************************
 *                                strip_path
 *****************************************************************************/
/**
 * Get the basename from the fullpath.
 *
 * In Orange'S FS v1.0, all files are stored in the root directory.
 * There is no sub-folder thing.
 *
 * This routine should be called at the very beginning of file operations
 * such as open(), read() and write(). It accepts the full path and returns
 * two things: the basename and a ptr of the root dir's i-node.
 *
 * e.g. After stip_path(filename, "/blah", ppinode) finishes, we get:
 *      - filename: "blah"
 *      - *ppinode: root_inode
 *      - ret val:  0 (successful)
 *
 * Currently an acceptable pathname should begin with at most one `/'
 * preceding a filename.
 *
 * Filenames may contain any character except '/' and '\\0'.
 *
 * @param[out] filename The string for the result.
 * @param[in]  pathname The full pathname.
 * @param[out] ppinode  The ptr of the dir's inode will be stored here.
 *
 * @return Zero if success, otherwise the pathname is not valid.
 *****************************************************************************/
/*
 * modified by sundong 2023.5.6
 * 增加了对目录树的支持
 */
PRIVATE int strip_path(char *filename, const char *pathname, struct inode **ppinode)
{
	const char *s = pathname;
	//	char * t = filename;

	//	if (s == 0)
	//		return -1;

	//	if (*s == '/')
	//		s++;

	//	while (*s) {		/* check each character */
	//		if (*s == '/')
	//			return -1;
	//		*t++ = *s++;
	/* if filename is too long, just truncate it */
	//		if (t - filename >= MAX_FILENAME_LEN)
	//			break;
	//	}
	//	*t = 0;

	//	*ppinode = root_inode;
	char pre_dir_name[MAX_FILENAME_LEN]; /*mod by xkx 2023-1-3*/
	memset((void *)pre_dir_name, 0, MAX_FILENAME_LEN);
	char *p = pre_dir_name; /*mod by xkx 2023-1-3*/
	*ppinode = root_inode;	/*mod by xkx 2023-1-3*/

	if (s == 0)
		return -1;
	if(*s == '/'){
		s++;
	}

	while (*s)
	{
		if (*s == '/')
		{
			if ((*ppinode)->i_mode != I_DIRECTORY) // 如果不为文件夹类型
			{
				return -1;
			}
			if (!find_dir_inode_cur_dir(ppinode, pre_dir_name))
			{
				return -1;
			}
			memset((void *)pre_dir_name, 0, MAX_FILENAME_LEN);
			p = pre_dir_name;
			s++;
			continue;
		}
		*p++ = *s++;
		if (p - pre_dir_name >= MAX_FILENAME_LEN)
			break;
	}

	memcpy(filename, pre_dir_name, MAX_FILENAME_LEN);
	return 0;
}

/*****************************************************************************
 *                                read_super_block
 *****************************************************************************/
/**
 * <Ring 1> Read super block from the given device then write it into a free
 *          super_block[] slot.
 *
 * @param dev  From which device the super block comes.
 *****************************************************************************/
// PRIVATE void read_super_block(int dev)
PUBLIC void read_super_block(int dev) // modified by mingxuan 2020-10-30
{
	int i;
	MESSAGE driver_msg;
	char *fsbuf = kern_kmalloc(BLOCK_SIZE); // local array, to substitute global fsbuf. added by xw, 18/12/27

	driver_msg.type = DEV_READ;
	driver_msg.DEVICE = dev;
	driver_msg.POSITION = BLOCK_SIZE * 1;
	driver_msg.BUF = fsbuf;
	driver_msg.CNT = BLOCK_SIZE;
	driver_msg.PROC_NR = proc2pid(p_proc_current); /// TASK_A
	// assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	// send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);
	/// zcr added
	// disp_str("In read_super_block()  before hd_rdwt\n");	/// just for debugging
	hd_rdwt(&driver_msg);
	// disp_str("In read_super_block()  after hd_rdwt\n");

	/* find a free slot in super_block[] */
	/* // deleted by mingxuan 2020-10-30
	for (i = 0; i < NR_SUPER_BLOCK; i++)
	if (super_block[i].sb_dev == NO_DEV)
			break;
	if (i == NR_SUPER_BLOCK)
		disp_str("Panic: super_block slots used up");	/// zcr modified.
	*/

	// find orange's superblock in super_block[]
	// added by mingxuan 2020-10-30
	for (i = 0; i < NR_SUPER_BLOCK; i++)
		if (super_block[i].fs_type == ORANGE_TYPE)
			break;
	if (i == NR_SUPER_BLOCK)
		disp_str("Cannot find orange's superblock!");

	// assert(i == 0); /* currently we use only the 1st slot */

	struct super_block *psb = (struct super_block *)fsbuf;

	super_block[i] = *psb;

	super_block[i].sb_dev = dev;
	super_block[i].fs_type = ORANGE_TYPE; // added by mingxuan 2020-10-30
	kern_kfree(fsbuf);
}

/*****************************************************************************
 *                                get_super_block
 *****************************************************************************/
/**
 * <Ring 1> Get the super block from super_block[].
 *
 * @param dev Device nr.
 *
 * @return Super block ptr.
 *****************************************************************************/
// PUBLIC struct super_block * get_super_block(int dev)
// {
// 	struct super_block * sb = super_block;
// 	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
// 		if (sb->sb_dev == dev)
// 			return sb;

// 	disp_str("Panic: super block of devie ");
// 	disp_int(dev);
// 	disp_str(" not found.\n");

// 	return 0;
// }

/// zcr(using hu's method.)
// PRIVATE struct super_block * get_super_block(int dev)
PUBLIC struct super_block *get_super_block(int dev) // modified by mingxuan 2020-10-30
{
	struct super_block *sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
	{
		if (sb->sb_dev == dev)
			return sb;
	}
	return 0;
}

/*****************************************************************************
 *                                get_inode
 *****************************************************************************/
/**
 * <Ring 1> Get the inode ptr of given inode nr. A cache -- inode_table[] -- is
 * maintained to make things faster. If the inode requested is already there,
 * just return it. Otherwise the inode will be read from the disk.
 *
 * @param dev Device nr.
 * @param num I-node nr.
 * @param flag if flag is I_CREATE then create inode else just search inode in inode table
 *
 * @return The inode ptr requested.
 *****************************************************************************/
// modified by sundong 2023.5.17
PRIVATE struct inode *get_inode(int dev, int num)
{
	if (num == 0)
		return 0;
	struct inode *p;
	struct inode *q = NULL;
	// 从现有的inode中查找inode
	/* 	if (flag == FIND_INODE)
		{
			for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++)
			{
				if (p->i_cnt)
				{
					if ((p->i_dev == dev) && (p->i_num == num))
					{

						return p;
					}
				}
			}
			return NULL;
		} */
	/* 	//创建新的inode
		else if (flag == CREATE_INODE)
		{

		} */
	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++)
	{
		if (p->i_cnt)
		{ /* not a free slot */
			if ((p->i_dev == dev) && (p->i_num == num))
			{
				/* this is the inode we want */
				//p->i_cnt++;
				return p;
			}
			continue;
		}
		else
		{			   /* a free slot */
			if (!q)	   /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		disp_str("Panic: the inode table is full");

	q->i_dev = dev;
	q->i_num = num;
	q->i_cnt = 1;

	struct super_block *sb = get_super_block(dev);
	int blk_nr = 1 + 1 + sb->nr_imap_blocks + sb->nr_smap_blocks + ((num - 1) / (BLOCK_SIZE / INODE_SIZE));
	//char fsbuf[SECTOR_SIZE];	 // local array, to substitute global fsbuf. added by xw, 18/12/27
	//RD_SECT(dev, blk_nr, fsbuf); // added by xw, 18/12/27

	char *fsbuf = kern_kmalloc(BLOCK_SIZE);	 // local array, to substitute global fsbuf. added by xw, 18/12/27
	RD_BLOCK(dev, blk_nr, fsbuf);
	struct inode *pinode =
		(struct inode *)((u8 *)fsbuf +
						 ((num - 1) % (BLOCK_SIZE / INODE_SIZE)) * INODE_SIZE);
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_block = pinode->i_start_block;
	q->i_nr_blocks = pinode->i_nr_blocks;
	kern_kfree(fsbuf);
	return q;
}

// modified by sundong 2023.5.17
PRIVATE struct inode *get_inode_sched(int dev, int num)
{
	if (num == 0)
		return 0;
	struct inode *p;
	struct inode *q = 0;
	/* // 从现有的inode中查找inode
	if (flag == FIND_INODE)
	{
		for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++)
		{
			if (p->i_cnt)
			{
				if ((p->i_dev == dev) && (p->i_num == num))
				{

					return p;
				}
			}
		}
		return NULL;
	}
	// 创建新的inode
	else if (flag == CREATE_INODE)
	{
		for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++)
		{
			if (p->i_cnt)
			{
				if ((p->i_dev == dev) && (p->i_num == num))
				{

					p->i_cnt++;
					return p;
				}
				continue;
			}
			else
			{
				if (!q)
					q = p;
			}
		}

		if (!q)
			disp_str("Panic: the inode table is full");

		q->i_dev = dev;
		q->i_num = num;
		q->i_cnt = 0;

		struct super_block *sb = get_super_block(dev);
		int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((num - 1) / (SECTOR_SIZE / INODE_SIZE));
		char fsbuf[SECTOR_SIZE];		   // local array, to substitute global fsbuf. added by xw, 18/12/27
		RD_SECT_SCHED(dev, blk_nr, fsbuf); // added by xw, 18/12/27
		struct inode *pinode =
			(struct inode *)((u8 *)fsbuf +
							 ((num - 1) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE);
		q->i_mode = pinode->i_mode;
		q->i_size = pinode->i_size;
		q->i_start_sect = pinode->i_start_sect;
		q->i_nr_sects = pinode->i_nr_sects;
		return q;
	} */
	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++)
	{
		if (p->i_cnt)
		{ /* not a free slot */
			if ((p->i_dev == dev) && (p->i_num == num))
			{
				/* this is the inode we want */
				//p->i_cnt++;
				return p;
			}
			continue;
		}
		else
		{			   /* a free slot */
			if (!q)	   /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		disp_str("Panic: the inode table is full");

	q->i_dev = dev;
	q->i_num = num;
	q->i_cnt = 1;

	struct super_block *sb = get_super_block(dev);
	int blk_nr = 1 + 1 + sb->nr_imap_blocks + sb->nr_smap_blocks + ((num - 1) / (BLOCK_SIZE / INODE_SIZE));
	//char fsbuf[SECTOR_SIZE];	 // local array, to substitute global fsbuf. added by xw, 18/12/27
	//RD_SECT_SCHED(dev, blk_nr, fsbuf); // added by xw, 18/12/27
	//char* fsbuf = kern_kmalloc(BLOCK_SIZE);
	//BUF_RD_BLOCK(dev,blk_nr,fsbuf);
	buf_head *bh = bread(dev,blk_nr);
	char *fsbuf = bh->buffer;
	struct inode *pinode =  
		(struct inode *)((u8 *)fsbuf +
						 ((num - 1) % (BLOCK_SIZE / INODE_SIZE)) * INODE_SIZE);
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_block = pinode->i_start_block;
	q->i_nr_blocks = pinode->i_nr_blocks;
	brelse(bh);
	//kern_kfree(fsbuf);
	return q;
}
/*****************************************************************************
 *                                put_inode
 *****************************************************************************/
/**
 * Decrease the reference nr of a slot in inode_table[]. When the nr reaches
 * zero, it means the inode is not used any more and can be overwritten by
 * a new inode.
 *
 * @param pinode I-node ptr.
 *****************************************************************************/
PRIVATE void put_inode(struct inode *pinode)
{
	// assert(pinode->i_cnt > 0);
	pinode->i_cnt--;
}

/*****************************************************************************
 *                                sync_inode
 *****************************************************************************/
/**
 * <Ring 1> Write the inode back to the disk. Commonly invoked as soon as the
 *          inode is changed.
 *
 * @param p I-node ptr.
 *****************************************************************************/
PRIVATE void sync_inode(struct inode *p)
{
	struct inode *pinode;
	struct super_block *sb = get_super_block(p->i_dev);
	int blk_nr = 1 + 1 + sb->nr_imap_blocks + sb->nr_smap_blocks + ((p->i_num - 1) / (BLOCK_SIZE / INODE_SIZE));
	//char *fsbuf = kern_kmalloc(BLOCK_SIZE);		// local array, to substitute global fsbuf. added by xw, 18/12/27
	//RD_SECT_SCHED(p->i_dev, blk_nr, fsbuf); // modified by xw, 18/12/27
	// RD_SECT(p->i_dev, blk_nr, fsbuf);	//modified by mingxuan 2019-5-20
	//BUF_RD_BLOCK(p->i_dev, blk_nr, fsbuf)
	buf_head *bh = bread(p->i_dev, blk_nr);
	char * fsbuf = bh->buffer;
	pinode = (struct inode *)((u8 *)fsbuf +
							  (((p->i_num - 1) % (BLOCK_SIZE / INODE_SIZE)) * INODE_SIZE));
	pinode->i_mode = p->i_mode;
	pinode->i_size = p->i_size;
	pinode->i_start_block = p->i_start_block;
	pinode->i_nr_blocks = p->i_nr_blocks;
	//WR_SECT_SCHED(p->i_dev, blk_nr, fsbuf); // modified by xw, 18/12/27
											// WR_SECT(p->i_dev, blk_nr, fsbuf);	//modified by mingxuan 2019-5-20
	//BUF_WR_BLOCK(p->i_dev, blk_nr, fsbuf)		
	//kern_kfree(fsbuf);
	mark_buff_dirty(bh);
	brelse(bh);								
}

/// added by zcr (from ch9/e/fs/open.c)
/*****************************************************************************
 *                                new_inode
 *****************************************************************************/
/**
 * Generate a new i-node and write it to disk.
 *
 * @param dev  Home device of the i-node.
 * @param inode_nr  I-node nr.
 * @param start_sect  Start sector of the file pointed by the new i-node.
 *
 * @return  Ptr of the new i-node.
 *****************************************************************************/
 //modified by sundong 2023.5.18
PRIVATE struct inode *new_inode(int dev, int inode_nr, int start_sect, int i_mod,int nr_sects)
{
	struct inode *new_inode = get_inode_sched(dev, inode_nr); // modified by xw, 18/8/28
	// struct inode * new_inode = get_inode(dev, inode_nr); //modified by mingxuan 2019-5-20

	new_inode->i_mode = i_mod;
	new_inode->i_size = 0;
	new_inode->i_start_block = start_sect;
	//new_inode->i_nr_sects = NR_DEFAULT_FILE_SECTS;
	new_inode->i_nr_blocks = nr_sects;

	new_inode->i_dev = dev;
	new_inode->i_cnt = 1;
	new_inode->i_num = inode_nr;

	/* write to the inode array */
	sync_inode(new_inode);

	return new_inode;
}

/*****************************************************************************
 *                                new_dir_entry
 *****************************************************************************/
/**
 * Write a new entry into the directory.
 *
 * @param dir_inode  I-node of the directory.
 * @param inode_nr   I-node nr of the new file.
 * @param filename   Filename of the new file.
 *****************************************************************************/
PRIVATE void new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename)
{
	/* write the dir_entry */
	int dir_blk0_nr = dir_inode->i_start_block;
	int nr_dir_blks = (dir_inode->i_size-1+ BLOCK_SIZE) / BLOCK_SIZE;
	int nr_dir_entries =
		dir_inode->i_size / DIR_ENTRY_SIZE; /**
											 * including unused slots
											 * (the file has been
											 * deleted but the slot
											 * is still there)
											 */
	int m = 0;
	struct dir_entry *pde;
	struct dir_entry *new_de = 0;

	int i, j;
	char *fsbuf =NULL; // kern_kmalloc(BLOCK_SIZE); // local array, to substitute global fsbuf. added by xw, 18/12/27
	buf_head *bh = NULL;
	for (i = 0; i < nr_dir_blks; i++)
	{
		//RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
		// RD_SECT(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by mingxuan 2019-5-20
		//BUF_RD_BLOCK(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);
		bh = bread(dir_inode->i_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (++m > nr_dir_entries)
				break;

			if (pde->inode_nr == 0)
			{ /* it's a free slot */
				new_de = pde;
				break;
			}
		}
		if (m > nr_dir_entries || new_de)	/* all entries have been iterated or free slot is found */
			break;
		brelse(bh);

	}
	if (!new_de)
	{ /* reached the end of the dir */
		new_de = pde;
		dir_inode->i_size += DIR_ENTRY_SIZE;
	}
	dir_inode->i_cnt++;
	new_de->inode_nr = inode_nr;
	strcpy(new_de->name, filename);
	new_de->name[strlen(filename)] = 0;

	/* write dir block -- ROOT dir block */
	//WR_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
	//BUF_WR_BLOCK(dir_inode->i_dev, dir_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
	mark_buff_dirty(bh);
	brelse(bh);
	// WR_SECT(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//added by mingxuan 2019-5-20
	//kern_kfree(fsbuf);
	/* update dir inode */
	sync_inode(dir_inode);
}

/*****************************************************************************
 *                                alloc_imap_bit
 *****************************************************************************/
/**
 * Allocate a bit in inode-map.
 *
 * @param dev  In which device the inode-map is located.
 *
 * @return  I-node nr.
 *****************************************************************************/
PRIVATE int alloc_imap_bit(int dev)
{
	/// zcr debug(output is 0x320, right)
	// disp_str("dev is: ");
	// disp_int(dev);
	// disp_str("  ");

	int inode_nr = 0;
	int i, j, k;

	int imap_blk0_nr = 1 + 1; /* 1 boot sector & 1 super block */
	struct super_block *sb = get_super_block(dev);

	/// zcr debug (output is 0x1, right.)
	// disp_str("How many the inode-map sectors: ");
	// disp_int(sb->nr_imap_sects);
	// disp_str("  ");

	//char *fsbuf = kern_kmalloc(BLOCK_SIZE);//[SECTOR_SIZE]; // local array, to substitute global fsbuf. added by xw, 18/12/27
	char *fsbuf = NULL;
	buf_head *bh = NULL;
	for (i = 0; i < sb->nr_imap_blocks; i++)
	{
		// RD_SECT(dev, imap_blk0_nr + i);		/// zcr: place the result in fsbuf?
		//RD_SECT_SCHED(dev, imap_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
		//BUF_RD_BLOCK(dev, imap_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
		bh = bread(dev, imap_blk0_nr + i);
		fsbuf = bh->buffer;
		// RD_SECT(dev, imap_blk0_nr + i, fsbuf);	//modified by mingxuan 2019-5-20
		/// zcr debug(output is 0x2, right.)
		// disp_str("imap_blk0_nr + i: ");
		// disp_int(imap_blk0_nr + i);
		// disp_str("  ");

		/// zcr debug
		// disp_str("fsbuf:");
		// for(int q = 0;q < 10;q++) {
		// 	disp_int(fsbuf[q]);
		// 	disp_str(" ");
		// }
		// disp_str("\n");

		for (j = 0; j < BLOCK_SIZE; j++)
		{
			/* skip `11111111' bytes */
			// if (fsbuf[j] == 0xFF)
			if (fsbuf[j] == '\xFF') // modified by xw, 18/12/28
				continue;

			/* skip `1' bits */
			for (k = 0; ((fsbuf[j] >> k) & 1) != 0; k++)
			{
			}
			/// zcr debug(output is 1,wrong! should be 5)
			// disp_str("k is: ");
			// disp_int(k);
			// disp_str("  ");

			// disp_str("j is: ");
			// disp_int(j);
			// disp_str("  ");

			/* i: sector index; j: byte index; k: bit index */
			inode_nr = (i * BLOCK_SIZE + j) * 8 + k;
			fsbuf[j] |= (1 << k);

			/* write the bit to imap */
			//WR_SECT_SCHED(dev, imap_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
			// WR_SECT(dev, imap_blk0_nr + i, fsbuf);	//added by mingxuan 2019-5-20
			//BUF_WR_BLOCK(dev, imap_blk0_nr + i, fsbuf);
			mark_buff_dirty(bh);
			break;
		}
		brelse(bh);
		if(inode_nr)goto alloc_end;
		
	}
alloc_end:
	//kern_kfree(fsbuf);
	if(!inode_nr)disp_str("Panic: inode-map is probably full.\n");/* no free bit in imap */
	return inode_nr;
	
}

/*****************************************************************************
 *                                alloc_smap_bit
 *****************************************************************************/
/**
 * Allocate a bit in sector-map.
 *
 * @param dev  In which device the sector-map is located.
 * @param nr_sects_to_alloc  How many sectors are allocated.
 *
 * @return  The 1st sector nr allocated.
 *****************************************************************************/
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc)
{
	/* int nr_sects_to_alloc = NR_DEFAULT_FILE_SECTS; */

	int i; /* sector index */
	int j; /* byte index */
	int k; /* bit index */

	struct super_block *sb = get_super_block(dev);

	int smap_blk0_nr = 1 + 1 + sb->nr_imap_blocks;
	int free_sect_nr = 0;
	//char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added by xw, 18/12/27
	//char* fsbuf = kern_kmalloc(BLOCK_SIZE);
	char* fsbuf = NULL;
	buf_head *bh = NULL;
	for (i = 0; i < sb->nr_smap_blocks; i++)
	{												 /* smap_blk0_nr + i :
									   current sect nr. */
		//RD_SECT_SCHED(dev, smap_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
		//BUF_RD_BLOCK(dev, smap_blk0_nr + i, fsbuf);
		bh = bread(dev, smap_blk0_nr + i);
		fsbuf = bh->buffer;
		// RD_SECT(dev, smap_blk0_nr + i, fsbuf);	//modified by mingxuan 2019-5-20

		/* byte offset in current sect */
		for (j = 0; j < BLOCK_SIZE && nr_sects_to_alloc > 0; j++)
		{
			k = 0;
			if (!free_sect_nr)
			{
				/* loop until a free bit is found */
				// if (fsbuf[j] == 0xFF) continue;
				if (fsbuf[j] == '\xFF')
					continue; // modified by xw, 18/12/28
				for (; ((fsbuf[j] >> k) & 1) != 0; k++)
				{
				}
				free_sect_nr = (i * BLOCK_SIZE + j) * 8 +
							   k - 1 + sb->n_1st_block;
			}

			for (; k < 8; k++)
			{ /* repeat till enough bits are set */
				// assert(((fsbuf[j] >> k) & 1) == 0);
				fsbuf[j] |= (1 << k);
				if (--nr_sects_to_alloc == 0)
					break;
			}
		}

		if (free_sect_nr)mark_buff_dirty(bh);								 /* free bit found, write the bits to smap */
		//	WR_SECT_SCHED(dev, smap_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
		//BUF_WR_BLOCK(dev, smap_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
		// WR_SECT(dev, smap_blk0_nr + i, fsbuf); //added by mingxuan 2019-5-20
		brelse(bh);
		if (nr_sects_to_alloc == 0)
			break;
	}

	// assert(nr_sects_to_alloc == 0);
	//kern_kfree(fsbuf);

	return free_sect_nr;
}

/// zcr added and modified from ch9/e/fs.open.c and close.c
/*****************************************************************************
 *                                close
 *****************************************************************************/
/**
 * Close a file descriptor.
 *
 * @param fd  File descriptor.
 *
 * @return Zero if successful, otherwise -1.
 *****************************************************************************/
// close is a syscall interface now. added by xw, 18/6/18
//  PUBLIC int close(int fd)
// PRIVATE int real_close(int fd)	//deleted by mingxuan 2019-5-17
PUBLIC int real_close( int fd) // modified by mingxuan 2019-5-17
{
	// return do_close(fd);	// terrible(always returns 0)
	return ora_close(fd); // modified by mingxuan 2021-8-20
}

/*****************************************************************************
 *                                do_close
 *****************************************************************************/
/**
 * Handle the message CLOSE.
 *
 * @return Zero if success.
 *****************************************************************************/
// PRIVATE int do_close(int fd)
PRIVATE int ora_close(int fd) // modified by mingxuan 2021-8-20
{
	/// zcr debug
	// disp_str("hh1 ");
	// disp_int(fd);
	// put_inode(p_proc_current->task.filp[fd]->fd_inode);	//deleted by mingxuan 2019-5-17
	put_inode(p_proc_current->task.filp[fd]->fd_node.fd_inode); // modified by mingxuan 2019-5-17
	// disp_str("hh2");
	// p_proc_current->task.filp[fd]->fd_inode = 0;	//deleted by mingxuan 2019-5-17
	p_proc_current->task.filp[fd]->fd_node.fd_inode = 0; // modified by mingxuan 2019-5-17
	p_proc_current->task.filp[fd]->flag = 0;			 // added by mingxuan 2019-5-17
	p_proc_current->task.filp[fd] = 0;

	return 0;
}

/// zcr copied from ch9/f/fs/read_write.c and modified it.

/*****************************************************************************
 *                                read
 *****************************************************************************/
/**
 * Read from a file descriptor.
 *
 * @param fd     File descriptor.
 * @param buf    Buffer to accept the bytes read.
 * @param count  How many bytes to read.
 *
 * @return  On success, the number of bytes read are returned.
 *          On error, -1 is returned.
 *****************************************************************************/
// read is a syscall interface now. added by xw, 18/6/18
//  PUBLIC int read(int fd, void *buf, int count)
//  PRIVATE int real_read(int fd, void *buf, int count)	//deleted by mingxuan 2019-5-17
PUBLIC int real_read(int fd, char *buf, int count) // 注意:buf的类型被修改成char //modified by mingxuan 2019-5-17
{
	// added by xw, 18/8/27
	MESSAGE fs_msg;

	fs_msg.type = READ;
	fs_msg.FD = fd;
	fs_msg.BUF = buf;
	fs_msg.CNT = count;
	fs_msg.source = proc2pid(p_proc_current); // added by xw, 18/12/22

	// send_recv(BOTH, TASK_FS, &msg);
	// do_rdwt(&fs_msg);
	ora_rdwt(&fs_msg); // modified by mingxuan 2021-8-20

	return fs_msg.CNT;
}

/*****************************************************************************
 *                                write
 *****************************************************************************/
/**
 * Write to a file descriptor.
 *
 * @param fd     File descriptor.
 * @param buf    Buffer including the bytes to write.
 * @param count  How many bytes to write.
 *
 * @return  On success, the number of bytes written are returned.
 *          On error, -1 is returned.
 *****************************************************************************/
// write is a syscall interface now. added by xw, 18/6/18
//  PUBLIC int write(int fd, const void *buf, int count)
//  PRIVATE int real_write(int fd, const void *buf, int count)	//deleted by mingxuan 2019-5-17
PUBLIC int real_write(int fd, const char *buf, int count) // 注意:buf的类型被修改成char //modified by mingxuan 2019-5-17
{
	// added by xw, 18/8/27
	MESSAGE fs_msg;

	fs_msg.type = WRITE;
	fs_msg.FD = fd;
	fs_msg.BUF = (void *)buf;
	fs_msg.CNT = count;
	fs_msg.source = proc2pid(p_proc_current); // added by xw, 18/12/22

	// send_recv(BOTH, TASK_FS, &msg);
	/// zcr added
	// do_rdwt(&fs_msg);
	ora_rdwt(&fs_msg); // modified by mingxuan 2021-8-20

	return fs_msg.CNT;
}

/*****************************************************************************
 *                                do_rdwt
 *****************************************************************************/
/**
 * Read/Write file and return byte count read/written.
 *
 * Sector map is not needed to update, since the sectors for the file have been
 * allocated and the bits are set when the file was created.
 *
 * @return How many bytes have been read/written.
 *****************************************************************************/
// PRIVATE int do_rdwt(MESSAGE *fs_msg)
PRIVATE int ora_rdwt(MESSAGE *fs_msg) // modified by mingxuan 2021-8-20
{
	int fd = fs_msg->FD;	 /**< file descriptor. */
	void *buf = fs_msg->BUF; /**< r/w buffer */
	int len = fs_msg->CNT;	 /**< r/w bytes */

	int src = fs_msg->source; /* caller proc nr. */

	if (!(p_proc_current->task.filp[fd]->fd_mode & O_RDWR))
		return -1;

	int pos = p_proc_current->task.filp[fd]->fd_pos;

	// struct inode * pin = p_proc_current->task.filp[fd]->fd_inode;	//deleted by mingxuan 2019-5-17
	struct inode *pin = p_proc_current->task.filp[fd]->fd_node.fd_inode; // modified by mingxuan 2019-5-17

	int imode = pin->i_mode & I_TYPE_MASK;

	if (imode == I_CHAR_SPECIAL)
	{
		int t = fs_msg->type == READ ? DEV_READ : DEV_WRITE;
		fs_msg->type = t;

		/*	//deleted by mingxuan 2019-5-19
		int dev = pin->i_start_sect;
		// assert(MAJOR(dev) == 4);
		/// zcr added
		if(MAJOR(dev) != 4) {
			disp_str("Error: MAJOR(dev) == 4\n");
		}

		fs_msg->DEVICE	= MINOR(dev);
		fs_msg->BUF	= buf;
		fs_msg->CNT	= len;
		fs_msg->PROC_NR	= src;
		// assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
		// send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &fs_msg);
		/// zcr added to replace the statement above
		hd_rdwt_sched(&fs_msg);		//modified by xw, 18/8/27
		// assert(fs_msg.CNT == len);
		*/

		// added by mingxuan 2019-5-19
		int dev = pin->i_start_block;
		int nr_tty = MINOR(dev);
		if (MAJOR(dev) != DEV_CHAR_TTY)
		{
			disp_str("Error: MAJOR(dev) != DEV_CHAR_TTY\n");
		}

		if (fs_msg->type == DEV_READ)
		{
			fs_msg->CNT = tty_read(&tty_table[nr_tty], buf, len);
		}
		else if (fs_msg->type == DEV_WRITE)
		{
			tty_write(&tty_table[nr_tty], buf, len);
		}

		return fs_msg->CNT;
	}
	else
	{
		// assert(pin->i_mode == I_REGULAR || pin->i_mode == I_DIRECTORY);
		// assert((fs_msg.type == READ) || (fs_msg.type == WRITE));

		int pos_end;
		if (fs_msg->type == READ)
			pos_end = min(pos + len, pin->i_size);
		else /* WRITE */
			pos_end = min(pos + len, pin->i_nr_blocks * BLOCK_SIZE);

		int off = pos % BLOCK_SIZE;
		int rw_sect_min = pin->i_start_block + (pos >> BLOCK_SIZE_SHIFT);
		int rw_sect_max = pin->i_start_block + (pos_end >> BLOCK_SIZE_SHIFT);

		// modified by xw, 18/12/27
		// int chunk = min(rw_sect_max - rw_sect_min + 1,
		//		FSBUF_SIZE >> SECTOR_SIZE_SHIFT);
		int chunk = min(rw_sect_max - rw_sect_min + 1,
						BLOCK_SIZE >> BLOCK_SIZE_SHIFT);

		int bytes_rw = 0;
		int bytes_left = len;
		int i;

		//char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added by xw, 18/12/27
		//char *fsbuf = kern_kmalloc(BLOCK_SIZE);
		char *fsbuf = NULL;
		buf_head * bh = NULL;
		//char fsbuf[BLOCK_SIZE];
		for (i = rw_sect_min; i <= rw_sect_max; i += chunk)
		{
			/* read/write this amount of bytes every time */
			int bytes = min(bytes_left, chunk * BLOCK_SIZE - off);
//			rw_sector_sched(DEV_READ, // modified by xw, 18/8/27
//									  // rw_sector(DEV_READ,	//modified by mingxuan 2019-5-21
//							pin->i_dev,
//							i * SECTOR_SIZE,
//							chunk * SECTOR_SIZE,
//							proc2pid(p_proc_current), /// TASK_FS
//							fsbuf);
			//RD_BLOCK_SCHED(pin->i_dev,i,fsbuf);

			//BUF_RD_BLOCK(pin->i_dev,i,fsbuf);
			bh = bread(pin->i_dev,i);
			fsbuf = bh->buffer;
			if (fs_msg->type == READ)
			{
				if (p_proc_current->task.filp[fd]->fd_pos + bytes > pos_end)
				{ // added by xiaofeng 2021-9-2
					bytes = pos_end - p_proc_current->task.filp[fd]->fd_pos;
				}
				phys_copy((void *)va2la(src, buf + bytes_rw),
						  (void *)va2la(proc2pid(p_proc_current), fsbuf + off),
						  bytes);
			}
			else
			{ /* WRITE */
				phys_copy((void *)va2la(proc2pid(p_proc_current), fsbuf + off),
						  (void *)va2la(src, buf + bytes_rw),
						  bytes);

//				rw_sector_sched(DEV_WRITE, // modified by xw, 18/8/27
//										   // rw_sector(DEV_WRITE,	//modified by mingxuan 2019-5-21
//								pin->i_dev,
//								i * SECTOR_SIZE,
//								chunk * SECTOR_SIZE,
//								proc2pid(p_proc_current),
//								fsbuf);
				//BUF_WR_BLOCK(pin->i_dev,i,fsbuf);
				mark_buff_dirty(bh);
			}
			off = 0;
			bytes_rw += bytes;
			p_proc_current->task.filp[fd]->fd_pos += bytes;
			bytes_left -= bytes;
			brelse(bh);
		}

		if (p_proc_current->task.filp[fd]->fd_pos > pin->i_size)
		{
			/* update inode::size */
			pin->i_size = p_proc_current->task.filp[fd]->fd_pos;

			/* write the updated i-node back to disk */
			sync_inode(pin);
		}
		//kern_kfree(fsbuf);

		return bytes_rw;
	}
}

/// zcr copied from ch9/h/lib/unlink.c and modified it

/*****************************************************************************
 *                                unlink
 *****************************************************************************/
/**
 * Delete a file.
 *
 * @param pathname  The full path of the file to delete.
 *
 * @return Zero if successful, otherwise -1.
 *****************************************************************************/
// unlink is a syscall interface now. added by xw, 18/6/19
//  PUBLIC int unlink(const char * pathname)
//  PRIVATE int real_unlink(const char * pathname)	//deleted by mingxuan 2019-5-17
PUBLIC int real_unlink(struct super_block *sb, const char *pathname) // modified by mingxuan 2019-5-17
{
	// added by xw, 18/8/27
	MESSAGE fs_msg;

	fs_msg.type = UNLINK;
	fs_msg.PATHNAME = (void *)pathname;
	fs_msg.NAME_LEN = strlen(pathname);
	fs_msg.source = proc2pid(p_proc_current); // added by xw, 18/12/22

	// send_recv(BOTH, TASK_FS, &msg);

	// return fs_msg.RETVAL;
	/// zcr added
	// return do_unlink(&fs_msg);
	return ora_unlink(&fs_msg); // modified by mingxuan 2021-8-20
}

/// zcr copied from the ch9/h/fs/link.c and modified it
/*****************************************************************************
 *                                do_unlink
 *****************************************************************************/
/**
 * Remove a file.
 *
 * @note We clear the i-node in inode_array[] although it is not really needed.
 *       We don't clear the data bytes so the file is recoverable.
 *
 * @return On success, zero is returned.  On error, -1 is returned.
 *****************************************************************************/
// PRIVATE int do_unlink(MESSAGE *fs_msg)
PRIVATE int ora_unlink(MESSAGE *fs_msg) // modified by mingxuan 2021-8-20
{
	char pathname[MAX_PATH];

	/* get parameters from the message */
	int name_len = fs_msg->NAME_LEN; /* length of filename */
	int src = fs_msg->source;		 /* caller proc nr. */
	// assert(name_len < MAX_PATH);
	phys_copy((void *)va2la(proc2pid(p_proc_current), pathname),
			  (void *)va2la(src, fs_msg->PATHNAME),
			  name_len);
	pathname[name_len] = 0;

	if (strcmp(pathname, "/") == 0)
	{
		/// zcr
		// disp_str("FS:do_unlink():: cannot unlink the root\n");
		disp_str("FS:ora_unlink():: cannot unlink the root\n"); // modified by mingxuan 2021-8-20
		return -1;
	}

	int inode_nr = search_file(pathname);
	if (inode_nr == INVALID_INODE)
	{ /* file not found */
		/// zcr
		// disp_str("FS::do_unlink():: search_file() returns invalid inode: ");
		disp_str("FS::ora_unlink():: search_file() returns invalid inode: "); // modified by mingxuan 2021-8-20
		disp_str(pathname);
		disp_str("\n");
		return -1;
	}

	char filename[MAX_PATH];
	struct inode *dir_inode;
	if (strip_path(filename, pathname, &dir_inode) != 0)
		return -1;

	struct inode *pin = get_inode_sched(dir_inode->i_dev, inode_nr); // modified by xw, 18/8/28

	if (pin->i_mode != I_REGULAR)
	{ /* can only remove regular files */
		// printl("cannot remove file %s, because it is not a regular file.\n", pathname);
		/// zcr
		disp_str("cannot remove file ");
		disp_str(pathname);
		disp_str(", because it is not a regular file.\n");
		return -1;
	}

	if (pin->i_cnt > 1)
	{ /* the file was opened */
		// printl("cannot remove file %s, because pin->i_cnt is %d.\n", pathname, pin->i_cnt);
		/// zcr
		disp_str("cannot remove file ");
		disp_str(pathname);
		disp_str(", because pin->i_cnt is ");
		disp_int(pin->i_cnt);
		disp_str("\n");
		return -1;
	}

	struct super_block *sb = get_super_block(pin->i_dev);

	/*************************/
	/* free the bit in i-map */
	/*************************/
	free_imap_bit(pin->i_dev, pin->i_num);

	// int byte_idx = inode_nr / 8;
	// int bit_idx = inode_nr % 8;
	//  assert(byte_idx < SECTOR_SIZE);	/* we have only one i-map sector */
	/* read sector 2 (skip bootsect and superblk): */
	// char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27
	// RD_SECT_SCHED(pin->i_dev, 2, fsbuf);		//modified by xw, 18/12/27
	//  assert(fsbuf[byte_idx % SECTOR_SIZE] & (1 << bit_idx));
	// fsbuf[byte_idx % SECTOR_SIZE] &= ~(1 << bit_idx);
	// WR_SECT_SCHED(pin->i_dev, 2, fsbuf);	//modified by xw, 18/12/27

	/**************************/
	/* free the bits in s-map */
	/**************************/
	/*
	 *           bit_idx: bit idx in the entire i-map
	 *     ... ____|____
	 *                  \        .-- byte_cnt: how many bytes between
	 *                   \      |              the first and last byte
	 *        +-+-+-+-+-+-+-+-+ V +-+-+-+-+-+-+-+-+
	 *    ... | | | | | |*|*|*|...|*|*|*|*| | | | |
	 *        +-+-+-+-+-+-+-+-+   +-+-+-+-+-+-+-+-+
	 *         0 1 2 3 4 5 6 7     0 1 2 3 4 5 6 7
	 *  ...__/
	 *      byte_idx: byte idx in the entire i-map
	 */
	free_smap_bit(pin->i_dev, pin->i_start_block, pin->i_nr_blocks);
	// bit_idx  = pin->i_start_sect - sb->n_1st_sect + 1;
	// byte_idx = bit_idx / 8;
	// int bits_left = pin->i_nr_sects;
	// int byte_cnt = (bits_left - (8 - (bit_idx % 8))) / 8;

	/* current sector nr. */
	// int s = 2  /* 2: bootsect + superblk */
	//	+ sb->nr_imap_sects + byte_idx / SECTOR_SIZE;

	// RD_SECT_SCHED(pin->i_dev, s, fsbuf);		//modified by xw, 18/12/27

	// int i;
	/* clear the first byte */
	// for (i = bit_idx % 8; (i < 8) && bits_left; i++,bits_left--) {
	//  assert((fsbuf[byte_idx % SECTOR_SIZE] >> i & 1) == 1);
	//	fsbuf[byte_idx % SECTOR_SIZE] &= ~(1 << i);
	//}

	/* clear bytes from the second byte to the second to last */
	// int k;
	// i = (byte_idx % SECTOR_SIZE) + 1;	/* the second byte */
	// for (k = 0; k < byte_cnt; k++,i++,bits_left-=8) {
	//	if (i == SECTOR_SIZE) {
	//		i = 0;
	//		WR_SECT_SCHED(pin->i_dev, s, fsbuf);		//modified by xw, 18/12/27
	//		RD_SECT_SCHED(pin->i_dev, ++s, fsbuf);		//modified by xw, 18/12/27
	//	}
	//  assert(fsbuf[i] == 0xFF);
	//	fsbuf[i] = 0;
	//}

	/* clear the last byte */
	// if (i == SECTOR_SIZE) {
	//	i = 0;
	//	WR_SECT_SCHED(pin->i_dev, s, fsbuf);			//modified by xw, 18/12/27
	//	RD_SECT_SCHED(pin->i_dev, ++s, fsbuf);			//modified by xw, 18/12/27
	// }
	// unsigned char mask = ~((unsigned char)(~0) << bits_left);
	//  assert((fsbuf[i] & mask) == mask);
	// fsbuf[i] &= (~0) << bits_left;
	// WR_SECT_SCHED(pin->i_dev, s, fsbuf);				//modified by xw, 18/12/27

	/***************************/
	/* clear the i-node itself */
	/***************************/
	pin->i_mode = 0;
	pin->i_size = 0;
	pin->i_start_block = 0;
	pin->i_nr_blocks = 0;
	sync_inode(pin);
	memset(pin, 0, sizeof(INODE_SIZE));
	/* 减少父目录的inode引用数量 */
	put_inode(dir_inode);

	/************************************************/
	/* set the inode-nr to 0 in the directory entry */
	/************************************************/
	int dir_blk0_nr = dir_inode->i_start_block;
	int nr_dir_blks = (dir_inode->i_size-1 + BLOCK_SIZE) / BLOCK_SIZE;
	int nr_dir_entries =
		dir_inode->i_size / DIR_ENTRY_SIZE; /* including unused slots
											 * (the file has been
											 * deleted but the slot
											 * is still there)
											 */
	int m = 0;
	struct dir_entry *pde = 0;
	int dir_size = 0;
	bool finish_unlink = false;
	//char fsbuf[SECTOR_SIZE];
	//char* fsbuf = kern_kmalloc(BLOCK_SIZE);
	char* fsbuf = NULL;
	buf_head *bh = NULL;
	for (int i = 0; i < nr_dir_blks; i++)
	{
		//RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
		//BUF_RD_BLOCK(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);
		bh = bread(dir_inode->i_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		int j;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (pde->inode_nr == INVALID_INODE)
				continue; // 有的目录项被删除了 此时pde指向的内存是空的，应当跳过去
			if (++m > nr_dir_entries)
				break;

			if (pde->inode_nr == inode_nr)
			{
				/* pde->inode_nr = 0; */
				memset(pde, 0, DIR_ENTRY_SIZE);
				//WR_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf); // modified by xw, 18/12/27
				//BUF_WR_BLOCK(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);
				mark_buff_dirty(bh);
				brelse(bh);
				finish_unlink = true;
				dir_inode->i_size -= DIR_ENTRY_SIZE;
				sync_inode(dir_inode);
				break;
			}
			/*
						if (pde->inode_nr != INVALID_INODE)
							dir_size += DIR_ENTRY_SIZE; */
		}
		brelse(bh);

		if (m > nr_dir_entries || finish_unlink) /* all entries have been iterated or 已经把文件删掉 */
			break;
	}
	// assert(flg);
	//	if (m == nr_dir_entries) { /* the file is the last one in the dir */
	//		dir_inode->i_size = dir_size;
	//		sync_inode(dir_inode);
	//	}
	//kern_kfree(fsbuf);
	return finish_unlink ? 0 : -1;
}

/// zcr defined
// lseek is a syscall interface now. added by xw, 18/6/18
//  PUBLIC int lseek(int fd, int offset, int whence)
//  PRIVATE int real_lseek(int fd, int offset, int whence)	//deleted by mingxuan 2019-5-17
PUBLIC int real_lseek(int fd, int offset, int whence) // modified by mingxuan 2019-5-17
{
	// added by xw, 18/8/27
	MESSAGE fs_msg;

	fs_msg.FD = fd;
	fs_msg.OFFSET = offset;
	fs_msg.WHENCE = whence;

	// return do_lseek(&fs_msg);
	return ora_lseek(&fs_msg); // modified by mingxuan 2021-8-20
}
/// zcr copied from ch9/j/fs/open.c
/*****************************************************************************
 *                                do_lseek
 *****************************************************************************/
/**
 * Handle the message LSEEK.
 *
 * @return The new offset in bytes from the beginning of the file if successful,
 *         otherwise a negative number.
 *****************************************************************************/
// PRIVATE int do_lseek(MESSAGE *fs_msg)
PRIVATE int ora_lseek(MESSAGE *fs_msg) // modified by mingxuan 2021-8-20
{
	int fd = fs_msg->FD;
	int off = fs_msg->OFFSET;
	int whence = fs_msg->WHENCE;

	int pos = p_proc_current->task.filp[fd]->fd_pos;
	// int f_size = p_proc_current->task.filp[fd]->fd_inode->i_size; //deleted by mingxuan 2019-5-17
	int f_size = p_proc_current->task.filp[fd]->fd_node.fd_inode->i_size; // modified by mingxuan 2019-5-17

	switch (whence)
	{
	case SEEK_SET:
		pos = off;
		break;
	case SEEK_CUR:
		pos += off;
		break;
	case SEEK_END:
		pos = f_size + off;
		break;
	default:
		return -1;
		break;
	}
	if ((pos > f_size) || (pos < 0))
	{
		return -1;
	}
	p_proc_current->task.filp[fd]->fd_pos = pos;
	return pos;
}

// added by xw, 18/6/18
/*	//deleted by mingxuan 2019-5-17
PUBLIC int sys_open(void *uesp)
{
	return real_open(get_arg(uesp, 1),
					 get_arg(uesp, 2));
}

PUBLIC int sys_close(void *uesp)
{
	return real_close(get_arg(uesp, 1));
}

PUBLIC int sys_read(void *uesp)
{
	return real_read(get_arg(uesp, 1),
					 get_arg(uesp, 2),
					 get_arg(uesp, 3));
}

PUBLIC int sys_write(void *uesp)
{
	return real_write(get_arg(uesp, 1),
					  get_arg(uesp, 2),
					  get_arg(uesp, 3));
}

PUBLIC int sys_lseek(void *uesp)
{
	return real_lseek(get_arg(uesp, 1),
					  get_arg(uesp, 2),
					  get_arg(uesp, 3));
}
//~xw, 18/6/18

//added by xw, 18/6/19
PUBLIC int sys_unlink(void *uesp)
{
	return real_unlink(get_arg(uesp, 1));
}
*/

/*add by xkx 2023-1-3*/
/**
 * @brief 在当前ppinode目录下寻找dir_name目录，更改ppinode指向dir_name目录对应的inode
 *
 * @param ppinode
 * @param dir_name
 * @return int 如果查找失败返回0,如果查找成功返回1
 */
static int find_dir_inode_cur_dir(struct inode **ppinode, char *dir_name)
{
	if (dir_name == 0)
	{
		return 0;
	}
	struct inode **cur_dir_inode = ppinode;
	int nr_dir_blks = ((*cur_dir_inode)->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

	int dir_blk0_nr = (*cur_dir_inode)->i_start_block;
	int nr_dir_entries = (*cur_dir_inode)->i_size / DIR_ENTRY_SIZE;

	int m = 0;
	struct dir_entry *pde;
	int i, j;
	//char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf.
	//char * fsbuf = kern_kmalloc(BLOCK_SIZE); // local array, to substitute global fsbuf.
	char * fsbuf = NULL;
	buf_head * bh = NULL;
	for (i = 0; i < nr_dir_blks; i++)
	{
		//RD_SECT_SCHED((*cur_dir_inode)->i_dev, dir_blk0_nr + i, fsbuf); // modified by mingxuan 2019-5-20
		//BUF_RD_BLOCK((*cur_dir_inode)->i_dev, dir_blk0_nr + i, fsbuf);
		bh = bread((*cur_dir_inode)->i_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (pde->inode_nr == INVALID_INODE)
			{
				continue;
			}
			if (++m > nr_dir_entries)
			{
				return 0;
			}

			if (!strcmp(pde->name, dir_name))
			{
				/*modified by gfx 2023-1-13*/
				// if ((*cur_dir_inode)->i_num != 1)//if not root inode
				// 	put_inode(*cur_dir_inode);
				*ppinode = get_inode_sched((*cur_dir_inode)->i_dev, pde->inode_nr);
				brelse(bh);
				//kern_kfree(fsbuf);
				return 1;
			}
		}
		brelse(bh);
		
	}

	//kern_kfree(fsbuf);
	return 0;
}

/*add by xkx 2023-1-3*/
/**
 * @brief
 *
 * @param ppinode
 * @param pathname
 * @return int
 */
// 利用绝对路径寻找目录，更改ppinode，绝对路径的结束应该是个目录
static int find_dir_inode(struct inode **ppinode, char *pathname)
{

	char dir_name[MAX_PATH];

	int str_out = strip_path(dir_name, pathname, ppinode);

	if (str_out == -1)
	{
		return 0;
	}
	else
	{
		if (strlen(dir_name) && !find_dir_inode_cur_dir(ppinode, dir_name))
		{
			return 0;
		}
	}

	return 1;
}

/**************************************************************************************************
 *                                search_file_in_dir
 *************************************************************************************************/
/**
 * Search the file/directory in a certain directory and return the inode_nr.
 *
 * @param	name name of the file/directory
 * @param	dir_inode pointer of inode of the father directory
 * @return  inode number of the i-node of the file/directory if successful, otherwise zero.
 *
 *************************************************************************************************/
static int search_file_in_dir(char *name, struct inode *dir_inode) /*added by gfx 2023-2-18*/
{
	/* 	disp_str("\nsearch_file_in_dir:");
		disp_str(name); */
	int i, j;
	if (name[0] == 0) /* path: "/" */
		return dir_inode->i_num;

	/**
	 * Search the dir for the file.
	 */
	int dir_blk0_nr = dir_inode->i_start_block;
	int nr_dir_blks = (dir_inode->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
	int nr_dir_entries =
		dir_inode->i_size / DIR_ENTRY_SIZE; /**
											 * including unused slots
											 * (the file has been deleted
											 * but the slot is still there)
											 */
	int m = 0;
	struct dir_entry *pde;
	//char *fsbuf = kern_kmalloc(BLOCK_SIZE); // local array, to substitute global fsbuf. added by xw, 18/12/27
	char *fsbuf = NULL;
	buf_head *bh = NULL;
	for (i = 0; i < nr_dir_blks; i++)
	{
		// RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
		//BUF_RD_BLOCK(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);
		bh = bread(dir_inode->i_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (pde->inode_nr == INVALID_INODE)
			{
				continue;
			}
			if (strcmp(name, pde->name) == 0){
				brelse(bh);
				return pde->inode_nr;
			}
				
			if (++m >= nr_dir_entries){
				brelse(bh);
				break;
			}
				
		}
		brelse(bh);
		if (m >= nr_dir_entries) /* all entries have been iterated */
			break;
	}
	//kern_kfree(fsbuf);
	return 0;
}
/*add by gfx 2022-1-14*/
/*****************************************************************************
 *                                find_max_i_cnt
 *****************************************************************************/
/**
 * @brief find max i_cnt of a directory's file tree
 * @return max_i_cnt
 *****************************************************************************/
static int find_max_i_cnt(struct inode *dir_inode)
{

	int max_icnt = 0;

	int i;
	int j;
	int m = 0;

	struct dir_entry *pde;
	//char *fsbuf = (char *)kern_kmalloc(BLOCK_SIZE); // 减少内核栈空间使用,改用堆空间
	char *fsbuf = NULL;
	buf_head *bh = NULL;

	int dir_blk0_nr = dir_inode->i_start_block;
	int nr_dir_entries = dir_inode->i_size / DIR_ENTRY_SIZE;
	for (i = 0; i < dir_inode->i_nr_blocks; i++)
	{
		//RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf); // modified by mingxuan 2019-5-20
		bh = bread(dir_inode->i_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (pde->inode_nr == INVALID_INODE)
			{
				continue;
			}
			if (!strcmp(pde->name, ".") || !strcmp(pde->name, ".."))
			{
				++m;
				continue;
			}
			else
			{
				struct inode *sub_inode;
				int sub_max_icnt;

				// dir_inode->i_cnt++;
				sub_inode = get_inode_sched(dir_inode->i_dev, pde->inode_nr);
				// put_inode(sub_inode);
				// put_inode(dir_inode);
				if (sub_inode->i_mode == I_DIRECTORY)
				{
					sub_max_icnt = find_max_i_cnt(sub_inode);
				}
				else if (sub_inode->i_mode == I_REGULAR) // 其他类型暂且统一处理
				{
					sub_max_icnt = sub_inode->i_cnt;
				}
				if (sub_max_icnt > max_icnt)
				{
					max_icnt = sub_max_icnt;
				}
			}

			if (++m >= nr_dir_entries)
				break;
		}
		brelse(bh);

		if (m >= nr_dir_entries) /* all entries have been iterated */
			break;
	}
	//kern_kfree((u32)fsbuf);
	return max_icnt;
}

/*add by gfx 2022-1-15*/
/*****************************************************************************
 *                                do_deletecheck
 *****************************************************************************/
/**
 * @brief check whether can delete a directory (only support directory)
 * @return 0 if can delete ,else return a negative error code
 *****************************************************************************/
static int do_deletecheck(MESSAGE *fs_msg)
{
	char pathname[MAX_PATH] = {0};

	int name_len = fs_msg->NAME_LEN; /* length of dirname */
	int src = fs_msg->source;		 /* caller proc nr. */

	memcpy((void *)va2la(src, pathname), (void *)va2la(src, fs_msg->PATHNAME), name_len);
	pathname[name_len] = 0;

	char dirname[MAX_FILENAME_LEN];
	struct inode *updir_inode;
	if (strlen(pathname) == 0) // 根目录
	{
		return DIR_PATH_INEXISTE; // 不能删掉根目录
	}
	if (strip_path(dirname, pathname, &updir_inode) != 0)
		return DIR_PATH_INEXISTE;
	int inode_nr = search_file_in_dir(dirname, updir_inode);
	if (inode_nr == INVALID_INODE) // 路径不存在
	{							   /* file not found */
		return DIR_PATH_INEXISTE;
	}

	struct inode *dir_inode;

	// updir_inode->i_cnt++;
	dir_inode = get_inode_sched(updir_inode->i_dev, inode_nr);
	// put_inode(updir_inode);
	if (dir_inode->i_mode != I_DIRECTORY) // 如果不为目录，也不能删去
	{
		// put_inode(dir_inode);
		return DIR_PATH_INEXISTE;
	}
	/* 	if ((find_max_i_cnt(dir_inode)) > 1) // 如果以dir_inode为根的文件树里存在i_cnt大于等于1的文件节点
		{
			//put_inode(dir_inode);
			return DIR_FILE_OCCUPIYED;
		} */
	if (dir_inode->i_cnt > 1)
	{ // 如果目录下有其他文件，则无法删除
		return DIR_FILE_OCCUPIYED;
	}
	// put_inode(dir_inode);
	return 0;
}

/*****************************************************************************
 *                                do_deletedir
 *****************************************************************************/
/**
 * delete a dir and return if successful
 *
 * @return 0 if successful, otherwise a negative error code.
 *****************************************************************************/
/*add by xkx 2023-1-3*/
static int ora_deletedir(MESSAGE *fs_msg)
{
	char pathname[MAX_PATH] = {0};

	int name_len = fs_msg->NAME_LEN; /* length of dirname */
	int src = fs_msg->source;		 /* caller proc nr. */

	memcpy((void *)va2la(src, pathname), (void *)va2la(src, fs_msg->PATHNAME), name_len);
	pathname[name_len] = 0;

	char dirname[MAX_FILENAME_LEN];
	struct inode *updir_inode;
	struct inode *dir_inode;
	if (strip_path(dirname, pathname, &updir_inode) != 0)
		return DIR_PATH_INEXISTE;
	int inode_nr = search_file_in_dir(dirname, updir_inode);

	// updir_inode->i_cnt++;
	dir_inode = get_inode_sched(updir_inode->i_dev, inode_nr);
	// put_inode(updir_inode);

	// put_inode(dir_inode);//此处不put

	/*modified by gfx 2022-1-14*/
	/***********************************************/
	/* clear directory entry and sub director/file */
	/***********************************************/
	int father_i_nr; /*记录父inode*/
	int dir_blk0_nr = dir_inode->i_start_block;
	int nr_dir_entries = dir_inode->i_size / DIR_ENTRY_SIZE;

	int i;
	int j;

	int m = 0;
	struct dir_entry *pde;
	// char fsbuf[SECTOR_SIZE];
	//char *fsbuf = (char *)kern_kmalloc(BLOCK_SIZE); // 减少内核栈空间使用,改用堆空间
	char *fsbuf = NULL;
	buf_head *bh = NULL;
	// for (i = 0; i < nr_dir_blks; i++)/*modified by gfx 2022-1-14*/
	for (i = 0; i < dir_inode->i_nr_blocks; i++)
	{
		//BUF_RD_BLOCK(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);
		bh = bread(dir_inode->i_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (pde->inode_nr == INVALID_INODE)
			{
				continue;
			}
			/*对于cur和pre目录要单独处理*/
			if (strcmp(pde->name, ".") == 0)
			{
				pde->inode_nr = 0;
				memset(pde, 0, DIR_ENTRY_SIZE); // just for easy to see ,can delete
				dir_inode->i_size -= DIR_ENTRY_SIZE;
			}
			else if (strcmp(pde->name, "..") == 0)
			{
				father_i_nr = pde->inode_nr; // 记录下父目录inode nr
				pde->inode_nr = 0;
				memset(pde, 0, DIR_ENTRY_SIZE); // just for easy to see ,can delete
				dir_inode->i_size -= DIR_ENTRY_SIZE;
				// 在处理完".."的时候同步到磁盘,因为"."在前面处理完了
				//BUF_WR_BLOCK(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);
				mark_buff_dirty(bh);
			}
			// 其他子目录和文件
			else
			{
				disp_str("delete dir error: dir is not empty!");
				brelse(bh);
				return -1;
				//modified by sundong 2023.5.19
				//暂时不支持级联删除（当目录下有文件or目录时无法删除该目录）
				/* struct inode *sub_inode;
				char sub_pathname[MAX_PATH] = {0};
				// snprintf(sub_pathname,MAX_PATH,"%s/%s",pathname,pde->name);
				memcpy(sub_pathname, pathname, strlen(pathname));
				sub_pathname[strlen(pathname)] = '/';
				memcpy(sub_pathname + strlen(pathname) + 1, pde->name, strlen(pde->name));

				MESSAGE fs_msg;
				fs_msg.PATHNAME = (void *)sub_pathname;
				fs_msg.NAME_LEN = strlen(sub_pathname);
				fs_msg.source = proc2pid(p_proc_current);

				// 获取子inode
				sub_inode = get_inode(dir_inode->i_dev, pde->inode_nr,FIND_INODE);
				//put_inode(sub_inode);
				if (sub_inode->i_mode == I_DIRECTORY)
				{
					fs_msg.type = DELETEDIR;
					if (do_deletedir(&fs_msg))
					{
						disp_str("[do_deletedir]: delete sub dir failed\n");
					}
				}
				else if (sub_inode->i_mode == I_REGULAR) // 其他统一处理
				{
					fs_msg.type = UNLINK;
					if (ora_unlink(&fs_msg))
					{
						disp_str("[do_deletedir]: delete sub file failed\n");
					}
				disp_str("[do_deletedir]: just can delete dir\n");

				} */
			}
			if (++m >= nr_dir_entries)
				break;
		}
		brelse(bh);
		if (m >= nr_dir_entries) /* all entries have been iterated */
			break;
	}

	/*clear entry in father directory*/
	struct inode *fath_inode = updir_inode;
	// fath_inode = get_inode(dir_inode->i_dev, father_i_nr,FIND_INODE);
	// put_inode(fath_inode);
	int fath_dir_blk0_nr = fath_inode->i_start_block;
	int fath_nr_dir_entries = fath_inode->i_size / DIR_ENTRY_SIZE;
	m = 0;
	for (i = 0; i < fath_inode->i_nr_blocks; i++)
	{
		bh = bread(fath_inode->i_dev, fath_dir_blk0_nr + i);
		fsbuf = bh->buffer;
		//BUF_RD_BLOCK(fath_inode->i_dev, fath_dir_blk0_nr + i, fsbuf);
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (pde->inode_nr == INVALID_INODE)
			{
				continue;
			}
			if (pde->inode_nr == dir_inode->i_num) // 如果在父目录文件找到此目录的pde
			{
				pde->inode_nr = 0;
				memset(pde, 0, DIR_ENTRY_SIZE); // just for easy to see ,can delete
				mark_buff_dirty(bh);
				//BUF_WR_BLOCK(fath_inode->i_dev, fath_dir_blk0_nr + i, fsbuf);
			}

			if (++m >= fath_nr_dir_entries)
				break;
		}
		brelse(bh);
		if (m >= fath_nr_dir_entries) /* all entries have been iterated */
			break;
	}
	fath_inode->i_size -= DIR_ENTRY_SIZE; /*调整父目录的文件大小*/
	// 减少父目录下的 i_cnt值的引用
	put_inode(fath_inode);
	sync_inode(fath_inode);

	/*******************************/
	/* free the bit in i-map/s-map */
	/*******************************/
	free_smap_bit(dir_inode->i_dev, dir_inode->i_start_block, NR_DEFAULT_FILE_BLOCKS);
	free_imap_bit(dir_inode->i_dev, dir_inode->i_num);

	/***************************/
	/* clear the i-node itself */
	/***************************/
	dir_inode->i_mode = 0;
	dir_inode->i_size = 0;
	dir_inode->i_start_block = 0;
	dir_inode->i_nr_blocks = 0;
	sync_inode(dir_inode);
	// dir_inode->i_num = 0;
	memset(dir_inode, 0, INODE_SIZE);
	//kern_kfree((u32)fsbuf);

	return 0;
}

/*****************************************************************************
 *                                do_opendir
 *****************************************************************************/
/**
 * open a dir and return if successful
 *
 * @return 0 if successful, otherwise a negative error code.
 *****************************************************************************/
/*add by xkx 2023-1-3*/
static int ora_opendir(MESSAGE *fs_msg)
{
	char pathname[MAX_PATH];

	int name_len = fs_msg->NAME_LEN; /* length of dirname */
	int src = fs_msg->source;		 /* caller proc nr. */

	memcpy((void *)va2la(src, pathname), (void *)va2la(src, fs_msg->PATHNAME), name_len);
	pathname[name_len] = 0;

	char dirname[MAX_FILENAME_LEN];
	struct inode *updir_inode;
	struct inode *dir_inode;
	if (strip_path(dirname, pathname, &updir_inode) != 0) // 如果没找到，则返回1
	{
		return DIR_PATH_INEXISTE;
	}

	int inode_nr = search_file_in_dir(dirname, updir_inode);
	if (!inode_nr)
	{
		return DIR_PATH_INEXISTE;
	}

	struct inode *found_inode = 0; /*add by gfx 2023-1-13*/

	dir_inode = get_inode_sched(updir_inode->i_dev, inode_nr); // modified by gfx 2023-1-13
	// put_inode(dir_inode); /*保持文件夹的i_cnt始终为0*/	 // modified by gfx 2023-1-13
	if (dir_inode->i_mode != I_DIRECTORY) // 如果不为文件夹类型
	{
		return DIR_PATH_INEXISTE;
	}

	return 0;
}

/*****************************************************************************
 *                                do_createdir
 *****************************************************************************/
/**
 * create a dir and return if successful
 *
 * @return 0 if successful, otherwise a negative error code.
 *****************************************************************************/
/*add by xkx 2023-1-3*/
static int ora_createdir(MESSAGE *fs_msg)
{

	char pathname[MAX_PATH] = {0};

	int name_len = fs_msg->NAME_LEN; /* length of filename */
	int src = fs_msg->source;		 /* caller proc nr. */

	memcpy((void *)va2la(src, pathname), (void *)va2la(src, fs_msg->PATHNAME), name_len);
	pathname[name_len] = 0;

	char dirname[MAX_FILENAME_LEN];
	struct inode *updir_inode;
	/*modified by gfx 2023-1-13*/
	if (strip_path(dirname, pathname, &updir_inode) != 0)
	{
		return DIR_PATH_INEXISTE;
	}
	int inode_nr = search_file_in_dir(dirname, updir_inode);
	if (inode_nr) // 如果当前目录已存在
	{
		return DIR_PATH_REPEATED;
	}

	int new_inode_nr = alloc_imap_bit(updir_inode->i_dev);
	int free_sect_nr = alloc_smap_bit(updir_inode->i_dev, NR_DEFAULT_FILE_BLOCKS);

	// updir_inode->i_cnt++; // modified by gfx 2023-1-13
	struct inode *newino = new_inode(updir_inode->i_dev, new_inode_nr, free_sect_nr, I_DIRECTORY,NR_DEFAULT_FILE_BLOCKS);
	// put_inode(updir_inode);

	new_dir_entry(updir_inode, newino->i_num, dirname);

	set_dir_entry(newino, updir_inode->i_num);
	return 0;
}
/*****************************************************************************
 *                                do_showdir
 *****************************************************************************/
/**
 * show the content of a dir and return if successful
 *
 * @return 0 if successful, otherwise -1.
 *****************************************************************************/
/*add by gfx 2023-2-13*/
static int ora_showdir(MESSAGE *fs_msg)
{
	char pathname[MAX_PATH] = {0};
	char dirname[MAX_FILENAME_LEN] = {0};

	int name_len = fs_msg->NAME_LEN; /* length of dirname */
	int src = fs_msg->source;		 /* caller proc nr. */
	char *dir_content = fs_msg->BUF;
	int num = 0;

	memcpy((void *)va2la(src, pathname), (void *)va2la(src, fs_msg->PATHNAME), name_len);

	struct inode *updir_inode;
	struct inode *dir_inode;
	if (strip_path(dirname, pathname, &updir_inode) != 0)
		return DIR_PATH_INEXISTE;
	int inode_nr = search_file_in_dir(dirname, updir_inode);

	// updir_inode->i_cnt++;
	dir_inode = get_inode_sched(updir_inode->i_dev, inode_nr);
	// put_inode(updir_inode);
	// put_inode(dir_inode);

	int i;
	int j;

	int m = 0;
	struct dir_entry *pde;
	//char fsbuf[SECTOR_SIZE];
	//char* fsbuf = kern_kmalloc(BLOCK_SIZE);
	char * fsbuf = NULL;
	buf_head *bh = NULL;
	int dir_blk0_nr = dir_inode->i_start_block;
	int nr_dir_entries = dir_inode->i_size / DIR_ENTRY_SIZE;
	for (i = 0; i < dir_inode->i_nr_blocks; i++)
	{
		bh = bread(dir_inode->i_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		//BUF_RD_BLOCK(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (pde->inode_nr == INVALID_INODE)
			{
				continue;
			}
			else
			{
				strcat((char *)va2la(src, dir_content), (char *)va2la(src, pde->name));
				strcat((char *)va2la(src, dir_content), "\n\0");
			}

			if (++num >= nr_dir_entries)
				break;
		}
		brelse(bh);

		if (num >= nr_dir_entries) /* all entries have been iterated */
			break;
	}

	return 0;
}
/*****************************************************************************
 *                                free_imap_bit
 *****************************************************************************/
/**
 * free bits in inode-map.
 *
 * @param dev  In which device the inode-map is located.
 * @param inode_nr inode number
 * @return  0, as it will success under most of the circumstance
 *****************************************************************************/
/*add by xkx 2023-1-3*/

static int free_imap_bit(int dev, int inode_nr)
{
	int i, j, k;

	int imap_blk0_nr = 1 + 1; /* 1 boot sector & 1 super block */
	struct super_block *sb = get_super_block(dev);

	//char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added by xw, 18/12/27
	//char* fsbuf = kern_kmalloc(BLOCK_SIZE); // local array, to substitute global fsbuf. added by xw, 18/12/27
	// (i * SECTOR_SIZE + j) * 8 + k = inode_nr ;
	k = inode_nr % 8;
	j = ((inode_nr - k) / 8) % BLOCK_SIZE;
	i = (((inode_nr - k) / 8) - j) / BLOCK_SIZE;
	char *fsbuf = NULL;
	buf_head *bh = NULL;
	bh = bread(dev, imap_blk0_nr + i);
	fsbuf = bh->buffer;
	//BUF_RD_BLOCK(dev, imap_blk0_nr + i, fsbuf);
	fsbuf[j] &= ~(1 << k);
	mark_buff_dirty(bh);
	brelse(bh);
	//BUF_WR_BLOCK(dev, imap_blk0_nr + i, fsbuf);

	return 0;
}

/*****************************************************************************
 *                                free_smap_bit
 *****************************************************************************/
/**
 * free bits in sector-map.
 *
 * @param dev  In which device the sector-map is located.
 * @param start_sect_nr the number of start sector
 * @param nr_sects_to_free number of sector to free
 * @return  if successful return 0, else return -1.
 *****************************************************************************/
static int free_smap_bit(int dev, int start_sect_nr, int nr_sects_to_free)
{
	// free_sect_nr = (i * SECTOR_SIZE + j) * 8 + k - 1 + sb->n_1st_sect;
	int i; /* sector index */
	int j; /* byte index */
	int k; /* bit index */

	struct super_block *sb = get_super_block(dev);

	int smap_blk0_nr = 1 + 1 + sb->nr_imap_blocks;
	//char* fsbuf = kern_kmalloc(BLOCK_SIZE);
	char* fsbuf = NULL;
	buf_head *bh = NULL;

	k = (start_sect_nr - sb->n_1st_block + 1) % 8;
	j = ((start_sect_nr - sb->n_1st_block + 1 - k) / 8) % BLOCK_SIZE;
	i = (((start_sect_nr - sb->n_1st_block + 1 - k) / 8) - j) / BLOCK_SIZE;

	for (; i < sb->nr_smap_blocks; i++)
	{ /* smap_blk0_nr + i :
current sect nr. */
		// RD_SECT_SCHED(dev, smap_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
		//BUF_RD_BLOCK(dev, smap_blk0_nr + i, fsbuf); // modified by mingxuan 2019-5-20
		bh = bread(dev, smap_blk0_nr + i);
		fsbuf = bh->buffer;

		/* byte offset in current sect */
		for (; j < BLOCK_SIZE && nr_sects_to_free > 0; j++)
		{

			for (; k < 8; k++)
			{ /* repeat till enough bits are set */

				fsbuf[j] &= (~(1 << k));
				if (--nr_sects_to_free == 0)
					break;
			}
			k = 0;
		}
		mark_buff_dirty(bh);
		brelse(bh);

		//BUF_WR_BLOCK(dev, smap_blk0_nr + i, fsbuf); // added by mingxuan 2019-5-20

		if (nr_sects_to_free == 0)
			break;
	}
	if (!nr_sects_to_free)
		//kern_kfree(fsbuf);
		return -1;

	//kern_kfree(fsbuf);
	return 0;
}

/*****************************************************************************
 *                                set_dir_entry
 *****************************************************************************/
/**
 * set its entry and father entry in a new directory file
 *
 * @param new_inode pointer to new directory file's inode
 * @param father_inode_nr inode number of father inode
 *****************************************************************************/
/*add by xkx 2023-1-3*/
static void set_dir_entry(struct inode *new_inode, int father_inode_nr)
{
	int dir_blk0_nr = new_inode->i_start_block;

	struct dir_entry *pde;

	//char* fsbuf = kern_kmalloc(BLOCK_SIZE); // local array, to substitute global fsbuf.
	char* fsbuf = NULL;
	buf_head * bh = NULL;
	bh = bread(new_inode->i_dev, dir_blk0_nr);
	fsbuf = bh->buffer;
	//BUF_RD_BLOCK(new_inode->i_dev, dir_blk0_nr, fsbuf);

	pde = (struct dir_entry *)fsbuf;

	pde->inode_nr = new_inode->i_num;
	strcpy(pde->name, ".");

	pde++;
	pde->inode_nr = father_inode_nr;
	strcpy(pde->name, ".."); 
	mark_buff_dirty(bh);
	brelse(bh);
	//BUF_WR_BLOCK(new_inode->i_dev, dir_blk0_nr, fsbuf);

	new_inode->i_size = 2 * DIR_ENTRY_SIZE;
	sync_inode(new_inode);
}

/**
 * @brief create directory
 * @param pathname  The full path of the directory to be opened/created.
 *
 * @return flag, 0 if successful, otherwise an error code.
 */
/*add by xkx 2023-1-3*/
int real_createdir(struct super_block *sb, const char *pathname)
{
	MESSAGE fs_msg;

	fs_msg.type = CREATEDIR;
	fs_msg.PATHNAME = (void *)pathname;
	fs_msg.NAME_LEN = strlen(pathname);
	fs_msg.source = proc2pid(p_proc_current);

	/*modified by gfx 2023-1-13*/
	int flag = ora_createdir(&fs_msg);

	return flag;
}
/**
 * @brief open a directory
 * @return 0 if successful ,otherwise an error code
 */
/*add by xkx 2023-1-3*/
int real_opendir(struct super_block *sb, const char *pathname)
{
	MESSAGE fs_msg;

	fs_msg.type = OPENDIR;
	fs_msg.PATHNAME = (void *)pathname;
	fs_msg.NAME_LEN = strlen(pathname);
	fs_msg.source = proc2pid(p_proc_current);
	/*modified by gfx 2023-1-13*/
	int flag = ora_opendir(&fs_msg);
	return flag;
}
/**
 * @brief delete a directory
 * @return 0 if successful ,otherwise an error code
 */
/*add by xkx 2023-1-3*/
int real_deletedir(struct super_block *sb, const char *pathname)
{
	MESSAGE fs_msg;

	fs_msg.type = DELETEDIR;
	fs_msg.PATHNAME = (void *)pathname;
	fs_msg.NAME_LEN = strlen(pathname);
	fs_msg.source = proc2pid(p_proc_current);
	int flag;
	flag = do_deletecheck(&fs_msg);
	if (flag) // 如果为错误码
	{
		return flag;
	}
	return ora_deletedir(&fs_msg);
}

/*add by gfx 2023-2-13*/
int real_showdir(const char *pathname, char *dir_content)
{
	MESSAGE fs_msg;

	fs_msg.type = SHOWDIR;
	fs_msg.PATHNAME = (void *)pathname;
	fs_msg.NAME_LEN = strlen(pathname);
	fs_msg.BUF = (void *)dir_content;
	fs_msg.source = proc2pid(p_proc_current);

	int flag = ora_showdir(&fs_msg);
	return flag;
}