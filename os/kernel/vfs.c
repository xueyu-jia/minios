/**********************************************************
*	vfs.c       //added by mingxuan 2019-5-17
***********************************************************/

#include "const.h"
#include "global.h"
#include "string.h"
#include "proto.h"
#include "fs.h"
#include "tty.h"
#include "vfs.h"
#include "hd.h"
#include "mount.h"

// PRIVATE struct device  device_table[NR_DEV];  //deleted by mingxuan 2020-10-18
// PRIVATE struct vfs  vfs_table[NR_FS];   //modified by mingxuan 2020-10-18
PUBLIC struct vfs  vfs_table[NR_FS]; //modified by ran
extern mount_table mnt_table[MAX_mnt_table_length];
PUBLIC struct file_desc f_desc_table[NR_FILE_DESC];
PUBLIC struct super_block super_blocks[NR_SUPER_BLOCK]; //added by mingxuan 2020-10-30
PUBLIC struct fs fs_table[NR_FS_TYPE];
PRIVATE vfs_inode vfs_inode_table[NR_INODE];
#define INODE_BMAP_SIZE (NR_INODE/8)
PUBLIC char vfs_bmap[INODE_BMAP_SIZE];
struct vfs_dentry *vfs_root;
PRIVATE SPIN_LOCK inode_alloc_lock;
PRIVATE SPIN_LOCK file_desc_lock;
PRIVATE SPIN_LOCK superblock_lock;
// PRIVATE struct file_op f_op_table[NR_fs]; //文件系统操作表
PRIVATE struct file_op f_op_table[NR_FS_OP]; //modified by mingxuan 2020-10-18
PRIVATE struct sb_op   sb_op_table[NR_SB_OP];   //added by mingxuan 2020-10-30


//PRIVATE void init_dev_table();//deleted by mingxuan 2020-10-30
PRIVATE void init_vfs_table();  //modified by mingxuan 2020-10-30
PUBLIC void init_file_desc_table();   //added by mingxuan 2020-10-30
PUBLIC void init_fileop_table();
PUBLIC void init_super_block_table();  //added by mingxuan 2020-10-30

//added by ran
PUBLIC struct vfs* vfs_alloc_vfs_entity()
{
    int i;
    for (int i = 0; i < NR_FS; i++)
    {
        if (!vfs_table[i].used)
        {
            return &vfs_table[i];
        }
    }
    return 0;
}

//PRIVATE int get_index(char path[]);
PUBLIC int get_index(char path[]); //modified by ran

// get and alloc an inode, 调用此函数分配新的inode, 
PUBLIC struct vfs_inode * vfs_get_inode(){
	struct vfs_inode *res = 0;
	char *s = vfs_bmap, *e = vfs_bmap + INODE_BMAP_SIZE, c;
	acquire(&inode_alloc_lock);
	while(s < e){
		if((c = *s) != 0xFF){
			int i = 0;
			while((c & (1<<i)) && i < 8){
				i++;
			}
			*s = c | (1<<i); // mark used;
			res = vfs_inode_table + ((s-vfs_bmap) << 3) + i;
			break;
		}
		s++;
	}
	release(&inode_alloc_lock);
	if(res){
		memset(res, 0, sizeof(vfs_inode));
		initlock(&res->lock, NULL);
		res->i_count = 1;
	}
	return res;
}

// 如果引用计数归0，释放资源, 调用此函数必须持有inode锁,除非inode还没有加入目录树
// 如果引用计数归0，且引用的文件数也为0，执行删除文件
PUBLIC void vfs_put_inode(struct vfs_inode * inode){
	if(--inode->i_count == 0){
		acquire(&inode_alloc_lock);
		int i = (inode - vfs_inode_table);
		vfs_bmap[i >> 3] &= ~(1 << (i%8));
		release(&inode_alloc_lock);
	}
}

