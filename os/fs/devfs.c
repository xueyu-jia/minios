#include <kernel/devfs.h>
#include <klib/string.h>
#include <kernel/memman.h>
#include <kernel/hd.h>
#include <kernel/fs.h>
#include <klib/spinlock.h>
#include <kernel/proto.h>

PRIVATE list_head devices;

PRIVATE SPIN_LOCK devices_lock;


PUBLIC void init_devices() {
	list_init(&devices);
	initlock(&devices_lock, NULL);
}

PRIVATE struct device* find_major_device(int dev) {
	struct device* dev_struct = NULL;
	int major = MAJOR(dev);
	acquire(&devices_lock);
	list_for_each(&devices, dev_struct, dev_list) {
		if(dev_struct->dev_major == major) {
			release(&devices_lock);
			return dev_struct;
		}
	}
	release(&devices_lock);
	return NULL;
}

PRIVATE void dev_to_basename(int major, char* dev_name) {
	char *prefix = NULL;
	switch (major)
	{
	case DEV_CHAR_TTY:
		prefix = "tty";
		strcpy(dev_name, prefix);
		break;

	// 这种写法的含义是：case L ... R: 匹配 >=L且<=R的值
	case DEV_HD_BASE ... (DEV_HD_LIMIT - 1):
		major -= DEV_HD_BASE;
		switch (major)
		{
		case IDE_BASE ... (IDE_LIMIT-1):
			prefix = "hd";
			major -= IDE_BASE;
			break;
		case SATA_BASE ... (SATA_LIMIT-1):
			prefix = "sd";
			major -= SATA_BASE;
			break;
		default:
			break;
		}
		strcpy(dev_name, prefix);
		dev_name[strlen(dev_name)] = 'a' + major;
		break;
	default:
		break;
	}
	dev_name[strlen(dev_name)] = 0;
}

PUBLIC struct device *register_device(int dev, int type, struct file_operations* fop) {
	struct device *dev_struct = find_major_device(dev);
	if(dev_struct == NULL) {
		dev_struct = (struct device *)kern_kmalloc(sizeof(struct device));
		list_init(&dev_struct->dev_list);
		dev_struct->dev_major = MAJOR(dev);
		dev_struct->dev_type = type;
		dev_struct->dev_fop = fop;
		dev_struct->dev_minor_map = 0;
		acquire(&devices_lock);
		list_add_last(&dev_struct->dev_list, &devices);
		release(&devices_lock);
	}
	dev_struct->dev_minor_map |= (1 << MINOR(dev));
	return dev_struct;
}

PUBLIC int unregister_device(int dev) {
	struct device *dev_struct = find_major_device(dev);
	if(dev_struct == NULL) {
		return -1;
	}
	dev_struct->dev_minor_map &= (~(1 << MINOR(dev)));
	if(dev_struct->dev_minor_map == 0) {
		acquire(&devices_lock);
		list_remove(&dev_struct->dev_list);
		release(&devices_lock);
		kern_kfree((u32)dev_struct);
	}
	return 0;
}


PUBLIC struct dentry *devfs_lookup(struct inode *dir, const char *filename) {
	struct device * dev_struct = NULL;
	int dev = 0;
	char dev_name[8];
	acquire(&devices_lock);
	list_for_each(&devices, dev_struct, dev_list) {
		memset(dev_name, 0, 8);
		dev_to_basename(dev_struct->dev_major, dev_name);
		int minor_name = strlen(dev_name);
		u32 minor_bit = dev_struct->dev_minor_map;
		for(int i = 0;minor_bit && i < 32; i++) {
			if(minor_bit & 1) {
				if(i != 0 || dev_struct->dev_type != DEV_BLOCK_TYPE) {
					itoa(i, dev_name + minor_name, 10);
				}
				if(!strcmp(dev_name, filename)) {
					dev = MAKE_DEV(dev_struct->dev_major, i);
					goto found;
				}
			}
			minor_bit >>= 1;
		}
	}
found:
	release(&devices_lock);
	if(dev) {
		struct inode *inode = vfs_get_inode(dir->i_sb, dev);
		return vfs_new_dentry(dev_name, inode);
	}
	return NULL;
}

PUBLIC int devfs_readdir(struct file_desc *file, unsigned int count, struct dirent* start) {
	struct device * dev_struct = NULL;
	struct dirent* dent = start;
	char dev_name[8];
	dirent_fill(dent, 1, 1);
	strcpy(dent->d_name, ".");
	count -= dent->d_len;
	dent = dirent_next(dent);
	dirent_fill(dent, 1, 2);
	strcpy(dent->d_name, "..");
	count -= dent->d_len;
	dent = dirent_next(dent);
	acquire(&devices_lock);
	list_for_each(&devices, dev_struct, dev_list) {
		memset(dev_name, 0, 8);
		dev_to_basename(dev_struct->dev_major, dev_name);
		int minor_name = strlen(dev_name);
		u32 minor_bit = dev_struct->dev_minor_map;
		for(int i = 0;minor_bit && i < 32; i++) {
			if(minor_bit & 1) {
				if(!(i == 0 && dev_struct->dev_type == DEV_BLOCK_TYPE)) {
					itoa(i, dev_name + minor_name, 10);
				}
				dirent_fill(dent, MAKE_DEV(dev_struct->dev_major, i), strlen(dev_name));
				strcpy(dent->d_name, dev_name);
				count -= dent->d_len;
				dent = dirent_next(dent);
			}
			minor_bit >>= 1;
		}
	}
	release(&devices_lock);
	return (u32)dent - (u32)start;
}

PUBLIC void devfs_read_inode(struct inode *inode) {
	if(inode->i_no == 1) {
		inode->i_mode = I_R|I_X;
		inode->i_type = I_DIRECTORY;
		inode->i_size = num_4K;
		inode->i_op = &devfs_inode_ops;
		inode->i_fop = &devfs_file_ops;
	} else {
		int dev = inode->i_no;
		struct device *dev_struct = find_major_device(dev);
		if(dev_struct == NULL) {
			disp_str("error: cant find device");
		}
		inode->i_mode = I_R|I_W;
		inode->i_b_cdev = dev;
		switch (dev_struct->dev_type)
		{
		case DEV_BLOCK_TYPE:
			inode->i_type = I_BLOCK_SPECIAL;
			inode->i_size = ((u64)hd_infos[MAJOR(dev) - DEV_HD_BASE].part[MINOR(dev)].size)
				 << SECTOR_SIZE_SHIFT;
			break;
		case DEV_CHAR_TYPE:
			inode->i_type = I_CHAR_SPECIAL;
			inode->i_size = 0;
		default:
			break;
		}
		inode->i_fop = dev_struct->dev_fop;
	}
	inode->i_crtime = inode->i_atime = inode->i_mtime = current_timestamp;
	inode->i_nlink = 1;
}

PUBLIC int devfs_fill_superblock(struct super_block * sb, int dev) {
	sb->sb_dev = dev;
	sb->sb_op = &devfs_sb_ops;
	sb->fs_type = DEV_FS_TYPE;
	struct inode *root_inode = vfs_get_inode(sb, 1);
	sb->sb_root = vfs_new_dentry("dev", root_inode);
	return 0;
}

struct superblock_operations devfs_sb_ops = {
.fill_superblock = devfs_fill_superblock,
.read_inode = devfs_read_inode,
};

struct inode_operations devfs_inode_ops = {
.lookup = devfs_lookup,
};

struct file_operations devfs_file_ops = {
.readdir = devfs_readdir,
};
