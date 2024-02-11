/**********************************************************
*	vfs.c       //added by mingxuan 2019-5-17
***********************************************************/

#include "const.h"
#include "global.h"
#include "string.h"
#include "proto.h"
#include "fs.h"
#include "tty.h"
#include "buffer.h"
#include "vfs.h"
#include "hd.h"
#include "mount.h"

// PUBLIC struct vfs  vfs_table[NR_FS]; //modified by ran
PUBLIC struct file_desc f_desc_table[NR_FILE_DESC];
PUBLIC struct super_block super_blocks[NR_SUPER_BLOCK]; //added by mingxuan 2020-10-30
PUBLIC struct fs_type fstype_table[NR_FS_TYPE];
struct vfs_dentry *vfs_root;
PRIVATE SPIN_LOCK inode_alloc_lock;
PRIVATE SPIN_LOCK file_desc_lock;
PRIVATE SPIN_LOCK superblock_lock;

// must held sb lock
PRIVATE struct vfs_inode* vfs_alloc_inode_no_lock(struct super_block* sb){
	struct vfs_inode* inode = kern_kmalloc(sizeof(struct vfs_inode));
	//init
	memset(inode, 0, sizeof(struct vfs_inode));
	inode->i_nlink = 1;
	atomic_set(&inode->i_count, 1);
	inode->i_sb = sb;
	return inode;
}


PUBLIC void init_file_desc_table(){
    int i;
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));
}

PUBLIC struct vfs_inode * vfs_new_inode(struct super_block* sb){
	struct vfs_inode* res = NULL;
	acquire(&inode_alloc_lock);
	res = vfs_alloc_inode_no_lock(sb);
	if(sb->sb_lru_list){
		res->lru_nxt = sb->sb_lru_list;
		sb->sb_lru_list->lru_pre = res;
	}
	sb->sb_lru_list = res;
	release(&inode_alloc_lock);
	return res;
}

// get an inode, if not exist, alloc one 调用此函数获取inode
PUBLIC struct vfs_inode * vfs_get_inode(struct super_block* sb, int ino){
	struct vfs_inode *res = 0, *inode;
	// char *s = vfs_bmap, *e = vfs_bmap + INODE_BMAP_SIZE, c;
	// while(s < e){
	// 	if((c = *s) != -1){
	// 		int i = 0;
	// 		while((c & (1<<i)) && i < 8){
	// 			i++;
	// 		}
	// 		*s = c | (1<<i); // mark used;
	// 		res = vfs_inode_table + ((s-vfs_bmap) << 3) + i;
	// 		break;
	// 	}
	// 	s++;
	// }
	acquire(&inode_alloc_lock);
	for(inode = sb->sb_lru_list; inode; inode = inode->lru_nxt){
		if(inode->i_sb == sb && inode->i_no == ino){
			if(inode->lru_nxt){
				inode->lru_nxt->lru_pre = inode->lru_pre;
			}
			if(inode->lru_pre){
				inode->lru_pre->lru_nxt = inode->lru_nxt;
			}
			if(inode->i_sb->sb_lru_list == inode){
				inode->i_sb->sb_lru_list = NULL;
			}
			res = inode;
			break;
		}
	}
	if(res){
		atomic_inc(&res->i_count);
	}else{
		res = vfs_alloc_inode_no_lock(sb);
		res->i_no = ino;
		sb->sb_op->read_inode(res);
	}
	if(sb->sb_lru_list){
		res->lru_nxt = sb->sb_lru_list;
		sb->sb_lru_list->lru_pre = res;
	}
	sb->sb_lru_list = res;
	release(&inode_alloc_lock);
	return res;
}

// 如果引用计数归0，释放资源, 调用此函数必须持有inode锁,除非inode还没有加入目录树
// 如果引用计数归0，且引用的文件数也为0，执行删除文件
// put inode之后自动解锁
PUBLIC void vfs_put_inode(struct vfs_inode * inode){
	if(atomic_dec_and_test(&inode->i_count)){
		struct superblock_operations* ops = inode->i_sb->sb_op;
		if(ops && ops->put_inode){
			ops->put_inode(inode);
		}
		if(inode->i_nlink == 0){
			if(ops && ops->delete_inode){
				ops->delete_inode(inode);
			}
		}  
		// int i = (inode - vfs_inode_table);
		// vfs_bmap[i >> 3] &= ~(1 << (i%8));
		acquire(&inode_alloc_lock);
		if(inode->lru_nxt){
			inode->lru_nxt->lru_pre = inode->lru_pre;
		}
		if(inode->lru_pre){
			inode->lru_pre->lru_nxt = inode->lru_nxt;
		}
		if(inode->i_sb->sb_lru_list == inode){
			inode->i_sb->sb_lru_list = NULL;
		}
		kern_kfree(inode);
		release(&inode_alloc_lock);
	}
	release(&inode->lock);
}

