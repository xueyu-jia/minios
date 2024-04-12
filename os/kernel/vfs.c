/**********************************************************
*	vfs.c       //added by mingxuan 2019-5-17
***********************************************************/
// VFS   modified by jiangfeng 2024-03
#include "const.h"
#include "string.h"
#include "proto.h"
#include "list.h"
#include "fs.h"
#include "tty.h"
#include "buffer.h"
#include "vfs.h"
#include "hd.h"
#include "mount.h"
#include "stat.h"

// PUBLIC struct vfs  vfs_table[NR_FS]; //modified by ran
PUBLIC struct file_desc f_desc_table[NR_FILE_DESC];
PUBLIC struct vfs_dentry *vfs_root;

// 实例文件系统调用：
PUBLIC struct vfs_inode * vfs_new_inode(struct super_block* sb);// inode create
PUBLIC struct vfs_inode * vfs_get_inode(struct super_block* sb, int ino); //inode lookup
PUBLIC struct vfs_dentry * vfs_new_dentry(const char* name, struct vfs_inode* inode); //inode lookup

PRIVATE u32 inode_free_cnt;
PRIVATE list_head inode_free_list;
#define MAX_FREE_INODE	32
PRIVATE SPIN_LOCK inode_alloc_lock;
PRIVATE SPIN_LOCK file_desc_lock;
PRIVATE SPIN_LOCK superblock_lock;

//尝试从之前分配的空闲链表取一个inode，没有则返回NULL
PRIVATE struct vfs_inode* get_rcu_free_inode() {
	if(list_empty(&inode_free_list)) {
		return NULL;
	}
	return list_entry(inode_free_list.next, struct vfs_inode, i_list);
}

// must held inode alloc lock
PRIVATE struct vfs_inode* vfs_alloc_inode_no_lock(struct super_block* sb){
	struct vfs_inode* inode = NULL;
	inode = get_rcu_free_inode();
	if(inode) {
		inode_free_cnt --;
		list_remove(&inode->i_list);
	} else {// real malloc
		inode = kern_kmalloc(sizeof(struct vfs_inode));
	}
	//init
	memset(inode, 0, sizeof(struct vfs_inode));
	inode->i_nlink = 1;
	atomic_set(&inode->i_count, 1);
	inode->i_sb = sb;
	list_init(&inode->i_list);
	return inode;
}


PRIVATE void init_file_desc_table(){
    int i;
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));
}

// VFS api
// alloc a new inode for superblock
// 调用此函数分配新的inode
PUBLIC struct vfs_inode * vfs_new_inode(struct super_block* sb){
	struct vfs_inode* res = NULL;
	acquire(&inode_alloc_lock);
	res = vfs_alloc_inode_no_lock(sb);
	list_add_first(&res->i_list, &sb->sb_inode_list);
	release(&inode_alloc_lock);
	return res;
}

PRIVATE void vfs_free_inode(struct vfs_inode* inode) {
	acquire(&inode_alloc_lock);
	if(inode_free_cnt > MAX_FREE_INODE) {
		list_remove(&inode->i_list);
		kern_kfree(inode);
	} else {// 添加到空闲链表暂存
		inode_free_cnt++;
		list_move(&inode->i_list, &inode_free_list);
	}
	release(&inode_alloc_lock);
}