// require mutex: dir, ent
PRIVATE void insert_sub_dentry(struct vfs_dentry* dir, struct vfs_dentry* ent){
	ent->d_nxt = dir->d_subdirs;
	if(dir->d_subdirs){
		dir->d_subdirs->d_pre = ent;
	}
	dir->d_subdirs = ent;
	ent->d_parent = dir;
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

PRIVATE struct vfs_dentry * alloc_dentry(char* name, struct vfs_inode* inode){
	struct vfs_dentry* entry;
	entry = kern_kmalloc(sizeof(struct vfs_dentry));
	initlock(&entry->lock, NULL);
	strcpy(entry->d_name, name);
	entry->d_inode = inode;
	entry->d_covers = entry;
	entry->d_mounts = entry;
	entry->d_parent = NULL;
	inode->i_count++;
	entry->d_nxt = entry->d_pre = entry->d_subdirs = NULL;
	return entry;
}

// alloc a new dentry for filled inode in dir
// require mutex: dir, inode
PUBLIC struct vfs_dentry * new_dentry(char* name, struct vfs_inode* dir, struct vfs_inode* inode){
	struct vfs_dentry* entry;
	entry = alloc_dentry(name, inode);
	if(inode->i_type == I_DIRECTORY){
		struct vfs_dentry *cur, *par;
		cur = alloc_dentry(".", inode);
		insert_sub_dentry(entry, cur);
		par = alloc_dentry("..", dir);
		insert_sub_dentry(entry, par);
	}
	return entry;
}

// delete dentry in dir
// require mutex: dir, dentry, dentry.inode
PUBLIC int delete_dentry(struct vfs_dentry* dentry, struct vfs_dentry* dir){
	if(dentry->d_subdirs || dentry->d_mounts != dentry){
		// check empty
		// disp_str("try to delete non empty dentry");
		// for dir, you must delete . and .. in dentry
		//  before calling delete_dentry
		return -1;
	}
	remove_sub_dentry(dir, dentry);
	vfs_put_inode(dentry->d_inode);
	kern_kfree(dentry);
	return 0;
}

PRIVATE void register_fs_type(char* fstype_name, int fs_type, 
	struct file_operations* f_op, 
	struct inode_operations* i_op,
	struct dentry_operations* d_op,
	struct superblock_operations* s_op){
	struct fs* fs = &fs_table[fs_type];
	fs->fstype_name = fstype_name;
	fs->fs_fop = f_op;
	fs->fs_iop = i_op;
	fs->fs_dop = d_op;
	fs->fs_sop = s_op;
}

PRIVATE void strip_base_path(char *path, char** file_name){
	int len = strlen(path), flag = 0;
	char *p = path + len - 1;
	while(*p){
		if(*p == '/'){
			*file_name = p + 1;
			*p = 0;
			break;
		}
		p--;
	}
}

PUBLIC int vfs_check_exec_permission(struct vfs_inode* inode){
	acquire(&inode->lock);
	int mode = inode->i_mode;
	release(&inode->lock);
	if(!(mode & 1)){
		return -1;
	}
	return 0;
}
// 上锁规则: 上层->下层目录:持有上层锁获取下层锁,已经持有下层锁的不允许获取上层锁,
//(todo:对于访问..可能产生的顺序是否存在死锁的可能,有待探究)
// dir lock must held, return entry with lock
// 参数dir dentry带锁,返回有效dentry也带锁
PRIVATE struct vfs_dentry * _do_lookup(struct vfs_dentry *dir, char *name){
	// acquire(&dir->lock);
	struct vfs_dentry* entry = NULL;
	if(!strcmp(dir->d_name, ".")){
		entry = dir;
	}else if(!strcmp(dir->d_name, "..")){
		entry = dir->d_covers->d_parent; // 对于挂载点下的dentry，上层是挂载前的
	}
	if(!entry){
		entry = dir->d_subdirs;
		// int new_dentry = 0;
		for( ;entry; entry = entry->d_nxt){
			if(!strcmp(entry->d_name, name)){
				break;
			}
		}
	}
	if(!entry){
		acquire(&dir->d_inode->lock);
		struct inode_operations * i_ops = NULL;
		int type = dir->d_inode->i_type;
		if(type == I_MOUNTPOINT || type == I_DIRECTORY){
			i_ops = dir->d_inode->i_op;
		}
		if(i_ops){
			entry = i_ops->lookup(dir->d_inode, name);
			if(entry){
				insert_sub_dentry(dir, entry);
				// new_dentry = 1;
			}
		}
		/* lookup内部应有的流程: 
		 *  1. 按照相应文件系统结构读取磁盘查找; 
		 *  2. 若查找成功: .1 调用vfs_get_inode 得到一个新inode,填入相应数据
		 *               .2 调用new_dentry 初始化新的dentry
		 *  3. 返回结果dentry的指针
		 *  注：每次新的lookup分配新inode之后，其引用计数为1，保证dentry中的inode有效
		 *     对于open的文件,引用计数再加1,close之后不会归0释放
		 *     可以简单认为目录树的dentry对于相应的inode都有一次额外的引用
		 *     删除dentry时,对应的引用计数清除,inode才有可能释放
		 */
		release(&dir->d_inode->lock);
	}
	if(entry != entry->d_mounts){
		entry = entry->d_mounts;// 对于普通目录项应相等，挂载点更新到对应文件系统的dentry
	}
	if(entry != dir){ // 排除当前目录
		if(entry){// 此时不管entry是命中的还是新创建的都已经在目录树中了，一锁换一锁
			acquire(&entry->lock);
		}
		release(&dir->lock);
	}
	return entry;
}

// 输入：根dentry base (已获得锁)
PUBLIC struct vfs_dentry *vfs_lookup(struct vfs_dentry *base, char *path){
	if(!path || !base || !(base->d_inode)){
		return NULL;
	}
	char d_name[32];
	char *p = path, *p_name = d_name;
	int flag_name = 0;
	struct vfs_dentry * dir = base;
	while(*p == '/')p++;
	while(*p || flag_name){
		if(*p == '/' || *p == 0){// meet file name end
			if(flag_name){
				*p_name = 0;
				flag_name = 0;
				dir = _do_lookup(dir, d_name);
				if(!dir){
					return NULL;
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

PUBLIC struct super_block * vfs_read_super(int dev, int fstype){
	if(hd_info[MAJOR(dev)].part[MINOR(dev)].fs_type != fstype){
		// dismatch fstype
		return NULL;
	}
	struct fs* file_sys = &fs_table[fstype];
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

PUBLIC void mount_root(){
	int root_dev = SATA_BASE;
	int root_fstype = ORANGE_TYPE;
	int dev = get_fs_dev(root_dev, ORANGE_TYPE);
	struct super_block *sb = vfs_read_super(dev, root_fstype);
	vfs_root = sb->root;
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
	
	mount_root();
}

PUBLIC int get_fstype_by_name(char* fstype_name){
	int fstype = NO_FS_TYPE;
	for(;fstype < NR_FS_TYPE; fstype++){
		if(!strcmp(fstype_name, fs_table[fstype].fstype_name)){
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
	//将字符串从用户空间复制到内核空间
    char block_filepath[MAX_PATH];
    char mntpoint_path[MAX_PATH];
    memset(mntpoint_path,0,MAX_PATH);
    memset(block_filepath,0,MAX_PATH);
    strcpy(block_filepath,source);
    strcpy(mntpoint_path,target);

    // modified by sundong 2023.5.28
    //int device = get_dev_from_name(source);
    //从根文件系统中获取块设备的设备号
	acquire(&vfs_root->lock);
	struct vfs_dentry* block_dev = vfs_lookup(vfs_root, block_filepath);
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
	
	acquire(&vfs_root->lock);
	struct vfs_dentry* dir_d = vfs_lookup(vfs_root, mntpoint_path);
	int fstype = get_fstype_by_name(filesystemtype);
	struct super_block *sb = vfs_read_super(dev, fstype);
	add_vfsmount(block_filepath, mntpoint_path, sb->root, dev);
	sb->root->d_covers = dir_d;
	dir_d->d_mounts = sb->root;
	release(&dir_d->lock);
	return 0;
}

PRIVATE struct file_desc * vfs_file_open(const char* path, int flags, int mode){
	int i;
	char full_path[MAX_PATH] = {0};
	char* file_name, *base_path = full_path;
	strcpy(full_path, path);
	strip_base_path(base_path, &file_name);
	acquire(&vfs_root->lock);
	struct vfs_dentry *dir = vfs_lookup(vfs_root, base_path), *entry = NULL;
	struct vfs_inode *inode = NULL;
	if(dir){
		entry = _do_lookup(dir, file_name);
	}
	if(!entry && (flags & O_CREAT)){
		//此处不调用createAPI,避免锁的一致性问题,
		//同时根据POSIX应将creat实现为open的特殊情况	
		if(dir){
			struct vfs_inode* inode = NULL;
			struct inode_operations * i_ops = NULL;
			acquire(&dir->d_inode->lock);
			int type = dir->d_inode->i_type;
			if(type == I_DIRECTORY){
				i_ops = dir->d_inode->i_op;
			}
			if(i_ops){
				inode = vfs_get_inode();
				inode->i_mode = mode&(~I_TYPE_MASK);
				inode->i_type = I_REGULAR;
				struct vfs_dentry * dentry = alloc_dentry(file_name, inode);
				int state = i_ops->create(dir->d_inode, dentry);
				if(state == 0){
					insert_sub_dentry(dir, dentry);
					entry = dentry;
					acquire(&entry->lock);
				}else{
					// create error
					vfs_put_inode(inode);
					inode = NULL;
				}
			}
			release(&dir->d_inode->lock);
			release(&dir->lock);
		}
	}
	if(!entry){
		return NULL;
	}
	inode = entry->d_inode;
	acquire(&inode->lock);
	if(inode->i_type == I_DIRECTORY){
		disp_str("error: try to open a dir");
		release(&inode->lock);
		release(&entry->lock);
		return NULL;
	}
	acquire(&file_desc_lock);
	/* find a free slot in f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++){
		if (f_desc_table[i].flag == 0)
			break;
	}
	if (i >= NR_FILE_DESC){
		release(&file_desc_lock);
		disp_str("f_desc_table[] is full (PID:");
		disp_int(proc2pid(p_proc_current));
		disp_str(")\n");
		return  NULL;
	}
	f_desc_table[i].flag = 1;
	release(&file_desc_lock);
	inode->i_count++;
	f_desc_table[i].fd_inode = inode;
	f_desc_table[i].fd_mode = flags;
	f_desc_table[i].fd_pos = 0;	
	release(&inode->lock);
	release(&entry->lock);
	return &f_desc_table[i];
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
    struct vfs_inode* inode = p_proc_current->task.filp[fd]->fd_inode;
	acquire(&inode->lock);
	vfs_put_inode(inode);
	release(&inode->lock);
	p_proc_current->task.filp[fd]->fd_inode = 0; // modified by mingxuan 2019-5-17
	acquire(&file_desc_lock);
	p_proc_current->task.filp[fd]->flag = 0;
	release(&file_desc_lock);			 // added by mingxuan 2019-5-17
	p_proc_current->task.filp[fd] = 0;
}

PUBLIC int kern_vfs_lseek(int fd, int offset, int whence){
	struct file_desc * file = p_proc_current->task.filp[fd];
	if(!file){
		return -1;
	}
	struct vfs_inode * inode = file->fd_inode;
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
	struct vfs_inode * inode = file->fd_inode;
	int cnt = -1;
	acquire(&inode->lock);
	if(inode->i_fop && inode->i_fop->read){
		cnt = inode->i_fop->read(file, count, buf);
	}
	release(&inode->lock);
	return cnt;
}

PUBLIC int kern_vfs_write(int fd, char *buf, int count){
	struct file_desc * file = p_proc_current->task.filp[fd];
	if(!file || count<0){
		return -1;
	}
	struct vfs_inode * inode = file->fd_inode;
	int cnt = -1;
	acquire(&inode->lock);
	if(inode->i_fop && inode->i_fop->write){
		cnt = inode->i_fop->write(file, count, buf);
	}
	release(&inode->lock);
	return cnt;
}

PUBLIC int kern_vfs_creat(char* path, int mode){
	return kern_vfs_open(path, O_CREAT, mode);
}

PUBLIC int kern_vfs_unlink(){

}
PUBLIC int kern_vfs_mknod(char* path, int mode, int dev){
	char full_path[MAX_PATH] = {0};
	char* file_name, *base_path = full_path;
	strcpy(full_path, path);
	strip_base_path(base_path, &file_name);
	acquire(&vfs_root->lock);
	struct vfs_dentry *dir = vfs_lookup(vfs_root, base_path), *entry = NULL;
	struct vfs_inode *inode = NULL;
	if(dir){
		entry = _do_lookup(dir, file_name);
	}
	if(entry){
		release(&entry->lock);
		return -1;
	}
	if(dir){
		struct vfs_inode* inode = NULL;
		struct inode_operations * i_ops = NULL;
		acquire(&dir->d_inode->lock);
		int type = dir->d_inode->i_type;
		if(type == I_DIRECTORY){
			i_ops = dir->d_inode->i_op;
		}
		if(i_ops){
			inode = vfs_get_inode();
			struct vfs_dentry * dentry = alloc_dentry(file_name, inode);
			int state = i_ops->create(dir->d_inode, dentry);
			inode->i_mode = mode&(~I_TYPE_MASK);
			inode->i_type = mode&I_TYPE_MASK;
			inode->i_dev = dev;
			switch (inode->i_type)
			{
			case I_CHAR_SPECIAL:
				inode->i_fop = &tty_file_ops;
				break;
			
			default:
				break;
			}
			if(state == 0){
				insert_sub_dentry(dir, dentry);
				entry = dentry;
				acquire(&entry->lock);
			}else{
				// create error
				vfs_put_inode(inode);
				inode = NULL;
			}
		}
		release(&dir->d_inode->lock);
		release(&dir->lock);
	}
	return 0;
}

PUBLIC int kern_vfs_mkdir(char* path, int mode){
	char full_path[MAX_PATH] = {0};
	char* file_name, *base_path = full_path;
	strcpy(full_path, path);
	strip_base_path(base_path, &file_name);
	acquire(&vfs_root->lock);
	struct vfs_dentry *dir = vfs_lookup(vfs_root, base_path), *entry = NULL;
	struct vfs_inode *inode = NULL;
	if(dir){
		entry = _do_lookup(dir, file_name);
	}
	if(entry){
		release(&entry->lock);
		return -1;
	}
	if(dir){
		struct vfs_inode* inode = NULL;
		struct inode_operations * i_ops = NULL;
		acquire(&dir->d_inode->lock);
		int type = dir->d_inode->i_type;
		if(type == I_DIRECTORY){
			i_ops = dir->d_inode->i_op;
		}
		if(i_ops){
			inode = vfs_get_inode();
			inode->i_mode = mode&(~I_TYPE_MASK);
			inode->i_type = I_DIRECTORY;
			struct vfs_dentry * dentry = alloc_dentry(file_name, inode);
			int state = i_ops->mkdir(dir->d_inode, dentry);
			if(state == 0){
				insert_sub_dentry(dir, dentry);
				entry = dentry;
				acquire(&entry->lock);
			}else{
				// create error
				vfs_put_inode(inode);
				inode = NULL;
			}
		}
		release(&dir->d_inode->lock);
		release(&dir->lock);
	}
	return 0;
}

PUBLIC void init_vfs(){

    init_file_desc_table();
    init_fileop_table();

    init_super_block_table();
    init_sb_op_table(); //added by mingxuan 2020-10-30

    //init_dev_table(); //deleted by mingxuan 2020-10-30
    init_vfs_table();   //modified by mingxuan 2020-10-30
}

//added by mingxuan 2020-10-30
PUBLIC void init_file_desc_table(){
    int i;
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));
}

PUBLIC void init_fileop_table(){
    // table[0] for tty
    f_op_table[0].open = real_open;
    f_op_table[0].close = real_close;
    f_op_table[0].write = real_write;
    f_op_table[0].lseek = real_lseek;
    f_op_table[0].unlink = real_unlink;
    f_op_table[0].read = real_read;
    //f_op_table[0].tag = 0;

    // table[1] for orange
    f_op_table[1].open = real_open;
    f_op_table[1].close = real_close;
    f_op_table[1].write = real_write;
    f_op_table[1].lseek = real_lseek;
    f_op_table[1].unlink = real_unlink;
    f_op_table[1].read = real_read;
    f_op_table[1].createdir = real_createdir;
    f_op_table[1].deletedir = real_deletedir;
    //f_op_table[1].tag = 0;

    // table[2] for fat32
    // f_op_table[2].create = CreateFile;
    // f_op_table[2].delete = DeleteFile;
    // f_op_table[2].open = OpenFile;
    // f_op_table[2].close = CloseFile;
    // f_op_table[2].write = WriteFile;
    // f_op_table[2].read = ReadFile;
    // f_op_table[2].lseek = LSeek;
    // f_op_table[2].opendir = OpenDir;
    // f_op_table[2].createdir = CreateDir;
    // f_op_table[2].deletedir = DeleteDir;
    // f_op_table[2].chdir = fat32_chdir;

    // table[2] for fat32
    // f_op_table[2].CreateFile = CreateFile;
    // f_op_table[2].DeleteFile = DeleteFile;
    // f_op_table[2].OpenFile = OpenFile;
    // f_op_table[2].CloseFile = CloseFile;
    // f_op_table[2].WriteFile = WriteFile;
    // f_op_table[2].ReadFile = ReadFile;
    // f_op_table[2].LSeek = LSeek;
    // f_op_table[2].OpenDir = OpenDir;
    // f_op_table[2].CreateDir = CreateDir;
    // f_op_table[2].DeleteDir = DeleteDir;
    // f_op_table[2].ChangeDir = fat32_chdir;
    // f_op_table[2].ReadDir = ReadDir;

    // table[2] for fat32 //added by ran
    // f_op_table[2].create = create_adapter;
    // f_op_table[2].delete = delete_adapter;
    // f_op_table[2].open = open_adapter;
    // f_op_table[2].close = close_adapter;
    // f_op_table[2].write = write_adapter;
    // f_op_table[2].read = read_adapter;
    // f_op_table[2].lseek = LSeek;
    // f_op_table[2].opendir = opendir_adapter;
    // f_op_table[2].createdir = createdir_adapter;
    // f_op_table[2].deletedir = deletedir_adapter;
    // f_op_table[2].readdir = readdir_adapter;
    // f_op_table[2].chdir = chdir_adapter;
    //f_op_table[2].tag = 1;

}

//added by mingxuan 2020-10-30
PUBLIC void init_super_block_table(){
    struct super_block * sb = super_blocks;						//deleted by mingxuan 2020-10-30

    //super_block[0] is tty0, super_block[1] is tty1, uper_block[2] is tty2
    for(; sb < &super_blocks[3]; sb++) {
        sb->sb_dev =  DEV_CHAR_TTY;
        // sb->fs_type = TTY_FS_TYPE;
        sb->used = 1;
    }

    //super_block[3] is orange's superblock
    sb->sb_dev = DEV_HD;
    sb->fs_type = ORANGE_TYPE;
    sb->used = 1;
    sb++;

    //deleted by ran
    //super_block[4] is fat32's superblock
    // sb->sb_dev = DEV_HD;
    // sb->fs_type = FAT32_TYPE;
    // sb++;

    //another super_block are free
    for (; sb < &super_blocks[NR_SUPER_BLOCK]; sb++)			//deleted by mingxuan 2020-10-30
	{
    	sb->sb_dev = NO_DEV;
        sb->fs_type = NO_FS_TYPE;
        sb->used = 0;
    }
}

//added by mingxuan 2020-10-30
PUBLIC void init_sb_op_table(){
    //orange
    sb_op_table[0].read_super_block = read_super_block;
    sb_op_table[0].get_super_block = get_super_block;

    // //fat32 and tty
    sb_op_table[1].read_super_block = NULL;
    sb_op_table[1].get_super_block = NULL;
}

//PRIVATE void init_dev_table(){
PRIVATE void init_vfs_table(){  // modified by mingxuan 2020-10-30


    // 我们假设每个tty就是一个文件系统
    // tty0
    // device_table[0].dev_name="dev_tty0";
    // device_table[0].op = &f_op_table[0];
    vfs_table[0].fs_name = "dev_tty0"; //modifed by mingxuan 2020-10-18
    vfs_table[0].op = &f_op_table[0];
    vfs_table[0].sb = &super_blocks[0];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[0].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30
    vfs_table[0].used = 1;

    // tty1
    //device_table[1].dev_name="dev_tty1";
    //device_table[1].op =&f_op_table[0];
    vfs_table[1].fs_name = "dev_tty1"; //modifed by mingxuan 2020-10-18
    vfs_table[1].op = &f_op_table[0];
    vfs_table[1].sb = &super_blocks[1];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[1].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30
    vfs_table[1].used = 1;

    // tty2
    //device_table[2].dev_name="dev_tty2";
    //device_table[2].op=&f_op_table[0];
    vfs_table[2].fs_name = "dev_tty2"; //modifed by mingxuan 2020-10-18
    vfs_table[2].op = &f_op_table[0];
    vfs_table[2].sb = &super_blocks[2];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[2].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30
    vfs_table[2].used = 1;

    // // orangefs
    vfs_table[3].fs_name = "orange";
    vfs_table[3].op = &f_op_table[1];
    vfs_table[3].sb = &super_blocks[3];
    vfs_table[3].s_op = &sb_op_table[0];
    vfs_table[3].used = 1;

    // fat32 //added by ran
    // vfs_table[4].fs_name = "fat0";
    // vfs_table[4].op = &f_op_table[2];
    // vfs_table[4].sb = &super_block[4];
    // vfs_table[4].s_op = &sb_op_table[1];
    // vfs_table[4].used = 0; //动态绑定

    // fat32 //added by ran
    // vfs_table[5].fs_name = "fat1";
    // vfs_table[5].op = &f_op_table[2];
    // vfs_table[5].sb = &super_block[5];
    // vfs_table[5].s_op = &sb_op_table[1];
    // vfs_table[5].used = 0; //动态绑定

    for(int i=4;i<NR_FS;i++)
    {
        vfs_table[i].fs_name = "";
        vfs_table[i].op = &f_op_table[0];
        vfs_table[i].sb = &super_blocks[i];
        vfs_table[i].s_op = &sb_op_table[0];
        vfs_table[i].used = 0; //动态绑定
    }
}

PUBLIC int set_vfstable(u32 device, char *target)
{
    int fs_type = hd_info[MAJOR(device)].part[MINOR(device)].fs_type;

    int vfs_index;
    struct vfs *pvfs = vfs_table;

    for(vfs_index = 0; vfs_index<NR_FS;vfs_index++,pvfs++)
    {
        if(pvfs->used == 1 && pvfs->sb->sb_dev == device)
        {
            break;
        }
    }

    if(pvfs == vfs_table+NR_FS)
    {
        pvfs = vfs_alloc_vfs_entity();
    }
    
    
    pvfs->fs_name = (char*)kern_kmalloc(12);
    strcpy(pvfs->fs_name, target);

    int sb_index;

    if( fs_type== ORANGE_TYPE)
    {
        sb_index = init_orangefs(device);
        pvfs->op = &f_op_table[1];
        pvfs->s_op = &sb_op_table[0];
    }
    else if(fs_type== FAT32_TYPE)
    {
        // sb_index = init_fat32fs(device);
        pvfs->op = &f_op_table[2];
        pvfs->s_op = &sb_op_table[1];
    }
    else 
    {
        disp_color_str("not support this fs foramt\n", 0x74);
        return -1;
    }

    if(pvfs-vfs_table != sb_index)
    {
        disp_str("Warning!!!, vfstable index not equal with sb_index\n");
    }

    pvfs->sb = &super_blocks[sb_index];
    pvfs->used = 1;

    return pvfs - vfs_table;
}

// mark 限制了文件名长度。函数功能与名字不符
PRIVATE int get_fs_len(const char *path) {
  int pathlen = strlen(path);
  //char dev_name[DEV_NAME_LEN];
  char fs_name[DEV_NAME_LEN] = {0};   //modified by mingxuan 2020-10-18
  int len = (pathlen < DEV_NAME_LEN) ? pathlen : DEV_NAME_LEN;
  int i,a=0;
  for(i=0;i<len;i++){
    if( path[i] == '/'){
      a=i;
      a++;
      break;
    }
    else {
      //dev_name[i] = path[i];
      fs_name[i] = path[i];   //modified by mingxuan 2020-10-18
    }
  }
  return strlen(fs_name);
}
/*根据绝对路径名获取该路径描述的文件所在的文件系统在vfs_table中的index
并且将绝对路径转化为该文件系统的相对路径
*/
/* int getIndex_and_stripPath(char* path){
    //遍历所有挂载点，查看path中是否包含挂载点的路径
    //若包含则返回挂载点挂载的文件系统在vfs_table的索引
    //若不包含则返回orangefs(根文件系统)在vfs_table的索引
    int index = 3;//默认是orangefs 根文件系统
    int mnt_point_len = 0;
    for (int i = 0; i < MAX_mnt_table_length; i++)
    {
         mnt_point_len = strlen(mnt_table[i].filename);
        if(strncmp(mnt_table[i].filename,path,strlen) == 0){
            index = i;
            break;
        }
    }
    //将绝对地址转化为文件系统内的相对地址
    if(index!=3){
        char backupPath[MAX_FILENAME_LEN];
        memset(backupPath,0,MAX_FILENAME_LEN);
        int pathlen = strlen(path);
        memcpy(backupPath,path,pathlen);  
        memset(path,0,pathlen+1);
        memcpy(path,backupPath+mnt_point_len+1,pathlen-mnt_point_len-1);
    }
    return index;
    

} */

int get_index(char path[]){

  int pathlen = strlen(path);
    //char dev_name[DEV_NAME_LEN];
    char fs_name[DEV_NAME_LEN];   //modified by mingxuan 2020-10-18
    int len = (pathlen < DEV_NAME_LEN) ? pathlen : DEV_NAME_LEN;

    int i,a=0;
    for(i=0;i<len;i++){
        if( path[i] == '/'){
            a=i;
            a++;
            break;
        }
        else {
            //dev_name[i] = path[i];
            fs_name[i] = path[i];   //modified by mingxuan 2020-10-18
        }
    }
    //dev_name[i] = '\0';
    fs_name[i] = '\0';  //modified by mingxuan 2020-10-18
    for(i=0;i<pathlen-a;i++)
        path[i] = path[i+a];
    path[pathlen-a] = '\0';

    //for(i=0;i<NR_DEV;i++)
    for(i=0;i<NR_FS;i++)    //modified by mingxuan 2020-10-29
    {
        // if(!strcmp(dev_name, device_table[i].dev_name))
        if(!strcmp(fs_name, vfs_table[i].fs_name)) //modified by mingxuan 2020-10-18
            return i;
    }

    return -1;
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

PUBLIC int sys_create() {
    return do_vcreate(get_arg(1));
}

PUBLIC int sys_delete() {
    return do_vdelete(get_arg(1));
}

PUBLIC int sys_opendir() {
    return do_vopendir(get_arg(1));
}

PUBLIC int sys_createdir() {
    return do_vcreatedir(get_arg(1));
}

PUBLIC int sys_deletedir() {
    return do_vdeletedir(get_arg(1));
}

PUBLIC int sys_readdir() {
    return do_vreaddir(get_arg(1), get_arg(2), get_arg(3));
    //return ReadDir((PCHAR)get_arg(1), (PDWORD)get_arg(2), (PCHAR)get_arg(3));
}

//added by ran
PUBLIC int sys_chdir() {
  return do_vchdir((PCHAR)get_arg(1));
}

//added by ran
PUBLIC int sys_getcwd() {
  return (int)do_vgetcwd((PCHAR)get_arg(1), (PDWORD)get_arg(2));
}

//added by mingxuan 2021-8-15
PUBLIC int do_vopen(const char *path, int flags, int mode) {
	#ifdef NEW_VFS
	return kern_vfs_open(path, flags, mode);
	#endif
    return kern_vopen(path, flags);
}

/*======================================================================*
                              do_v* 系列函数
 *======================================================================*/
//PUBLIC int do_vopen(const char *path, int flags) {
PUBLIC int kern_vopen(const char *path, int flags) {    //modified by mingxuan 2021-8-15

    int pathlen = strlen(path);
    char pathname[MAX_PATH] = {0};
    strcpy(pathname, path);

    int index,i;
    int ret = vfs_path_transfer(pathname,&index);
    if(ret < 0){
        return -1;
    }
    int fd = -1;
    // index = get_index(pathname);
    // if(index == -1){
    //     disp_str("pathname error!\n");
    //     disp_str(path);
    //     return -1;
    // }

/*     index = 3;  //Orange
    struct vfs *pvfs = &vfs_table[index];
    if (pvfs->op->tag)
    {
        strcpy(pathname, path);
    } */
    fd = vfs_table[index].op->open(vfs_table[index].sb,pathname, flags);
    //若file中的dev_index 未被设置 则在此赋值
    //tty设备已经设置了dev_index ，常规文件dev_index 为-1
    //add by sundong 2023.5.18
    if(fd>=0 && p_proc_current -> task.filp[fd] -> dev_index == -1){
        p_proc_current -> task.filp[fd] -> dev_index = index;
    }
    // if (pvfs->sb->fs_type == FAT32_TYPE)
    // {
    //     fd = pvfs->op->OpenFile(pvfs->sb, pathname, flags);
    // }
    // else
    // {
    //     fd = vfs_table[index].op->open(pathname, flags);
    // }

/*     if(fd != -1)
    {
        index = get_index(pathnamebackup);
        if(0<=index&&index<=3)
        {
            p_proc_current -> task.filp[fd] -> dev_index = index;
        }
        //disp_str("          open file success!\n");   //deleted by mingxuan 2019-5-22
    }
    else {
        disp_str("vfs open: error!\n");
    } */

    return fd;
}

//added by mingxuan 2021-8-15
PUBLIC int do_vclose(int fd) {
	#ifdef NEW_VFS
	return kern_vfs_close(fd);
	#endif
    return kern_vclose(fd);
}

//PUBLIC int do_vclose(int fd) {
PUBLIC int kern_vclose(int fd) {    //modified by mingxuan 2021-8-15
    int index = p_proc_current->task.filp[fd]->dev_index;
    struct vfs* pvfs = &vfs_table[index];

    int result;
    result = pvfs->op->close(fd);
    // if (pvfs->sb->fs_type == FAT32_TYPE)
    // {
    //     result = pvfs->op->CloseFile(pvfs->sb, fd);
    // }
    // else
    // {
    //     result = pvfs->op->close(fd);
    // }

    return result;

    //return device_table[index].op->close(fd);
    //return vfs_table[index].op->close(fd);  //modified by mingxuan 2020-10-18
    // if (state == 1) {
    //     debug("          close file success!");
    // } else {
	// 	DisErrorInfo(state);
    // }
}

//added by mingxuan 2021-8-15
PUBLIC int do_vread(int fd, char *buf, int count) {
	#ifdef NEW_VFS
	return kern_vfs_read(fd, buf, count);
	#endif
    return kern_vread(fd, buf, count);
}

//PUBLIC int do_vread(int fd, char *buf, int count) {
PUBLIC int kern_vread(int fd, char *buf, int count) {   //modified by mingxuan 2021-8-15
    //disp_int(fd);
    int index = p_proc_current->task.filp[fd]->dev_index;
    struct vfs* pvfs = &vfs_table[index];

    int result;
    result = pvfs->op->read(fd, buf, count);
    // if (pvfs->sb->fs_type == FAT32_TYPE)
    // {
    //     result = pvfs->op->ReadFile(pvfs->sb, fd, buf, count);
    // }
    // else
    // {
    //     result = pvfs->op->read(fd, buf, count);
    // }

    return result;

    //disp_int(index);
    //return device_table[index].op->read(fd, buf, count);
    //return vfs_table[index].op->read(fd, buf, count);   //modified by mingxuan 2020-10-18
    // if (size >= 0) {
    //     debug("          read file success!");
    // } else {
	// 	DisErrorInfo(size);
    // }
    // return size;
}

//added by mingxuan 2021-8-15
PUBLIC int do_vwrite(int fd, const char *buf, int count) {
	#ifdef NEW_VFS
	return kern_vfs_write(fd, buf, count);
	#endif
    return kern_vwrite(fd, buf, count);
}

//PUBLIC int do_vwrite(int fd, const char *buf, int count) {
PUBLIC int kern_vwrite(int fd, const char *buf, int count) {    //modified by mingxuan 2021-8-15
    //char s[256];  //deleted by mingxuan 2019-5-19
    /*  //deleted by mingxuan 2019-5-23
    char s[FILE_MAX_LEN]; //modified by mingxuan 2019-5-19

    strcpy(s, buf);

    int index = p_proc_current->task.filp[fd]->dev_index;
    return device_table[index].op->write(fd,s,strlen(s));
    */

    //modified by mingxuan 2019-5-23
    char s[512];
    int index = p_proc_current->task.filp[fd]->dev_index;
    struct vfs* pvfs = &vfs_table[index];
    int fs_type = pvfs->sb->fs_type;
    char *fsbuf = buf;
    int f_len = count;
    int bytes;
    int success_bytes = 0; //表示成功写入的字节数   //added by mingxuan 2021-8-31
    /*  //deleted by mingxuan 2021-8-30
    while(f_len)
    {
        int iobytes = min(512, f_len);
        int i=0;
        for(i=0; i<iobytes; i++)
        {
            s[i] = *fsbuf;
            fsbuf++;
        }
        bytes = pvfs->op->write(fd, s, iobytes);
        // if (fs_type == FAT32_TYPE)
        // {
        //     bytes = pvfs->op->WriteFile(pvfs->sb, fd, s, iobytes);
        // }
        // else
        // {
        //     bytes = pvfs->op->write(fd, s, iobytes);
        // }

        //bytes = device_table[index].op->write(fd,s,iobytes);
        //bytes = vfs_table[index].op->write(fd,s,iobytes);   //modified by mingxuan 2020-10-18
        if(bytes != iobytes)
        {
            return bytes;
        }
        f_len -= bytes;
    }
    return count;
    */
    //modified by mingxuan 2021-8-30
    while(f_len)
    {
        int iobytes = min(512, f_len);  // iobytes是期望写入的字节数
        int i=0;
        for(i=0; i<iobytes; i++)
        {
            s[i] = *fsbuf;
            fsbuf++;
        }
        bytes = pvfs->op->write(fd, s, iobytes); //bytes是文件系统返回的实际写入的字节数

        /*  //deleted by mingxuan 2021-8-31
        if(bytes != iobytes)
        {
            return bytes;
        }
        */
       //added by mingxuan 2021-8-31
       if(bytes != iobytes || bytes == 0) //说明发生了写入异常
       {
           return success_bytes;    //返回已经成功写入的字节数
       }

        success_bytes += bytes;
        f_len -= bytes;
    }
    //return count;
    return success_bytes;   //modified by mingxuan 2021-8-31
}

//added by mingxuan 2021-8-15
PUBLIC int do_vunlink(const char *path) {
    return kern_vunlink(path);
}

//PUBLIC int do_vunlink(const char *path) {
PUBLIC int kern_vunlink(const char *path) {   //modified by mingxuan 2021-8-15
    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    //index = get_index(pathname);
    int ret = vfs_path_transfer(pathname,&index);
    if(ret < 0){
        disp_str("pathname error!\n");
        return -1;
    }
    //return device_table[index].op->unlink(pathname);
    return vfs_table[index].op->unlink(vfs_table[index].sb,pathname);   //modified by mingxuan 2020-10-18
}

//added by mingxuan 2021-8-15
PUBLIC int do_vlseek(int fd, int offset, int whence) {
	#ifdef NEW_VFS
	return kern_vfs_lseek(fd, offset, whence);
	#endif
    return kern_vlseek(fd, offset, whence);
}

//PUBLIC int do_vlseek(int fd, int offset, int whence) {
PUBLIC int kern_vlseek(int fd, int offset, int whence) {    //modified by mingxuan 2021-8-15
    int index = p_proc_current->task.filp[fd]->dev_index;
    struct vfs *pvfs = &vfs_table[index];

    int result;
    result = pvfs->op->lseek(fd, offset, whence);
    // if (pvfs->sb->fs_type == FAT32_TYPE)
    // {
    //     result = pvfs->op->LSeek(fd, offset, whence);
    // }
    // else
    // {
    //     result = pvfs->op->lseek(fd, offset, whence);
    // }

    return result;

    //return device_table[index].op->lseek(fd, offset, whence);
    //return vfs_table[index].op->lseek(fd, offset, whence);  //modified by mingxuan 2020-10-18

}

//added by mingxuan 2021-8-15
PUBLIC int do_vcreate(char *filepath)
{
    return kern_vcreate(filepath);
}

//PUBLIC int do_vcreate(char *pathname) {
//PUBLIC int do_vcreate(char *filepath) { //modified by mingxuan 2019-5-17
PUBLIC int kern_vcreate(char *filepath) { //modified by mingxuan 2019-5-17
    // disp_str("hhh");
    // const char *path = get_arg(1);

    // int pathlen = strlen(path);
    // char pathname[MAX_PATH];

    // strcpy(pathname,path);
    // pathname[pathlen] = '\0';

    // int index;
    // index = get_index(pathname);
    // disp_int(index);
    // if(index==-1){
    //     disp_str("          pathname error!\n");
    //     return -1;
    // }


    // int state = 0;

    // //int state = device_table[index].op -> create(pathname);
    // if (state == 1) {
    //     debug("          create file success!");
    // } else {
	// 	DisErrorInfo(state);
    // }
    // return state;

    //added by mingxuan 2019-5-17
    int state;
    const char *path = filepath;

    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    //index = (int)(pathname[1]-'0');   //deleted by mingxuan 2019-5-17
    //disp_int(index);  //deleted by mingxuan 2019-5-17
    //index = get_index(pathname);
    int ret = vfs_path_transfer(pathname,&index);
    if(ret < 0){
        disp_str("pathname error!\n");
        disp_str(path);
        return -1;
    }
    struct vfs *pvfs = &vfs_table[index];

/*     if (pvfs->op->tag)
    {
        strcpy(pathname, path);
    } */
    state = pvfs->op->create(pvfs->sb,pathname);

    // if (pvfs->sb->fs_type == FAT32_TYPE)
    // {
    //     state = pvfs->op->CreateFile(pvfs->sb, pathname);
    // }
    // else
    // {
    //     state = pvfs->op->create(pathname);
    // }

    /*
    for(int j=0;j<= pathlen-3;j++)
    {
        pathname[j] = pathname[j+3];
    }
    */
    //state = f_op_table[index].create(pathname);
    //state = device_table[index].op->create(pathname);
    //state = vfs_table[index].op->create(pathname); //modified by mingxuan 2020-10-18
//    if (state == 1) {
//        debug("          create file success!");
//    } else {
//		DisErrorInfo(state);
//    }
    return state;
}

//added by mingxuan 2021-8-15
PUBLIC int do_vdelete(char *path)
{
    return kern_vdelete(path);
}

//PUBLIC int do_vdelete(char *path) {
PUBLIC int kern_vdelete(char *path) {   //modified by mingxuan 2021-8-15

    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    //index = get_index(pathname);
    int ret = vfs_path_transfer(pathname,&index);
    if(ret < 0){
        disp_str("pathname error!\n");
        disp_str(path);
        return -1;
    }
    struct vfs *pvfs = &vfs_table[index];

    int result;

/*     if (pvfs->op->tag)
    {
        strcpy(pathname, path);
    } */
    result = pvfs->op->delete(pvfs->sb,pathname);
    // if (pvfs->sb->fs_type == FAT32_TYPE)
    // {
    //     result = pvfs->op->DeleteFile(pvfs->sb, pathname);
    // }
    // else
    // {
    //     result = pvfs->op->delete(pathname);
    // }

    return result;

    //return device_table[index].op->delete(pathname);
    //return vfs_table[index].op->delete(pathname);   //modified by mingxuan 2020-10-18
    // state = f_op_table[index].delete(pathname);
    // if (state == 1) {
    //     debug("          delete file success!");
    // } else {
	// 	DisErrorInfo(state);
    // }
}

//added by mingxuan 2021-8-15
PUBLIC int do_vopendir(char *path) {
    return kern_vopendir(path);
}

//PUBLIC int do_vopendir(char *path) {
PUBLIC int kern_vopendir(char *path) {  //modified by mingxuan 2021-8-15
    int state;

    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;

    //index = get_index(pathname);
    int ret = vfs_path_transfer(pathname,&index);
    if(ret < 0){
        return -1;
    }
    state = vfs_table[index].op->opendir(vfs_table[index].sb,pathname);

    return state;
}

//added by mingxuan 2021-8-15
PUBLIC int do_vcreatedir(char *path) {
	#ifdef NEW_VFS
	return kern_vfs_mkdir(path, I_RWX);
	#endif
    return kern_vcreatedir(path);
}

//PUBLIC int do_vcreatedir(char *path) {
PUBLIC int kern_vcreatedir(char *path) {  //modified by mingxuan 2021-8-15
    int state;

    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    //index = (int)(pathname[1]-'0');
    //int fs_len = get_fs_len(path) + 1;
    //index = get_index(pathname);
    int ret = vfs_path_transfer(pathname,&index);
    if(ret < 0){
        disp_str("pathname error!\n");
        disp_str(path);
        return -1;
    }
    struct vfs *pvfs = &vfs_table[index];

/*     if (pvfs->op->tag)
    {
        strcpy(pathname, path);
    } */
    state = pvfs->op->createdir(pvfs->sb,pathname);

    // if (pvfs->sb->fs_type == FAT32_TYPE)
    // {
    //     state = pvfs->op->CreateDir(pvfs->sb, pathname);
    // }
    // else
    // {
    //     state = pvfs->op->createdir(pathname);
    // }

    //state = f_op_table[index].createdir(pathname);
    //state = vfs_table[index].op->createdir(pathname);
//    if (state == 1) {
//        debug("          create dir success!");
//    } else {
//		DisErrorInfo(state);
//    }
    return state;
}

//added by mingxuan 2021-8-15
PUBLIC int do_vdeletedir(char *path) {
    return kern_vdeletedir(path);
}

//PUBLIC int do_vdeletedir(char *path) {
PUBLIC int kern_vdeletedir(char *path) {    //modified by mingxuan 2021-8-15
    int state;
    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    //index = (int)(pathname[1]-'0');
    //index = get_index(pathname);
    int ret = vfs_path_transfer(pathname,&index);
    if(ret < 0){
        return -1;
    }
    struct vfs *pvfs = &vfs_table[index];

/*     if (pvfs->op->tag)
    {
        strcpy(pathname, path);
    } */
    state = pvfs->op->deletedir(pvfs->sb,pathname);

    // if (pvfs->sb->fs_type == FAT32_TYPE)
    // {
    //     state = pvfs->op->DeleteDir(pvfs->sb, pathname);
    // }
    // else
    // {
    //     state = pvfs->op->deletedir(pathname);
    // }

    //state = vfs_table[index].op->deletedir(pathname);
//    if (state == 1) {
//        debug("          delete dir success!");
//    } else {
//		DisErrorInfo(state);
//    }
    return state;
}


//added by mingxuan 2021-8-15
PUBLIC int do_vchdir(const char *path) {
    return kern_vchdir(path);
}

//PUBLIC int do_vchdir(const char *path) {
PUBLIC int kern_vchdir(const char *path) {  //modified by mingxuan 2021-8-15
    char pathname[MAX_PATH];
    strcpy(pathname, path);
    //int index = get_index(pathname);
    //strcpy(pathname, path);
    int index;
    int ret = vfs_path_transfer(pathname,&index);
    if(ret < 0){
        return -1;
    }
    struct vfs *pvfs = &vfs_table[index];
    int state = pvfs->op->chdir(pvfs->sb,pathname);
    return state;
}


//PUBLIC char* do_vgetcwd(char *buf, int size) {
PUBLIC char* kern_vgetcwd(char *buf, int size) {    //modified by mingxuan 2021-8-15
    if (!buf)
    {
        return 0;
    }
    strncpy(buf, p_proc_current->task.cwd, size);
    return buf;
}

//added by mingxuan 2021-8-15
PUBLIC int do_vgetcwd(char *buf, int size) {
    return (int)kern_vgetcwd(buf, size);
}


//added by mingxuan 2021-8-15
PUBLIC int do_vreaddir(PCHAR dirname, DWORD dir[3], PCHAR filename)
{
    return kern_vreaddir(dirname, dir, filename);
}

//PUBLIC int do_vreaddir(PCHAR dirname, DWORD dir[3], PCHAR filename)
PUBLIC int kern_vreaddir(PCHAR dirname, DWORD dir[3], PCHAR filename) //modified by mingxuan 2021-8-15
{
    char pathname[MAX_PATH];
    strcpy(pathname, dirname);
    //int index = get_index(pathname);
    int index;
    int ret = vfs_path_transfer(pathname,&index);
    if(ret < 0){
        return -1;
    }
    struct vfs *pvfs = &vfs_table[index];
/*     if (pvfs->op->tag)
    {
        strcpy(pathname, dirname);
    } */
    pvfs->op->readdir(pvfs->sb,pathname, dir, filename);
}