// require mutex: dir, ent
PRIVATE void insert_sub_dentry(struct vfs_dentry* dir, struct vfs_dentry* dentry){
	dentry->d_nxt = dir->d_subdirs;
	if(dir->d_subdirs){
		dir->d_subdirs->d_pre = dentry;
	}
	dir->d_subdirs = dentry;
	dentry->d_parent = dir;
	if(!dentry->d_vfsmount)
		dentry->d_vfsmount = dir->d_vfsmount;
}

// require mutex: dir, ent
PRIVATE void remove_sub_dentry(struct vfs_dentry* dir, struct vfs_dentry* ent){
	if(dir->d_subdirs == ent){
		dir->d_subdirs = ent->d_nxt;
	}
	if(ent->d_pre){
		ent->d_pre->d_nxt = ent->d_nxt;
	}
	if(ent->d_nxt){
		ent->d_nxt->d_pre = ent->d_pre;
	}
	ent->d_parent = NULL;
}

// 0 for empty, else -1
PRIVATE int check_dir_entry_empty(struct vfs_dentry* dir){
	for(struct vfs_dentry* dentry = dir->d_subdirs; dentry; dentry = dentry->d_nxt){
		if((!(strcmp(dentry->d_name, ".")))
		||(!(strcmp(dentry->d_name, "..")))){
			continue;
		}
		return -1;
	}
	return 0;
}

PRIVATE struct vfs_dentry * alloc_dentry(const char* name){
	struct vfs_dentry* dentry;
	dentry = kern_kmalloc(sizeof(struct vfs_dentry));
	initlock(&dentry->lock, NULL);
	strcpy(dentry->d_name, name);
	// if(inode->i_sb){
	// 	entry->d_op = inode->i_sb->sb_dop;
	// }
	// entry->d_covers = entry;
	// entry->d_mounts = entry;
	dentry->d_mounted = 0;
	dentry->d_parent = NULL;
	dentry->d_nxt = dentry->d_pre = dentry->d_subdirs = NULL;
	return dentry;
}

// alloc a new dentry for filled inode in dir
// require mutex: dir, inode
PUBLIC struct vfs_dentry * vfs_new_dentry(const char* name, struct vfs_inode* inode){
	struct vfs_dentry* dentry;
	dentry = alloc_dentry(name);
	dentry->d_inode = inode;
	return dentry;
}

// delete dentry in dir
// require mutex: dir, dentry, dentry.inode
PUBLIC int delete_dentry(struct vfs_dentry* dentry, struct vfs_dentry* dir){
	if(dentry->d_subdirs || dentry->d_mounted){
		// check empty
		// disp_str("try to delete non empty dentry");
		return -1;
	}
	remove_sub_dentry(dir, dentry);
	vfs_put_inode(dentry->d_inode);
	kern_kfree(dentry);
	return 0;
}

PUBLIC void vfs_get_path(struct vfs_dentry* dir, char* buf, int size){
	if (!buf)
    {
        return;
    }
	if(dir == vfs_root){
		strncpy(buf, "/", size);
		return;
	}
	char path[MAX_PATH];
	char*p = path + MAX_PATH;
	*(--p) = 0;
	int len;
	while(dir != vfs_root){
		while(dir->d_vfsmount && dir->d_vfsmount->mnt_root==dir){
			dir = dir->d_vfsmount->mnt_mountpoint;
		}
		len = strlen(dir->d_name);
		p -= len;
		strncpy(p, dir->d_name, len);
		*(--p) = '/';
		dir = dir->d_parent;
	}
	strncpy(buf, p, size);
}

PRIVATE void register_fs_type(char* fstype_name, int fs_type, 
	struct file_operations* f_op, 
	struct inode_operations* i_op,
	struct dentry_operations* d_op,
	struct superblock_operations* s_op){
	struct fs_type* fs = &fstype_table[fs_type];
	fs->fstype_name = fstype_name;
	fs->fs_fop = f_op;
	fs->fs_iop = i_op;
	fs->fs_dop = d_op;
	fs->fs_sop = s_op;
}