// VFS api
// get an inode by ino, if not exist in mem, alloc one and read from disk
// 调用此函数获取inode
PUBLIC struct vfs_inode * vfs_get_inode(struct super_block* sb, int ino){
	struct vfs_inode *res = 0, *inode;
	acquire(&inode_alloc_lock);
	list_for_each(&sb->sb_inode_list, inode, i_list)
	{
		if(inode->i_sb == sb && inode->i_no == ino){
			list_remove(&inode->i_list);
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
	list_add_first(&res->i_list, &sb->sb_inode_list);
	release(&inode_alloc_lock);
	return res;
}

// 如果引用计数归0，释放资源, 调用此函数必须持有inode锁,除非inode还没有加入目录树
// 如果引用计数归0，且引用的文件数也为0，执行删除文件
// put inode之后自动解锁
PUBLIC void vfs_put_inode(struct vfs_inode * inode){
	if(atomic_dec_and_test(&inode->i_count)){
		struct superblock_operations* ops = inode->i_sb->sb_op;
		if(inode->i_nlink == 0){
			if(ops && ops->delete_inode){
				ops->delete_inode(inode);
			}
		}
		if(ops && ops->put_inode){
			ops->put_inode(inode);
		}
		vfs_free_inode(inode);
		return;
	}
	release(&inode->lock);
}

// require mutex: dir, ent
PRIVATE void insert_sub_dentry(struct vfs_dentry* dir, struct vfs_dentry* dentry){
	// dentry->d_nxt = dir->d_subdirs;
	// if(dir->d_subdirs){
	// 	dir->d_subdirs->d_pre = dentry;
	// }
	// dir->d_subdirs = dentry;
	list_add_first(&dentry->d_list, &dir->d_subdirs);
	dentry->d_parent = dir;
	if(!dentry->d_vfsmount)
		dentry->d_vfsmount = dir->d_vfsmount;
}

// require mutex: dir, ent
PRIVATE void remove_sub_dentry(struct vfs_dentry* dir, struct vfs_dentry* ent){
	// if(dir->d_subdirs == ent){
	// 	dir->d_subdirs = ent->d_nxt;
	// }
	// if(ent->d_pre){
	// 	ent->d_pre->d_nxt = ent->d_nxt;
	// }
	// if(ent->d_nxt){
	// 	ent->d_nxt->d_pre = ent->d_pre;
	// }
	list_remove(&ent->d_list);
	ent->d_parent = NULL;
}

// 0 for empty, else -1
PRIVATE int check_dir_entry_empty(struct vfs_dentry* dir){
	// for(struct vfs_dentry* dentry = dir->d_subdirs; dentry; dentry = dentry->d_nxt){
	struct vfs_dentry* dentry = NULL;
	list_for_each(&dir->d_subdirs, dentry, d_list) {
		if((!(strcmp(dentry_name(dentry), ".")))
		||(!(strcmp(dentry_name(dentry), "..")))){
			continue;
		}
		return -1;
	}
	return 0;
}

PRIVATE struct vfs_dentry * alloc_dentry(const char* name){
	struct vfs_dentry* dentry;
	dentry = kern_kmalloc(sizeof(struct vfs_dentry));
	memset(dentry, 0, sizeof(struct vfs_dentry));
	initlock(&dentry->lock, NULL);
	int len = strlen(name);
	if(len < MAX_DNAME_LEN) {
		strcpy(dentry->d_shortname, name);
	} else {
		char* pstr = kern_kmalloc(len + 1);
		strcpy(pstr, name);
		dentry->d_longname = pstr;
	}
	dentry->d_mounted = 0;
	atomic_set(&dentry->d_count, 1);
	dentry->d_parent = NULL;
	// dentry->d_nxt = dentry->d_pre = dentry->d_subdirs = NULL;
	list_init(&dentry->d_list);
	list_init(&dentry->d_subdirs);
	return dentry;
}

PRIVATE void free_dentry(struct vfs_dentry* dentry) {
	if(dentry->d_longname) {
		kern_kfree(dentry->d_longname);
	}
	kern_kfree(dentry);
}

// VFS api
// alloc a new dentry and connect to inode
// require mutex: dir, inode
PUBLIC struct vfs_dentry * vfs_new_dentry(const char* name, struct vfs_inode* inode){
	struct vfs_dentry* dentry;
	dentry = alloc_dentry(name);
	dentry->d_inode = inode;
	return dentry;
}

// locked: dentry
PUBLIC void vfs_put_dentry(struct vfs_dentry* dentry) {
	if(atomic_dec_and_test(&dentry->d_count)){
		// 如果其中的inode已经无效，说明是已经unlink/rmdir的dentry
		if(dentry->d_inode && dentry->d_inode->i_nlink == 0) {
			acquire(&dentry->d_inode);
			vfs_put_inode(dentry->d_inode);
			dentry->d_inode = NULL;
		}
		if(!dentry->d_inode) {
			free_dentry(dentry);
			return;
		}
	}
	release(&dentry->lock);
}

PRIVATE void vfs_get_dentry(struct vfs_dentry* dentry) {
	acquire(&dentry->lock);
	atomic_inc(&dentry->d_count);
}
// delete dentry in dir
// require mutex: dir, dentry
PRIVATE int delete_dentry(struct vfs_dentry* dentry, struct vfs_dentry* dir){
	// if(dentry->d_subdirs || dentry->d_mounted){
	if((!list_empty(&dentry->d_subdirs)) || dentry->d_mounted){
		// check empty
		// disp_str("try to delete non empty dentry");
		return -1;
	}
	remove_sub_dentry(dir, dentry);
	return 0;
}

PRIVATE void vfs_get_path(struct vfs_dentry* dir, char* buf, int size){
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
		while(dir != vfs_root && dir->d_vfsmount && dir->d_vfsmount->mnt_root==dir){
			dir = dir->d_vfsmount->mnt_mountpoint;
		}
		len = strlen(dentry_name(dir));
		p -= len;
		strncpy(p, dentry_name(dir), len);
		*(--p) = '/';
		dir = dir->d_parent;
	}
	strncpy(buf, p, size);
}

PRIVATE void register_fs_type(char* fstype_name, int fs_type, 
	struct file_operations* f_op, 
	struct inode_operations* i_op,
	struct dentry_operations* d_op,
	struct superblock_operations* s_op,
	int (*identify)(int, u32)){
	struct fs_type* fs = &fstype_table[fs_type];
	fs->fstype_name = fstype_name;
	fs->fs_fop = f_op;
	fs->fs_iop = i_op;
	fs->fs_dop = d_op;
	fs->fs_sop = s_op;
	fs->identify = identify;
}

// 文件名拆分：同时删除末尾多余的"/"
PRIVATE char* strip_dir_path(char *path, char* dir){
	int len = strlen(path), flag = 0;
	char *p = path + len - 1, *file_name = path, *file_end = path + len - 1;
	while(p != path && (*p) == '/')p--;
	file_end = p;
	file_end[1] = '\0';
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
// dir lock must held, return dentry with lock
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
		if(dir == vfs_root) {
			dentry = vfs_root;
		} else {
			while(dir != vfs_root && dir->d_vfsmount && dir->d_vfsmount->mnt_root==dir){
				dir = dir->d_vfsmount->mnt_mountpoint;
			}
			dentry = dir->d_parent; // 对于挂载点下的dentry，上层是挂载前的
		}
	}
	int (*compare)(const char*, const char*) = strcmp;// default compare func
	if(dir->d_op && dir->d_op->compare){
		compare = dir->d_op->compare;
	}
	if(!dentry){
		// dentry = dir->d_subdirs;
		// int new_dentry = 0;
		// for( ;dentry; dentry = dentry->d_nxt){
		int found = 0;
		if(!list_empty(&dir->d_subdirs)) {
			list_for_each(&dir->d_subdirs, dentry, d_list) {
				if(!compare(dentry_name(dentry), name)){
					found = 1;
					break;
				}
			}
		}
		if(found == 0) {
			dentry = NULL;
		}
	}
	if(dentry){ // dentry 命中
		if(dentry != dir) {
			vfs_get_dentry(dentry);
		}
	} else {	// real lookup
		acquire(&dir->d_inode->lock);
		struct inode_operations * i_ops = NULL;
		int type = dir->d_inode->i_type;
		if(type == I_DIRECTORY){
			i_ops = dir->d_inode->i_op;
		}
		if(i_ops){
			dentry = i_ops->lookup(dir->d_inode, name);
			if(dentry){
				acquire(&dentry->lock);
				insert_sub_dentry(dir, dentry);
			}
		}
		/* lookup内部应有的流程: 
		 *  1. 按照相应文件系统结构读取磁盘查找; 
		 *  2. 若查找成功: .1 调用vfs_get_inode 得到一个inode,填入相应数据
		 *               .2 调用new_dentry 初始化新的dentry
		 *  3. 返回结果dentry的指针
		 *  注：每次新的lookup分配新inode之后，其引用计数为1，(引用来自dentry)
		 * 	   dentry引用计数d_count 为1，保证dentry有效，引用来自上层调用
		 *     如open的文件,引用归属于file
		 *     删除dentry时,对应的引用计数清除,dentry才有可能释放
		 */
		release(&dir->d_inode->lock);
	}
	while(dentry && (dentry->d_mounted)){
		// todo: follow mount
		struct vfs_mount* mnt = lookup_vfsmnt(dentry);
		vfs_get_dentry(mnt->mnt_root);
		vfs_put_dentry(dentry);
		dentry = mnt->mnt_root;
	}
	if(dentry != dir){
		if(release_origin){
			vfs_put_dentry(dir);
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
	vfs_get_dentry(dir);
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
	inode->i_b_cdev = dev;
	switch (type)
	{
		case I_CHAR_SPECIAL:
			inode->i_size = 0;
			inode->i_fop = &tty_file_ops;
			break;
		case I_BLOCK_SPECIAL:
			inode->i_size = hd_infos[MAJOR(dev)-DEV_HD_BASE].part[MINOR(dev)].size << SECTOR_SIZE_SHIFT;
			inode->i_fop = &blk_file_ops;
		default:
			break;
	}
	release(&inode->lock);
}

PRIVATE struct vfs_dentry* vfs_create(struct vfs_inode* dir,
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
		state = -1; // default err
		// negative dentry, create call will get inode
		if(type == I_DIRECTORY){
			if(i_ops->mkdir){
				state = i_ops->mkdir(dir, dentry, mode);
			}
		}else{ // regular file
			if(i_ops->mkdir){
				state = i_ops->create(dir, dentry, mode);
			}
		}
		if(state != 0){// error
			vfs_put_dentry(dentry);
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

PRIVATE struct super_block * vfs_read_super(int dev, u32 fstype){
	if(dev) {
		// 对于 fstype NO_FS_TYPE, 可以认为其为 自动 ，即跟随hd_infos中的FSTYPE
		if(fstype == NO_FS_TYPE) {
			fstype = get_hd_fstype(dev);
		}else if(get_hd_fstype(dev) != fstype){
			// dismatch fstype
			disp_str("fail: fstype not match");
			return NULL;
		}
		// todo: check hd busy
	}
	struct fs_type* file_sys = &fstype_table[fstype];
	acquire(&superblock_lock);
	struct super_block* sb = &super_blocks[get_free_superblock()];
	if(!(file_sys->fs_sop && file_sys->fs_sop->fill_superblock)){
		release(&superblock_lock);
		return NULL;
	}
	list_init(&sb->sb_inode_list);
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

PRIVATE struct super_block * vfs_get_super(int dev, u32 fstype) {
	for(int i = 0; i < NR_SUPER_BLOCK; i++) {
		if(super_blocks[i].used && super_blocks[i].sb_dev == dev) {
			return &super_blocks[i];
		}
	}
	return vfs_read_super(dev, fstype);
}

// return mounted root
PRIVATE struct vfs_dentry *kern_mount_dev(int dev, u32 fstype, const char *dev_name, struct vfs_dentry * mnt) {
	struct super_block *sb = vfs_get_super(dev, fstype);
	if(!sb){
		return NULL;
	}
	if(sb->sb_vfsmount) {
		disp_str("device already mounted");
		return NULL;
	}
	sb->sb_vfsmount = sb->sb_root->d_vfsmount =  add_vfsmount(dev_name, mnt, sb->sb_root, sb);
	if(mnt) {
		mnt->d_mounted = 1;
	}
	return sb->sb_root;
}

PRIVATE void mount_root(int root_drive){
	//ROOT_FSTYPE 和 ROOT_PART 由Makefile中的配置传递，这样不用再手动修改了 2024-03-10 jiangfeng
	#ifndef ROOT_FSTYPE
	#define ROOT_FSTYPE "orangefs"
	#endif
	int root_fstype = get_fstype_by_name(ROOT_FSTYPE);
	int dev = -1;
	#ifdef ROOT_PART
		dev = get_hd_part_dev(root_drive, ROOT_PART, root_fstype);
	#else
		dev = get_hd_dev(root_drive, root_fstype); // 自动匹配符合文件系统类型的第一个分区
	#endif
	// struct super_block *sb = vfs_get_super(dev, root_fstype);
	vfs_root = kern_mount_dev(dev, root_fstype, "/", NULL);
}

PRIVATE void init_fsroot() {
	kern_vfs_mkdir("/dev", I_RWX);
	struct vfs_dentry *dev_dir = _do_lookup(vfs_root, "dev", 0);
	kern_mount_dev(NO_DEV, DEV_FS_TYPE, "dev", dev_dir);
	vfs_put_dentry(dev_dir);
// 	struct super_block *dev_sb = vfs_get_super(NO_DEV, DEV_FS_TYPE);
	kern_init_block_dev();
	kern_init_char_dev();
}

PUBLIC void register_fs_types() {
	register_fs_type("devfs", DEV_FS_TYPE, 
		&devfs_file_ops,
		&devfs_inode_ops,
		NULL,
		&devfs_sb_ops,
		NULL);
	
	register_fs_type("orangefs", ORANGE_TYPE,
		&orange_file_ops, 
		&orange_inode_ops,
		NULL,
		&orange_sb_ops,
		orangefs_identify);
		
	register_fs_type("fat32", FAT32_TYPE,
		&fat32_file_ops,
		&fat32_inode_ops,
		&fat32_dentry_ops,
		&fat32_sb_ops,
		fat32_identify);
}

PUBLIC void init_fs(){
	initlock(&inode_alloc_lock, "");
	initlock(&file_desc_lock, "");
	init_file_desc_table();
	list_init(&inode_free_list);
	inode_free_cnt = 0;
	mount_root(SATA_BASE);
	init_fsroot();
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
	int dev = block_dev->d_inode->i_b_cdev;
	if(block_dev->d_inode->i_type != I_BLOCK_SPECIAL){
		// 不是块设备，挂载失败
		dev = -1;
	}
	release(&block_dev->d_inode->lock);
	vfs_put_dentry(block_dev);
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
	char dev_name[MNT_DEVNAME_LEN];
	vfs_get_path(block_dev, dev_name, MNT_DEVNAME_LEN);
	struct vfs_dentry * mnt_root = kern_mount_dev(dev, fstype, dev_name, dir_d);
	if(!mnt_root){
		vfs_put_dentry(dir_d);
		return -1;
	}
	dir_d->d_mounted = 1;
	vfs_put_dentry(dir_d);
	return 0;
}

PUBLIC int kern_vfs_umount(const char *target){
	struct vfs_dentry *mnt = vfs_lookup(target), *mountpoint;
	if(!mnt){
		return -1;
	}
	mountpoint = remove_vfsmnt(mnt);
	vfs_put_dentry(mnt);
	if(!mountpoint){
		return -1;
	}
	mountpoint->d_mounted = 0;
	// mnt->
	vfs_put_dentry(mountpoint);
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
		acquire(&dentry->lock);
		insert_sub_dentry(dir, dentry);
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
	file->fd_dentry = dentry;
	file->fd_mode = rmode;//fd_mode: bit 0:read 1:write
	file->fd_pos = 0;	
	release(&inode->lock);
	release(&dentry->lock);
	vfs_put_dentry(dir);
	return file;
err:
	release(&inode->lock);
	vfs_put_dentry(dentry);
no_dentry_out:
	vfs_put_dentry(dir);
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

PUBLIC int kern_vfs_close(int fd) {
	PROCESS* p_proc = proc_real(p_proc_current);
	struct file_desc* file = p_proc->task.filp[fd];
	if(!file){
		return -1;
	}
	if(atomic_dec_and_test(&file->fd_count)){
		acquire(&file_desc_lock);
		vfs_put_dentry(file->fd_dentry);
		file->fd_dentry = 0;
		file->flag = 0;
		release(&file_desc_lock);
	}
	p_proc->task.filp[fd] = 0;
	return 0;
}

PUBLIC int kern_vfs_lseek(int fd, int offset, int whence){
	PROCESS* p_proc = proc_real(p_proc_current);
	struct file_desc * file = p_proc->task.filp[fd];
	if(!file){
		return -1;
	}
	struct vfs_inode * inode = file->fd_dentry->d_inode;
	u64 size;
	acquire(&inode->lock);
	if(!inode_allow_lseek(inode->i_type)){
		return -1;
	}
	size = inode->i_size;
	release(&inode->lock);
	u64 _offset = -1;
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
	PROCESS* p_proc = proc_real(p_proc_current);
	struct file_desc * file = p_proc->task.filp[fd];
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
	PROCESS* p_proc = proc_real(p_proc_current);
	struct file_desc * file = p_proc->task.filp[fd];
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
	struct vfs_dentry *dir = vfs_lookup(dir_path), *dentry = NULL;
	if(!dir){
		return -1;
	}
	dentry = _do_lookup(dir, file_name, 0);
	if(!dentry){
		release(&dir->lock);
		return -1;
	}
	struct vfs_inode *inode = dentry->d_inode;
	acquire(&inode->lock);
	if(inode->i_type == I_DIRECTORY){
		goto err;	
	}
	if(inode->i_op && inode->i_op->unlink){
		acquire(&dir->d_inode->lock);
		int state = inode->i_op->unlink(dir->d_inode, dentry);
		release(&dir->d_inode->lock);
		if(state){
			goto err;
		}
		inode->i_nlink--;
	}
	release(&inode->lock);
	delete_dentry(dentry, dir);
	vfs_put_dentry(dentry);
	vfs_put_dentry(dir);
	return 0;
err:
	release(&inode->lock);
	vfs_put_dentry(dentry);
	vfs_put_dentry(dir);
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
		acquire(&dentry->lock);
		insert_sub_dentry(dir, dentry);
	}
	vfs_put_dentry(dentry);
	vfs_put_dentry(dir);
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
		vfs_put_dentry(dentry);
		vfs_put_dentry(dir);
		return -1;
	}
	dentry = vfs_create(dir->d_inode, file_name, I_DIRECTORY, mode&(~I_TYPE_MASK), 0);
	if(!dentry) {
		vfs_put_dentry(dir);
		return -1;
	}
	acquire(&dentry->lock);
	insert_sub_dentry(dir, dentry);
	vfs_put_dentry(dentry);
	vfs_put_dentry(dir);
	return 0;
}

PUBLIC int kern_vfs_rmdir(const char* path){
	char dir_path[MAX_PATH] = {0};
	char* file_name = strip_dir_path(path, dir_path);
	if(file_name[strlen(file_name) - 1] == '.') {
		disp_str(". in name end is invalid ");
		return -1;
	}
	struct vfs_dentry *dir = vfs_lookup(dir_path), *dentry = NULL;
	if(!dir){
		return -1;	// cant find parent
	}
	dentry = _do_lookup(dir, file_name, 0);
	if(!dentry){
		vfs_put_dentry(dir);
		return -1;	// cant find target dir
	}
	struct vfs_inode *inode = dentry->d_inode;
	acquire(&inode->lock);
	if(dentry == vfs_root || (dentry->d_vfsmount && dentry->d_vfsmount->mnt_root == dentry)){// 不允许删除挂载点
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
		inode->i_nlink--;
	}
	release(&inode->lock);
	struct vfs_dentry* sub;
	// clear sub dentry
	while(!list_empty(&dentry->d_subdirs)){
		sub = list_entry(&dentry->d_subdirs.next, struct vfs_dentry, d_list);
		delete_dentry(sub, dentry);
	}
	delete_dentry(dentry, dir);
	vfs_put_dentry(dentry);
	vfs_put_dentry(dir);
	return 0;
err:
	release(&inode->lock);
	vfs_put_dentry(dentry);
	vfs_put_dentry(dir);
	return -1;
}

PUBLIC DIR* kern_vfs_opendir(const char* path){
	struct file_desc * file = vfs_file_open(path, O_DIRECTORY|O_RDONLY, I_RWX);
	if(!file){
		return NULL;
	}
	DIR* dirp = kern_malloc_4k();
	dirp->file = file;
	dirp->pos = 0;
	dirp->total = 0;
	dirp->init = 0;
	return dirp;
}

PUBLIC int kern_vfs_closedir(DIR* dirp){
	struct file_desc* file = dirp->file;
	if(!file){
		return -1;
	}
	if(atomic_dec_and_test(&file->fd_count)){
		acquire(&file_desc_lock);
		vfs_put_dentry(file->fd_dentry);
		file->fd_dentry = 0; // modified by mingxuan 2019-5-17
		file->flag = 0;
		release(&file_desc_lock);
	}
	kern_free_4k(dirp);
	return 0;
}

struct dirent* kern_vfs_readdir(DIR* dirp){
	if(!dirp->init){
		struct vfs_inode* inode = dirp->file->fd_dentry->d_inode;
		struct dirent* data_start = DIR_DATA(dirp);
		acquire(&inode->lock);
		if(inode->i_fop && inode->i_fop->readdir){
			dirp->total = inode->i_fop->readdir(dirp->file, num_4K - sizeof(DIR), data_start);
		}
		dirp->init = 1;
		release(&inode->lock);
	}
	struct dirent* ent = NULL;
	if(dirp->pos < dirp->total){
		ent = (struct dirent*)(((char*)DIR_DATA(dirp)) + dirp->pos);
		dirp->pos += ent->d_len;
		if(!strcmp(ent->d_name, "..")){// check parent needed for mount point
			vfs_get_dentry(dirp->file->fd_dentry);
			struct vfs_dentry* found = _do_lookup(dirp->file->fd_dentry, ent->d_name, 1);
			if(found){
				ent->d_ino = found->d_inode->i_no;
				vfs_put_dentry(found);
			}
		}
	}
	return ent;
}

PUBLIC int kern_vfs_chdir(const char* path){
	PROCESS* p_proc = proc_real(p_proc_current);
	struct vfs_dentry *dir = vfs_lookup(path);
	if(!dir){
		return -1;
	}
	struct vfs_inode* inode = dir->d_inode;
	int err = -1;
	if(inode->i_type == I_DIRECTORY){
		if(p_proc->task.cwd) {
			vfs_put_dentry(p_proc->task.cwd);
		}
		p_proc->task.cwd = dir;
		err = 0;
	}
	release(&dir->lock);
	return err;
}


PUBLIC char* kern_vfs_getcwd(char *buf, int size) {
	PROCESS* p_proc = proc_real(p_proc_current);
    vfs_get_path(p_proc->task.cwd, buf, size);
    return buf;
}

PUBLIC int kern_vfs_stat(const char *path, struct stat *statbuf) {
	struct vfs_dentry *dentry = vfs_lookup(path);
	if(NULL == dentry) {
		return -1;
	}
	struct vfs_inode *inode = dentry->d_inode;
	statbuf->st_dev = inode->i_dev;
	statbuf->st_ino = inode->i_no;
	statbuf->st_type = inode->i_type;
	statbuf->st_mode = inode->i_mode;
	statbuf->st_nlink = inode->i_nlink;
	statbuf->st_rdev = inode->i_b_cdev;
	statbuf->st_size = inode->i_size;
	statbuf->st_atim = inode->i_atime;
	statbuf->st_mtim = inode->i_mtime;
	statbuf->st_crtim = inode->i_crtime;
	vfs_put_dentry(dentry);
	return 0;
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

PUBLIC int do_vstat(const char *pathname, struct stat* statbuf) {
	return (int)kern_vfs_stat(pathname, statbuf);
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

PUBLIC int sys_stat() {
	return do_vstat((const char*)get_arg(1), (struct stat*)get_arg(2));
}