PRIVATE char* strip_dir_path(const char *path, char* dir){
	int len = strlen(path), flag = 0;
	char *p = path + len - 1, *file_name = path;
	while(p != path && (*p) == '/')p--;
	while(p >= path && (*p)){
		if(*(p) == '/'){
			file_name = p + 1;
			break;
		}
		p--;
	}
	if(file_name != path){
		strncpy(dir, path, file_name - path);
		dir[file_name - path] = 0;
	}
	return file_name;
}

PUBLIC int vfs_check_exec_permission(struct vfs_inode* inode){
	int mode = inode->i_mode;
	if(!(mode & I_X)){
		return -1;
	}
	return 0;
}
// 上锁规则: 上层->下层目录:持有上层锁获取下层锁,已经持有下层锁的不允许获取上层锁,
//(todo:对于访问..可能产生的顺序是否存在死锁的可能,有待探究)
// dir lock must held, return entry with lock
// 参数dir dentry带锁,返回有效dentry也带锁
PRIVATE struct vfs_dentry * _do_lookup(struct vfs_dentry *dir, char *name, int release_origin){
	// acquire(&dir->lock);
	struct vfs_dentry* dentry = NULL;
	// root 特判
	if(dir == vfs_root && (!name[0])){
		dentry = dir;
	}else if(!strcmp(name, ".")){
		dentry = dir;
	}else if(!strcmp(name, "..")){
		while(dir->d_vfsmount && dir->d_vfsmount->mnt_root==dir){
			dir = dir->d_vfsmount->mnt_mountpoint;
		}
		dentry = dir->d_parent; // 对于挂载点下的dentry，上层是挂载前的
	}
	int (*compare)(const char*, const char*) = strcmp;// default compare func
	if(dir->d_op && dir->d_op->compare){
		compare = dir->d_op->compare;
	}
	if(!dentry){
		dentry = dir->d_subdirs;
		// int new_dentry = 0;
		for( ;dentry; dentry = dentry->d_nxt){
			if(!compare(dentry->d_name, name)){
				break;
			}
		}
	}
	if(!dentry){
		acquire(&dir->d_inode->lock);
		struct inode_operations * i_ops = NULL;
		int type = dir->d_inode->i_type;
		if(type == I_DIRECTORY){
			i_ops = dir->d_inode->i_op;
		}
		if(i_ops){
			dentry = i_ops->lookup(dir->d_inode, name);
			if(dentry){
				insert_sub_dentry(dir, dentry);
			}
		}
		/* lookup内部应有的流程: 
		 *  1. 按照相应文件系统结构读取磁盘查找; 
		 *  2. 若查找成功: .1 调用vfs_get_inode 得到一个inode,填入相应数据
		 *               .2 调用new_dentry 初始化新的dentry
		 *  3. 返回结果dentry的指针
		 *  注：每次新的lookup分配新inode之后，其引用计数为1，保证dentry中的inode有效
		 *     对于open的文件,引用计数再加1,close之后不会归0释放
		 *     可以简单认为目录树的dentry对于相应的inode都有一次额外的引用
		 *     删除dentry时,对应的引用计数清除,inode才有可能释放
		 */
		release(&dir->d_inode->lock);
	}
	while(dentry && (dentry->d_mounted)){
		// todo: follow mount
		struct vfs_mount* mnt = lookup_vfsmnt(dentry);
		dentry = mnt->mnt_root;
	}
	if(dentry != dir){ // 排除当前目录
		if(dentry){// 此时不管entry是命中的还是新创建的都已经在目录树中了，一锁换一锁
			acquire(&dentry->lock);
		}
		if(release_origin){
			release(&dir->lock);
		}
	}
	return dentry;
}

// 输入：根dentry base
PUBLIC struct vfs_dentry *vfs_lookup(const char *path){
	if(!path){
		return NULL;
	}
	char d_name[MAX_DNAME_LEN];
	char *p = path, *p_name = d_name;
	int flag_name = 0;
	struct vfs_dentry *dir = vfs_root;
	if(path[0] != '/'){
		dir = p_proc_current->task.cwd;
	}
	acquire(&dir->lock);
	while(*p == '/')p++;
	while(*p || flag_name){
		if(*p == '/' || *p == 0){// meet file name end
			if(flag_name){
				*p_name = 0;
				flag_name = 0;
				dir = _do_lookup(dir, d_name, 1);
				if(!dir){
					return NULL;
				}
				if(*p == 0){//最后一项也查找完毕
					break;
				}
			}
		}else if(!flag_name){
			flag_name = 1;
			p_name = d_name;
		}
		if(flag_name){
			*p_name++ = *p;
		}
		p++;
	}
	return dir;
}

PRIVATE void init_special_inode(struct vfs_inode* inode, int type, int mode, int dev){
	acquire(&inode->lock);
	inode->i_mode = mode;
	inode->i_type = type;
	inode->i_dev = dev;
	switch (type)
	{
		case I_CHAR_SPECIAL:
			inode->i_size = 0;
			inode->i_fop = &tty_file_ops;
			break;
		case I_BLOCK_SPECIAL:
			inode->i_size = hd_infos[MAJOR(dev)].part[MINOR(dev)].size;
			inode->i_fop = &blk_file_ops;
		default:
			break;
	}
	release(&inode->lock);
}

PUBLIC struct vfs_dentry* vfs_create(struct vfs_inode* dir,
	const char* file_name, int type, int mode, int dev){
	struct inode_operations * i_ops = NULL;
	struct vfs_dentry* dentry = NULL;
	struct vfs_inode* inode = NULL;
	int state;
	acquire(&dir->lock);
	if(dir->i_type == I_DIRECTORY){
		i_ops = dir->i_op;
	}
	if(i_ops){
		dentry = alloc_dentry(file_name); 
		// negative dentry, create call will get inode
		if(type == I_DIRECTORY){
			state = i_ops->mkdir(dir, dentry, mode);
		}else{ // regular file
			state = i_ops->create(dir, dentry, mode);
		}
		if(state != 0){// error
			kern_kfree(dentry);
			release(&dir->lock);
			return NULL;
		}
		if(type == I_BLOCK_SPECIAL || type == I_CHAR_SPECIAL){
			init_special_inode(dentry->d_inode, type, mode, dev);
		}
	}
	release(&dir->lock);
	return dentry;	
}

PUBLIC struct super_block * vfs_read_super(int dev, int fstype){
	if(hd_infos[MAJOR(dev)].part[MINOR(dev)].fs_type != fstype){
		// dismatch fstype
		return NULL;
	}
	struct fs_type* file_sys = &fstype_table[fstype];
	acquire(&superblock_lock);
	struct super_block* sb = &super_blocks[get_free_superblock()];
	if(!(file_sys->fs_sop && file_sys->fs_sop->fill_superblock)){
		release(&superblock_lock);
		return NULL;
	}
	int state = file_sys->fs_sop->fill_superblock(sb, dev);
	if(state){
		//error
		release(&superblock_lock);
		return NULL;
	}
	sb->used = 1;
	release(&superblock_lock);
	return sb;
}

PUBLIC void mount_root(int root_dev, int part, int root_fstype){
	int dev = get_fs_part_dev(root_dev, part, root_fstype);
	struct super_block *sb = vfs_read_super(dev, root_fstype);
	vfs_root = sb->sb_root;
}

PUBLIC void init_fs(){
	initlock(&inode_alloc_lock, "");
	initlock(&file_desc_lock, "");
	init_file_desc_table();

	register_fs_type("orangefs", ORANGE_TYPE,
		&orange_file_ops, 
		&orange_inode_ops,
		NULL,
		&orange_sb_ops);
	register_fs_type("fat32", FAT32_TYPE,
		&fat32_file_ops,
		&fat32_inode_ops,
		&fat32_dentry_ops,
		&fat32_sb_ops);
	int drive = SATA_BASE;
	int partition = 2;
	mount_root(drive, partition, ORANGE_TYPE);
}

PUBLIC int get_fstype_by_name(const char* fstype_name){
	int fstype = NO_FS_TYPE;
	for(;fstype < NR_FS_TYPE; fstype++){
		if(!strcmp(fstype_name, fstype_table[fstype].fstype_name)){
			break;
		}
	}
	if(fstype == NR_FS_TYPE){
		// unknown fs type
		return NO_FS_TYPE;
	}
	return fstype;
}

PUBLIC int kern_vfs_mount(const char *source, const char *target,
                      const char *filesystemtype, unsigned long mountflags, const void *data)
{
    // modified by sundong 2023.5.28
    //int device = get_dev_from_name(source);
    //从根文件系统中获取块设备的设备号
	struct vfs_dentry* block_dev = vfs_lookup(source);
	if(!block_dev){
		return -1;
	}
	acquire(&block_dev->d_inode->lock);
	int dev = block_dev->d_inode->i_dev;
	if(block_dev->d_inode->i_type != I_BLOCK_SPECIAL){
		// 不是块设备，挂载失败
		dev = -1;
	}
	release(&block_dev->d_inode->lock);
	release(&block_dev->lock);
	if(dev == -1){
		return -1;
	}
	struct vfs_dentry* dir_d = vfs_lookup(target);
	if(!dir_d){
		disp_str("mountpoint dir not exist\n");
        return -1;
	}
	if(dir_d->d_mounted){
		disp_str("mountpoint has already be mountted\n");
        return -1;
	}
	int fstype = get_fstype_by_name(filesystemtype);
	struct super_block *sb = vfs_read_super(dev, fstype);
	if(!sb){
		release(&dir_d->lock);
		return -1;
	}
	sb->sb_root->d_vfsmount = add_vfsmount(block_dev, dir_d, sb->sb_root, sb);
	dir_d->d_mounted = 1;
	release(&dir_d->lock);
	return 0;
}

PUBLIC int kern_vfs_umount(const char *target){
	struct vfs_dentry *mnt = vfs_lookup(target), *mountpoint;
	if(!mnt){
		return -1;
	}
	release(&mnt->lock);
	mountpoint = remove_vfsmnt(mnt);
	if(!mountpoint){
		return -1;
	}
	mountpoint->d_mounted = 0;
	// mnt->
	return 0;
}

PRIVATE struct file_desc * vfs_file_open(const char* path, int flags, int mode){
	int i;
	char dir_path[MAX_PATH] = {0};
	char* file_name = strip_dir_path(path, dir_path);
	struct vfs_dentry *dir = vfs_lookup(dir_path), *dentry = NULL;
	struct vfs_inode *inode = NULL;
	struct file_desc *file = NULL;
	if(!dir){
		return NULL;
	}
	dentry = _do_lookup(dir, file_name, 0);
	if(!dentry && (flags & O_CREAT)){
		//create regular file
		dentry = vfs_create(dir->d_inode, file_name, I_REGULAR, mode&(~I_TYPE_MASK), 0);
		if(!dentry)
			goto no_dentry_out;
		insert_sub_dentry(dir, dentry);
		acquire(&dentry->lock);
	}
	if(!dentry){
		goto no_dentry_out;
	}
	inode = dentry->d_inode;
	acquire(&inode->lock);
	if((!(flags & O_DIRECTORY)) && inode->i_type == I_DIRECTORY){
		disp_str("error: try to open a dir");
		goto err;
	}
	int rmode = (flags + 1) & O_ACC;
	if((rmode & (~inode->i_mode))&O_ACC){
		disp_str("error: no permission");
		goto err;
	}
	acquire(&file_desc_lock);
	/* find a free slot in f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++){
		if (f_desc_table[i].flag == 0){
			file = &f_desc_table[i];
			break;
		}	
	}
	if (i >= NR_FILE_DESC){
		release(&file_desc_lock);
		disp_str("f_desc_table[] is full (PID:");
		disp_int(proc2pid(p_proc_current));
		disp_str(")\n");
		goto err;
	}
	file->flag = 1;
	atomic_set(&file->fd_count, 1);
	release(&file_desc_lock);
	atomic_inc(&inode->i_count);
	file->fd_dentry = dentry;
	file->fd_mode = rmode;//fd_mode: bit 0:read 1:write
	file->fd_pos = 0;	
	release(&inode->lock);
	release(&dentry->lock);
	release(&dir->lock);
	return file;
err:
	release(&inode->lock);
	release(&dentry->lock);
no_dentry_out:
	release(&dir->lock);
	return NULL;
}

PUBLIC int kern_vfs_open(const char *path, int flags, int mode) {
	
	int fd = -1, i;
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
		disp_str("filp[] is full (PID:");
		disp_int(proc2pid(p_proc_current));
		disp_str(")\n");
		return -1;
	}
	//此处拆解开始是因为 某些情况下用户对底层filp应当不可见(如opendir)
	struct file_desc* filp = vfs_file_open(path, flags, mode);
	if(filp == NULL){
		return -1;
	}
	/* connects proc with file_descriptor */
	p_proc_current->task.filp[fd] = filp;
    return fd;
}

PUBLIC int kern_vfs_close(int fd) {    //modified by mingxuan 2021-8-15
	struct file_desc* file = p_proc_current->task.filp[fd];
	if(!file){
		return -1;
	}
    struct vfs_inode* inode = file->fd_dentry->d_inode;
	acquire(&inode->lock);
	vfs_put_inode(inode);
	p_proc_current->task.filp[fd]->fd_dentry = 0; // modified by mingxuan 2019-5-17
	acquire(&file_desc_lock);
	fput(file);
	release(&file_desc_lock);			 // added by mingxuan 2019-5-17
	p_proc_current->task.filp[fd] = 0;
	return 0;
}

PUBLIC int kern_vfs_lseek(int fd, int offset, int whence){
	struct file_desc * file = p_proc_current->task.filp[fd];
	if(!file){
		return -1;
	}
	struct vfs_inode * inode = file->fd_dentry->d_inode;
	int size;
	acquire(&inode->lock);
	if(!inode_allow_lseek(inode->i_type)){
		return -1;
	}
	size = inode->i_size;
	release(&inode->lock);
	int _offset = -1;
	switch (whence)
	{
		case SEEK_SET:
			_offset = offset;
			break;
		case SEEK_CUR:
			_offset = file->fd_pos + offset;
			break;
		case SEEK_END:
			_offset = size + offset;
			break;
		default:
			break;
	}
	if(_offset >= 0){
		file->fd_pos = _offset;
		return _offset;
	}
	return -1;
}

PUBLIC int kern_vfs_read(int fd, char *buf, int count){
	struct file_desc * file = p_proc_current->task.filp[fd];
	if(!file || count<0){
		return -1;
	}
	if(!(file->fd_mode & I_R)){
		disp_str("read error: no permission");
		return -1;
	}
	struct vfs_inode * inode = file->fd_dentry->d_inode;
	int cnt = -1;
	acquire(&inode->lock);
	if(inode->i_fop && inode->i_fop->read){
		cnt = inode->i_fop->read(file, count, buf);
	}
	release(&inode->lock);
	return cnt;
}

PUBLIC int kern_vfs_write(int fd, const char *buf, int count){
	struct file_desc * file = p_proc_current->task.filp[fd];
	if(!file || count<0){
		return -1;
	}
	if(!(file->fd_mode & I_W)){
		disp_str("write error: no permission");
		return -1;
	}
	struct vfs_inode * inode = file->fd_dentry->d_inode;
	int cnt = -1;
	acquire(&inode->lock);
	if(inode->i_fop && inode->i_fop->write){
		cnt = inode->i_fop->write(file, count, buf);
	}
	release(&inode->lock);
	return cnt;
}

PUBLIC int kern_vfs_creat(const char* path, int mode){
	return kern_vfs_open(path, O_CREAT|O_WRONLY, mode);
}

PUBLIC int kern_vfs_unlink(const char *path){
	char dir_path[MAX_PATH] = {0};
	char* file_name = strip_dir_path(path, dir_path);
	struct vfs_dentry *dir = vfs_lookup(dir_path), *entry = NULL;
	if(!dir){
		release(&dir->lock);
		return -1;
	}
	entry = _do_lookup(dir, file_name, 0);
	if(!entry){
		release(&dir->lock);
		return -1;
	}
	struct vfs_inode *inode = entry->d_inode;
	acquire(&inode->lock);
	if(inode->i_type == I_DIRECTORY){
		goto err;	
	}
	if(inode->i_op && inode->i_op->unlink){
		acquire(&dir->d_inode->lock);
		int state = inode->i_op->unlink(dir->d_inode, entry);
		release(&dir->d_inode->lock);
		if(state){
			goto err;
		}
		inode->i_nlink = 0;
	}
	delete_dentry(entry, dir);
	release(&entry->lock);
	release(&dir->lock);
	return 0;
err:
	release(&inode->lock);
	release(&entry->lock);
	release(&dir->lock);
	return -1;
}

PUBLIC int kern_vfs_mknod(const char* path, int mode, int dev){
	char dir_path[MAX_PATH] = {0};
	char* file_name = strip_dir_path(path, dir_path);
	struct vfs_dentry *dir = vfs_lookup(dir_path), *dentry = NULL;
	struct vfs_inode *inode = NULL;
	if(!dir){
		return -1;
	}
	dentry = _do_lookup(dir, file_name, 0);
	if(dentry){
		init_special_inode(dentry->d_inode, mode&I_TYPE_MASK, mode&(~I_TYPE_MASK), dev);
	}else{
		dentry = vfs_create(dir->d_inode, file_name, mode&I_TYPE_MASK, mode&(~I_TYPE_MASK), dev);
		if(!dentry){
			release(&dir->lock);
			return -1;
		}
		insert_sub_dentry(dir, dentry);
		acquire(&dentry->lock);
	}
	release(&dentry->lock);
	release(&dir->lock);
	return 0;
}

PUBLIC int kern_vfs_mkdir(const char* path, int mode){
	char dir_path[MAX_PATH] = {0};
	char* file_name = strip_dir_path(path, dir_path);
	struct vfs_dentry *dir = vfs_lookup(dir_path), *dentry = NULL;
	struct vfs_inode *inode = NULL;
	if(!dir){
		return -1;
	}
	dentry = _do_lookup(dir, file_name, 0);
	if(dentry){
		release(&dentry->lock);
		release(&dir->lock);
		return -1;
	}
	dentry = vfs_create(dir->d_inode, file_name, I_DIRECTORY, mode&(~I_TYPE_MASK), 0);
	insert_sub_dentry(dir, dentry);
	release(&dir->lock);
	return 0;
}

PUBLIC int kern_vfs_rmdir(const char* path){
	char dir_path[MAX_PATH] = {0};
	char* file_name = strip_dir_path(path, dir_path);
	struct vfs_dentry *dir = vfs_lookup(dir_path), *dentry = NULL;
	if(!dir){
		release(&dir->lock);
		return -1;
	}
	dentry = _do_lookup(dir, file_name, 0);
	if(!dentry){
		release(&dir->lock);
		return -1;
	}
	struct vfs_inode *inode = dentry->d_inode;
	acquire(&inode->lock);
	if(dentry == vfs_root || dentry->d_vfsmount->mnt_root == dentry){// 不允许删除挂载点
		goto err;
	}
	if(inode->i_type != I_DIRECTORY){
		goto err;	
	}
	if(check_dir_entry_empty(dentry) != 0){
		goto err;
	}
	if(inode->i_op && inode->i_op->rmdir){
		acquire(&dir->d_inode->lock);
		int state = inode->i_op->rmdir(dir->d_inode, dentry);
		release(&dir->d_inode->lock);
		if(state){
			goto err;
		}
		inode->i_nlink = 0;
	}
	vfs_put_inode(inode);
	struct vfs_dentry* sub;
	while(sub = dentry->d_subdirs){
		delete_dentry(sub, dentry);
	}
	delete_dentry(dentry, dir);
	release(&dentry->lock);
	release(&dir->lock);
	return 0;
err:
	release(&inode->lock);
	release(&dentry->lock);
	release(&dir->lock);
	return -1;
}

PUBLIC DIR* kern_vfs_opendir(const char* path){
	struct file_desc * file = vfs_file_open(path, O_DIRECTORY|O_RDONLY, I_RWX);
	if(!file){
		return NULL;
	}
	DIR* dirp = kern_malloc_4k();
	dirp->file = file;
	dirp->count = 0;
	dirp->total = 0;
	dirp->init = 0;
	return dirp;
}

PUBLIC int kern_vfs_closedir(DIR* dirp){
	struct vfs_inode* inode = dirp->file->fd_dentry->d_inode;
	acquire(&inode->lock);
	vfs_put_inode(inode);
	dirp->file->fd_dentry = 0; // modified by mingxuan 2019-5-17
	acquire(&file_desc_lock);
	dirp->file->flag = 0;
	release(&file_desc_lock);
	kern_free_4k(dirp);
	return 0;
}

struct dirent* kern_vfs_readdir(DIR* dirp){
	if(!dirp->init){
		struct vfs_inode* inode = dirp->file->fd_dentry->d_inode;
		struct dirent* data_start = DIR_DATA(dirp);
		acquire(&inode->lock);
		if(inode->i_fop && inode->i_fop->readdir){
			dirp->total = inode->i_fop->readdir(dirp->file, data_start);
		}
		dirp->init = 1;
		release(&inode->lock);
	}
	struct dirent* ent = NULL;
	if(dirp->count < dirp->total){
		ent = &DIR_DATA(dirp)[dirp->count++];
		if(!strcmp(ent->d_name, "..")){// check parent needed for mount point
			acquire(&dirp->file->fd_dentry->lock);
			struct vfs_dentry* found = _do_lookup(dirp->file->fd_dentry, ent->d_name, 1);
			if(found){
				ent->d_ino = found->d_inode->i_no;
				release(&found->lock);
			}
		}
	}
	return ent;
}

PUBLIC int kern_vfs_chdir(const char* path){
	struct vfs_dentry *dir = vfs_lookup(path);
	if(!dir){
		return -1;
	}
	struct vfs_inode* inode = dir->d_inode;
	int err = -1;
	if(inode->i_type == I_DIRECTORY){
		p_proc_current->task.cwd = dir;
		err = 0;
	}
	release(&dir->lock);
	return err;
}


PUBLIC char* kern_vfs_getcwd(char *buf, int size) {
    vfs_get_path(p_proc_current->task.cwd, buf, size);
    return buf;
}

//// do_xx
PUBLIC int do_vopen(const char *path, int flags, int mode) 
{
	return kern_vfs_open(path, flags, mode);
}

PUBLIC int do_vclose(int fd) 
{
	return kern_vfs_close(fd);
}

PUBLIC int do_vread(int fd, char *buf, int count) 
{
	return kern_vfs_read(fd, buf, count);
}

PUBLIC int do_vwrite(int fd, const char *buf, int count) 
{
	return kern_vfs_write(fd, buf, count);
}

PUBLIC int do_vlseek(int fd, int offset, int whence) 
{
	return kern_vfs_lseek(fd, offset, whence);
}

PUBLIC int do_vunlink(const char *path) {
	return kern_vfs_unlink(path);
}

PUBLIC int do_vcreat(const char *filepath)
{
	return kern_vfs_creat(filepath, I_RWX);
}

PUBLIC int do_vopendir(const char* path)
{
    return kern_vfs_opendir(path);
}

PUBLIC int do_vclosedir(DIR* dirp)
{
	return kern_vfs_closedir(dirp);
}

PUBLIC int do_vmkdir(const char *path, int mode) {
	return kern_vfs_mkdir(path, mode);
}

PUBLIC int do_vrmdir(const char *path) {
    return kern_vfs_rmdir(path);
}

PUBLIC struct dirent* do_vreaddir(DIR* dirp)
{
	return kern_vfs_readdir(dirp);
}

PUBLIC int do_vchdir(const char *path) 
{
    return kern_vfs_chdir(path);
}

PUBLIC int do_vgetcwd(char *buf, int size) 
{
    return (int)kern_vfs_getcwd(buf, size);
}
/*======================================================================*
                              sys_* 系列函数
 *======================================================================*/

PUBLIC int sys_open()
{
    return do_vopen(get_arg(1), get_arg(2), get_arg(3));
}

PUBLIC int sys_close()
{
    return do_vclose(get_arg(1));
}

PUBLIC int sys_read()
{
    return do_vread(get_arg(1), get_arg(2), get_arg(3));
}

PUBLIC int sys_write()
{
    return do_vwrite(get_arg(1), get_arg(2), get_arg(3));
}

PUBLIC int sys_lseek()
{
    return do_vlseek(get_arg(1), get_arg(2), get_arg(3));
}

PUBLIC int sys_unlink() {
    return do_vunlink(get_arg(1));
}

PUBLIC int sys_creat() {
    return do_vcreat(get_arg(1));
}

PUBLIC int sys_closedir() {
    return do_vclosedir((DIR*)get_arg(1));
}

PUBLIC int sys_opendir() {
    return do_vopendir((const char*)get_arg(1));
}

PUBLIC int sys_mkdir() {
    return do_vmkdir(get_arg(1), get_arg(2));
}

PUBLIC int sys_rmdir() {
    return do_vrmdir(get_arg(1));
}

PUBLIC int sys_readdir() {
    return do_vreaddir((DIR*)get_arg(1));
}

//added by ran
PUBLIC int sys_chdir() {
  return do_vchdir((const char*)get_arg(1));
}

//added by ran
PUBLIC int sys_getcwd() {
  return (int)do_vgetcwd((char*)get_arg(1), (int)get_arg(2));
}