#include <minios/vfs.h>
#include <minios/config.h>
#include <minios/buffer.h>
#include <minios/hd.h>
#include <minios/clock.h>
#include <minios/memman.h>
#include <minios/mount.h>
#include <minios/stat.h>
#include <minios/console.h>
#include <minios/tty.h>
#include <minios/assert.h>
#include <fs/devfs/devfs.h>
#include <fs/orangefs/orangefs.h>
#include <fs/fat32/fat32.h>
#include <fs/ext4/ext4.h>
#include <klib/stddef.h>
#include <compiler.h>
#include <list.h>
#include <string.h>
#include <stdbool.h>

// PUBLIC struct file_desc f_desc_table[NR_FILE_DESC];
struct dentry* vfs_root;

struct inode* vfs_new_inode(struct super_block* sb);
struct inode* vfs_get_inode(struct super_block* sb, u32 ino);
struct dentry* vfs_new_dentry(const char* name, struct inode* inode);

// 此锁维护inode内存申请释放以及超级块(super_block)中使用中inode链表的一致性
static spinlock_t inode_lock;

static spinlock_t file_desc_lock;
static spinlock_t superblock_lock;

static void init_file_desc_table() {
    // int i;
    // for (i = 0; i < NR_FILE_DESC; i++)
    // 	memset(&f_desc_table[i], 0, sizeof(struct file_desc));
}

static struct file_desc* alloc_file() {
    struct file_desc* file = NULL;
    // spinlock_lock_or_yield(&file_desc_lock);
    file = kern_kmalloc(sizeof(struct file_desc));
    atomic_set(&file->fd_count, 1);
    // spinlock_release(&file_desc_lock);
    return file;
}

static void free_file(struct file_desc* file) {
    // spinlock_lock_or_yield(&file_desc_lock); // file动态分配不再需要锁
    vfs_put_dentry(file->fd_dentry);
    file->fd_dentry = 0;
    kern_kfree(file);
    // spinlock_release(&file_desc_lock);
}

static struct inode* alloc_inode_locked(struct super_block* sb) {
    //! ATTENTION: the locked method is required to be invoked only when
    //! inode_lock is held
    assert(inode_lock.locked);

    const int inode_impl_size =
        sizeof(struct inode) + fstype_table[sb->fs_type].fs_size_info.inode_size;
    struct inode* inode = kern_kmalloc(inode_impl_size);
    memset(inode, 0, inode_impl_size);

    inode->i_nlink = 1;
    atomic_set(&inode->i_count, 1);
    inode->i_sb = sb;
    list_init(&inode->i_list);
    list_init(&inode->i_dentry);
    inode->i_mapping = &inode->i_data;
    inode->i_mapping->host = inode;
    init_mem_page(inode->i_mapping, MEMPAGE_CACHED);

    return inode;
}

// VFS api
// alloc a new inode for superblock
// 调用此函数分配新的inode
struct inode* vfs_new_inode(struct super_block* sb) {
    struct inode* res = NULL;
    spinlock_lock_or_yield(&inode_lock);
    res = alloc_inode_locked(sb);
    list_add_first(&res->i_list, &sb->sb_inode_list);
    spinlock_release(&inode_lock);
    return res;
}

static void free_inode(struct inode* inode) {
    spinlock_lock_or_yield(&inode_lock);
    list_remove(&inode->i_list);
    kern_kfree(inode);
    spinlock_release(&inode_lock);
}

// VFS api
// get an inode by ino, if not exist in mem, alloc one and read from disk
// 调用此函数获取inode
struct inode* vfs_get_inode(struct super_block* sb, u32 ino) {
    struct inode *res = 0, *inode;
    spinlock_lock_or_yield(&inode_lock);
    list_for_each(&sb->sb_inode_list, inode, i_list) {
        if (inode->i_no == ino) {
            list_remove(&inode->i_list);
            res = inode;
            break;
        }
    }
    if (res) {
        atomic_inc(&res->i_count);
    } else {
        res = alloc_inode_locked(sb);
        res->i_no = ino;
        sb->sb_op->read_inode(res);
    }
    list_add_first(&res->i_list, &sb->sb_inode_list);
    spinlock_release(&inode_lock);
    return res;
}

void vfs_sync_inode(struct inode* inode) {
    struct superblock_operations* ops = inode->i_sb->sb_op;
    if (ops && ops->sync_inode) { ops->sync_inode(inode); }
    sync_buffers(0);
}

// 如果引用计数归0，释放资源,
// 调用此函数必须持有inode锁,除非inode还没有加入目录树
// 如果引用计数归0，且引用的文件数也为0，执行删除文件
// put inode之后自动解锁
void vfs_put_inode(struct inode* inode) {
    if (atomic_dec_and_test(&inode->i_count)) {
        memory_page_t* _page = NULL;
        UNUSED(_page);
        struct superblock_operations* ops = inode->i_sb->sb_op;
        if (inode->i_nlink == 0) {
            if (ops && ops->delete_inode) { ops->delete_inode(inode); }
        }
        if (ops && ops->put_inode) { ops->put_inode(inode); }
        spinlock_lock_or_yield(&inode->i_mapping->lock);
        free_mem_pages(inode->i_mapping);
        spinlock_release(&inode->i_mapping->lock);
        vfs_sync_inode(inode);
        free_inode(inode);
        return;
    }
    spinlock_release(&inode->lock);
}

// 在dir 文件夹下加入dentry
// require mutex: dir->lock, dentry->lock
static void insert_sub_dentry(struct dentry* dir, struct dentry* dentry) {
    list_add_first(&dentry->d_list, &dir->d_subdirs);
    dentry->d_parent = dir;
    if (!dentry->d_vfsmount) dentry->d_vfsmount = dir->d_vfsmount;
}

// require mutex: dir, ent
static void remove_sub_dentry(struct dentry* dir, struct dentry* dentry) {
    UNUSED(dir);
    list_remove(&dentry->d_list);
    // ent->d_parent = NULL;
    // parent saved,因为存在此dentry已被删除但仍有引用的情况，比如rmdir ../cwd,
    // cd
    // ..
}

// 0 for empty, else -1
static int check_dir_entry_empty(struct dentry* dir) {
    struct dentry* dentry = NULL;
    list_for_each(&dir->d_subdirs, dentry, d_list) {
        if ((!(strcmp(dentry_name(dentry), "."))) || (!(strcmp(dentry_name(dentry), "..")))) {
            continue;
        }
        return -1;
    }
    return 0;
}

static struct dentry* alloc_dentry(const char* name) {
    struct dentry* dentry;
    dentry = kern_kmalloc(sizeof(struct dentry));
    memset(dentry, 0, sizeof(struct dentry));
    spinlock_init(&dentry->lock, NULL);
    int len = strlen(name);
    if (len < MAX_DNAME_LEN) {
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
    list_init(&dentry->d_alias);
    list_init(&dentry->d_subdirs);
    return dentry;
}

static void free_dentry(struct dentry* dentry) {
    if (dentry->d_longname) { kern_kfree(dentry->d_longname); }
    kern_kfree(dentry);
}

// VFS api
// alloc a new dentry and connect to inode
// require mutex: dir, inode
struct dentry* vfs_new_dentry(const char* name, struct inode* inode) {
    struct dentry* dentry;
    dentry = alloc_dentry(name);
    dentry->d_inode = inode;
    list_add_last(&dentry->d_alias, &inode->i_dentry);
    return dentry;
}

// locked: dentry
void vfs_put_dentry(struct dentry* dentry) {
    if (atomic_dec_and_test(&dentry->d_count)) {
        // 如果其中的inode已经无效，说明是已经unlink/rmdir的dentry
        if (dentry->d_inode && dentry->d_inode->i_nlink == 0) {
            spinlock_lock_or_yield(&dentry->d_inode->lock);
            vfs_put_inode(dentry->d_inode);
            dentry->d_inode = NULL;
        }
        if (!dentry->d_inode) {
            free_dentry(dentry);
            return;
        }
        vfs_sync_inode(dentry->d_inode);
    }
    spinlock_release(&dentry->lock);
}

static void vfs_get_dentry(struct dentry* dentry) {
    spinlock_lock_or_yield(&dentry->lock);
    atomic_inc(&dentry->d_count);
}

// delete dentry in dir
// require mutex: dir, dentry
static int delete_dentry(struct dentry* dentry, struct dentry* dir) {
    if ((!list_empty(&dentry->d_subdirs)) || dentry->d_mounted) {
        // check empty / mountpoint
        kprintf("error: try to delete non empty or mountpoint dentry");
        return -1;
    }
    remove_sub_dentry(dir, dentry);
    return 0;
}

/*!
 * \return whether truncated or not
 */
static bool vfs_get_path(struct dentry* dir, char* buf, int size) {
    if (!buf) { return false; }
    if (dir == vfs_root) {
        strncpy(buf, "/", size);
        return size <= 1;
    }
    char path[MAX_PATH];
    char* p = path + MAX_PATH;
    *(--p) = 0;
    int len;
    while (dir && dir != vfs_root) {
        while (dir != vfs_root && dir->d_vfsmount && dir->d_vfsmount->mnt_root == dir) {
            dir = dir->d_vfsmount->mnt_mountpoint;
        }
        len = strlen(dentry_name(dir));
        p -= len;
        strncpy(p, dentry_name(dir), len);
        *(--p) = '/';
        dir = dir->d_parent;
    }
    strncpy(buf, p, size);
    return size <= strlen(p);
}

static void register_fs_type(char* fstype_name, int fs_type, struct file_operations* f_op,
                             struct inode_operations* i_op, struct dentry_operations* d_op,
                             struct superblock_operations* s_op, int (*identify)(int, u32)) {
    auto fs = &fstype_table[fs_type];
    fs->fstype_name = fstype_name;
    assert(s_op && s_op->query_size_info);
    s_op->query_size_info(&fs->fs_size_info);
    fs->fs_fop = f_op;
    fs->fs_iop = i_op;
    fs->fs_dop = d_op;
    fs->fs_sop = s_op;
    fs->identify = identify;
}

// 文件名拆分：同时删除末尾多余的"/"
static const char* strip_dir_path(const char* path, char* dir) {
    int len = strlen(path), flag = 0;
    UNUSED(flag);
    const char *p = path + len - 1, *file_name = path, *file_end = path + len - 1;
    UNUSED(file_end);
    while (p != path && (*p) == '/') p--;
    file_end = p;
    // file_end[1] = '\0';
    while (p >= path && (*p)) {
        if (*(p) == '/') {
            file_name = p + 1;
            break;
        }
        p--;
    }
    if (file_name != path) {
        strncpy(dir, path, file_name - path);
        dir[file_name - path] = 0;
    }
    return file_name;
}

// 上锁规则:
// 上层->下层目录:持有上层锁获取下层锁,已经持有下层锁的不允许获取上层锁, dir
// lock must held, return dentry with lock 参数dir
// dentry带锁,返回有效dentry也带锁
// 如果dir和dentry目录项的关系满足dentry在dir之下，且spinlock_release_base为1,则此函数释放dir的目录项
static struct dentry* _do_lookup(struct dentry* dir, const char* name, int spinlock_release_base) {
    // spinlock_lock_or_yield(&dir->lock);
    struct dentry* dentry = NULL;
    // root 特判
    if (dir == vfs_root && (!name[0])) {
        dentry = dir;
    } else if (!strcmp(name, ".")) {
        dentry = dir;
    } else if (!strcmp(name, "..")) {
        if (dir == vfs_root) {
            dentry = vfs_root;
        } else {
            while (dir != vfs_root && dir->d_vfsmount && dir->d_vfsmount->mnt_root == dir) {
                struct dentry* mnt_dir = dir->d_vfsmount->mnt_mountpoint;
                if (mnt_dir) {
                    vfs_get_dentry(mnt_dir);
                } else {
                    kprintf("error: invalid mountpoint\n");
                }
                vfs_put_dentry(dir);
                dir = mnt_dir;
                // dir = dir->d_vfsmount->mnt_mountpoint;
            }
            dentry = dir->d_parent; // 对于挂载点下的dentry，上层是挂载前的
            // 对于..的路径，由于严格要求目录项的锁持有顺序满足：
            // 目录项到根目录项的距离递增，
            // 所以访问上层目录项之前释放父目录项的锁，此情况dentry在dir上层，
            // 故spinlock_release_base参数不起作用
            if (dentry != dir) {
                vfs_put_dentry(dir);
                vfs_get_dentry(dentry);
            }
            return dentry;
        }
    }

    typedef int (*fn_dirent_name_cmp)(const char*, const char*);
    fn_dirent_name_cmp dirent_name_cmp =
        dir->d_op && dir->d_op->compare ? dir->d_op->compare : strcmp;
    if (!dentry) {
        // dentry = dir->d_subdirs;
        // int new_dentry = 0;
        // for( ;dentry; dentry = dentry->d_nxt){
        int found = 0;
        if (!list_empty(&dir->d_subdirs)) {
            list_for_each(&dir->d_subdirs, dentry, d_list) {
                if (!dirent_name_cmp(dentry_name(dentry), name)) {
                    found = 1;
                    break;
                }
            }
        }
        if (found == 0) { dentry = NULL; }
    }
    if (dentry) { // dentry 命中
        if (dentry != dir) { vfs_get_dentry(dentry); }
    } else { // real lookup
        spinlock_lock_or_yield(&dir->d_inode->lock);
        struct inode_operations* i_ops = NULL;
        int type = dir->d_inode->i_type;
        if (type == I_DIRECTORY) { i_ops = dir->d_inode->i_op; }
        if (i_ops) {
            dentry = i_ops->lookup(dir->d_inode, name);
            if (dentry) {
                spinlock_lock_or_yield(&dentry->lock);
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
        spinlock_release(&dir->d_inode->lock);
    }
    while (dentry && (dentry->d_mounted)) {
        // todo: follow mount
        struct vfs_mount* mnt = lookup_vfsmnt(dentry);
        vfs_get_dentry(mnt->mnt_root);
        vfs_put_dentry(dentry);
        dentry = mnt->mnt_root;
    }
    if (spinlock_release_base) {
        if (dentry != dir) { vfs_put_dentry(dir); }
    }
    return dentry;
}

// 输入：根dentry base
struct dentry* vfs_lookup(const char* path) {
    if (!path) { return NULL; }
    char d_name[MAX_PATH];
    const char* p_name = NULL;
    const char* p = path;
    int flag_name = 0;
    UNUSED(flag_name);
    struct dentry* dir = vfs_root;
    if (path[0] != '/') { dir = p_proc_current->task.cwd; }
    vfs_get_dentry(dir);
    char c = '\0';
    while (1) {
        // 注意：此处是读入一个字符到c, c = *p 是赋值不是比较，下同
        while ((c = *p) && c == '/') p++; // 找到这一级名字的开头
        if (c == '\0') {
            break; // 查找到末尾
        }
        p_name = p;
        while ((c = *p) && c != '/') p++; // 找到这一级名字的末尾
        strncpy(d_name, p_name, p - p_name);
        d_name[p - p_name] = '\0';
        dir = _do_lookup(dir, d_name, 1);
        if (dir == NULL) {
            return NULL; // 中途任意一级不存在，则查找失败
        }
    }
    return dir;
}

static void init_special_inode(struct inode* inode, int type, int mode, int dev) {
    spinlock_lock_or_yield(&inode->lock);
    inode->i_mode = mode;
    inode->i_type = type;
    inode->i_b_cdev = dev;
    switch (type) {
        case I_CHAR_SPECIAL:
            inode->i_size = 0;
            inode->i_fop = &tty_file_ops;
            break;
        case I_BLOCK_SPECIAL:
            inode->i_size = hd_infos[MAJOR(dev) - DEV_HD_BASE].part[MINOR(dev)].size
                            << SECTOR_SIZE_SHIFT;
            inode->i_fop = &blk_file_ops;
        default:
            break;
    }
    spinlock_release(&inode->lock);
}

static struct dentry* vfs_create(struct inode* dir, const char* file_name, int type, int mode,
                                 int dev) {
    struct inode_operations* i_ops = NULL;
    struct dentry* dentry = NULL;
    int state = -1;
    spinlock_lock_or_yield(&dir->lock);
    if (dir->i_type == I_DIRECTORY) { i_ops = dir->i_op; }
    if (i_ops) {
        dentry = alloc_dentry(file_name);
        state = -1; // default err
        // negative dentry, create call will get inode
        if (type == I_DIRECTORY) {
            if (i_ops->mkdir) { state = i_ops->mkdir(dir, dentry, mode); }
        } else if (i_ops->create) { // regular file
            state = i_ops->create(dir, dentry, mode);
        }
        if (state != 0) { // error
            vfs_put_dentry(dentry);
            spinlock_release(&dir->lock);
            return NULL;
        }
        if (type == I_BLOCK_SPECIAL || type == I_CHAR_SPECIAL) {
            init_special_inode(dentry->d_inode, type, mode, dev);
        }
    }
    spinlock_release(&dir->lock);
    return dentry;
}

static struct super_block* vfs_read_super(int dev, u32 fstype) {
    if (dev) {
        // 对于 fstype FS_TYPE_NONE, 可以认为其为 自动 ，即跟随hd_infos中的FSTYPE
        if (fstype == FS_TYPE_NONE) {
            fstype = get_hd_fstype(dev);
        } else if (get_hd_fstype(dev) != fstype) {
            // dismatch fstype
            kprintf("fail: fstype not match");
            return NULL;
        }
        //! TODO: check hd busy
    }

    struct fs_type* file_sys = &fstype_table[fstype];
    if (!(file_sys->fs_sop && file_sys->fs_sop->fill_superblock)) {
        kprintf("error: fstype undefined fill_superblock\n");
        return NULL;
    }

    spinlock_lock_or_yield(&superblock_lock);

    int sb_index = get_free_superblock();
    if (sb_index == -1) {
        spinlock_release(&superblock_lock);
        kprintf("error: no available superblock\n");
        return NULL;
    }

    const int sb_private_size = file_sys->fs_size_info.sb_size;
    assert(sb_private_size >= 0);
    const int sb_impl_size = sizeof(struct super_block) + sb_private_size;
    struct super_block* sb = kern_kmalloc(sb_impl_size);
    memset(sb, 0, sb_impl_size);

    list_init(&sb->sb_inode_list);
    int state = file_sys->fs_sop->fill_superblock(sb, dev);
    if (state != 0) {
        kern_kfree(sb);
        super_blocks[sb_index] = NULL;
        spinlock_release(&superblock_lock);
        kprintf("error: fs fill_superblock failed\n");
        return NULL;
    }
    sb->used = true;

    super_blocks[sb_index] = sb;

    spinlock_release(&superblock_lock);
    return sb;
}

static int vfs_clear_super(struct super_block* sb) {
    struct inode* inode = NULL;
    struct dentry* dentry = NULL;
    int check_busy = 0;

    // check and clean all mounted inode
    spinlock_lock_or_yield(&inode_lock);
    list_head dentry_list;
    list_init(&dentry_list);
    list_for_each(&sb->sb_inode_list, inode, i_list) {
        while (!list_empty(&inode->i_dentry)) {
            dentry = list_front(&inode->i_dentry, struct dentry, d_alias);
            if ((atomic_get(&dentry->d_count) > 0) || dentry->d_mounted) {
                check_busy = 1;
                goto out_inode;
            } else {
                list_move(&dentry->d_alias, &dentry_list);
            }
        }
    }
out_inode:
    spinlock_release(&inode_lock);
    if (check_busy) {
        kprintf("error: superblock busy\n");
        return -1;
    }
    while ((dentry = list_front(&dentry_list, struct dentry, d_alias))) {
        spinlock_lock_or_yield(&dentry->d_inode->lock);
        vfs_put_inode(dentry->d_inode);
        list_remove(&dentry->d_alias);
        free_dentry(dentry);
    }
    return 0;
}

static struct super_block* vfs_get_super(int dev, u32 fstype) {
    for (int i = 0; i < NR_SUPER_BLOCK; ++i) {
        if (super_blocks[i] == NULL) { continue; }
        const auto sb = super_blocks[i];
        if (sb->used && sb->sb_dev == dev) { return sb; }
    }
    return vfs_read_super(dev, fstype);
}

// 挂载块设备核心实现
// 返回文件系统分区实例的根目录项
static struct dentry* vfs_mount_dev(int dev, u32 fstype, const char* dev_name, struct dentry* mnt) {
    struct super_block* sb = vfs_get_super(dev, fstype);
    if (!sb) { return NULL; }
    if (sb->sb_vfsmount) {
        kprintf("error: device already mounted\n");
        return NULL;
    }
    sb->sb_root->d_vfsmount = add_vfsmount(dev_name, mnt, sb->sb_root, sb);
    sb->sb_vfsmount = sb->sb_root->d_vfsmount;
    if (mnt) { mnt->d_mounted = true; }
    return sb->sb_root;
}

static void mount_root(int root_drive) {
    int root_fstype = get_fstype_by_name(ROOT_FSTYPE);
    int dev = -1;
#ifdef ROOT_PART
    dev = get_hd_part_dev(root_drive, ROOT_PART, root_fstype);
#else
    dev = get_hd_dev(root_drive,
                     root_fstype); // 自动匹配符合文件系统类型的第一个分区
#endif
    // struct super_block *sb = vfs_get_super(dev, root_fstype);
    vfs_root = vfs_mount_dev(dev, root_fstype, "/", NULL);
}

int init_block_dev();
int init_char_dev();
int kern_vfs_mkdir(const char* path, int mode);
static void init_fsroot() {
    kern_vfs_mkdir("/dev", I_RWX);
    struct dentry* dev_dir = _do_lookup(vfs_root, "dev", 0);
    vfs_mount_dev(NO_DEV, FS_TYPE_DEV, "dev", dev_dir);
    vfs_put_dentry(dev_dir);
    // 加入devfs之后，设备文件的初始化实际上只进行设备的注册
    init_block_dev();
    init_char_dev();
}

void register_fs_types() {
    register_fs_type("devfs", FS_TYPE_DEV, &devfs_file_ops, &devfs_inode_ops, NULL, &devfs_sb_ops,
                     NULL);
    register_fs_type("orangefs", FS_TYPE_ORANGE, &orange_file_ops, &orange_inode_ops, NULL,
                     &orange_sb_ops, orangefs_identify);
    register_fs_type("fat32", FS_TYPE_FAT32, &fat32_file_ops, &fat32_inode_ops, &fat32_dentry_ops,
                     &fat32_sb_ops, fat32_identify);
    register_fs_type("ext4", FS_TYPE_EXT4, &ext4_file_ops, &ext4_inode_ops, &ext4_dentry_ops,
                     &ext4_sb_ops, ext4_identify);
}

void init_fs(int drive) {
    spinlock_init(&inode_lock, "");
    spinlock_init(&file_desc_lock, "");
    init_file_desc_table();
    mount_root(drive);
    init_fsroot();
}

int get_fstype_by_name(const char* fstype_name) {
    int fstype = FS_TYPE_NONE;
    for (; fstype < NR_FS_TYPE; ++fstype) {
        if (!strcmp(fstype_name, fstype_table[fstype].fstype_name)) { break; }
    }
    if (fstype == NR_FS_TYPE) {
        // unknown fs type
        return FS_TYPE_NONE;
    }
    return fstype;
}

int kern_vfs_mount(const char* source, const char* target, const char* filesystemtype,
                   int mountflags, const void* data) {
    UNUSED(mountflags);
    UNUSED(data);
    // modified by sundong 2023.5.28
    // int device = get_dev_from_name(source);
    // 从根文件系统中获取块设备的设备号
    struct dentry* block_dev = vfs_lookup(source);
    if (!block_dev) { return -1; }
    spinlock_lock_or_yield(&block_dev->d_inode->lock);
    int dev = block_dev->d_inode->i_b_cdev;
    if (block_dev->d_inode->i_type != I_BLOCK_SPECIAL) {
        // 不是块设备，挂载失败
        dev = -1;
    }
    spinlock_release(&block_dev->d_inode->lock);
    vfs_put_dentry(block_dev);
    if (dev == -1) { return -1; }
    struct dentry* dir_d = vfs_lookup(target);
    int retval = -1;
    do {
        if (!dir_d) {
            kprintf("mountpoint dir not exist\n");
            return -1;
        }
        if (dir_d->d_inode->i_type != I_DIRECTORY) {
            kprintf("mountpoint is not a dir\n");
            break;
        }
        if (dir_d->d_mounted) {
            kprintf("mountpoint has already be mountted\n");
            break;
        }
        int fstype = get_fstype_by_name(filesystemtype);
        char dev_name[MNT_DEVNAME_LEN] = {};
        vfs_get_path(block_dev, dev_name, MNT_DEVNAME_LEN);
        struct dentry* mnt_root = vfs_mount_dev(dev, fstype, dev_name, dir_d);
        if (!mnt_root) { break; }
        dir_d->d_mounted = true;
        retval = 0;
    } while (0);
    vfs_put_dentry(dir_d);
    return retval;
}

int kern_vfs_umount(const char* target) {
    struct dentry* mnt_root = vfs_lookup(target);
    if (!mnt_root) { return -1; }
    assert(!mnt_root->d_mounted && mnt_root->d_vfsmount != NULL);
    struct super_block* sb = mnt_root->d_inode->i_sb;
    vfs_put_dentry(mnt_root);

    //! ATTENTION: fs impl is expected to alloc dentry "/" as root on mount registry, so "/" i.e.
    //! mnt_root will always has at least one ref, put once again here to kill the initial ref or
    //! else the sb will be reported busy and reject the umount request
    vfs_put_dentry(mnt_root);

    spinlock_lock_or_yield(&superblock_lock);
    int state = vfs_clear_super(sb);
    if (state != 0) {
        // 如果存在正在使用的目录项，或者存在嵌套的下级挂载点
        // 则报 busy 不予卸载
        spinlock_release(&superblock_lock);
        return -1;
    }
    const bool sb_released = release_superblock(sb);
    assert(sb_released);
    spinlock_release(&superblock_lock);

    struct dentry* mount_point = remove_vfsmnt(mnt_root);
    if (!mount_point) { return -1; }
    spinlock_lock_or_yield(&superblock_lock);
    sb->used = false;
    spinlock_release(&superblock_lock);
    mount_point->d_mounted = false;
    vfs_put_dentry(mount_point);
    return 0;
}

static struct file_desc* vfs_file_open(const char* path, int flags, int mode) {
    if (path == NULL) { return NULL; }
    const size_t sz_path = strlen(path);
    if (sz_path == 0 || sz_path + 1 > MAX_PATH) { return NULL; }

    char dir_path[MAX_PATH] = {0};
    const char* file_name = strip_dir_path(path, dir_path);
    struct dentry *dir = vfs_lookup(dir_path), *dentry = NULL;
    struct inode* inode = NULL;
    struct file_desc* file = NULL;
    if (!dir) { return NULL; }

    dentry = _do_lookup(dir, file_name, 0);
    if (!dentry && (flags & O_CREAT)) {
        // create regular file
        dentry = vfs_create(dir->d_inode, file_name, I_REGULAR, mode & (~I_TYPE_MASK), 0);
        if (!dentry) goto no_dentry_out;
        spinlock_lock_or_yield(&dentry->lock);
        insert_sub_dentry(dir, dentry);
    }
    if (!dentry) { goto no_dentry_out; }

    inode = dentry->d_inode;
    spinlock_lock_or_yield(&inode->lock);

    if ((flags & O_DIRECTORY) && inode->i_type != I_DIRECTORY) {
        kprintf("error: not a dir");
        goto err;
    }
    int rmode = (flags + 1) & O_ACC;
    if ((rmode & (~inode->i_mode)) & O_ACC) {
        kprintf("error: no permission\n");
        goto err;
    }
    if ((inode->i_type == I_DIRECTORY) && (rmode & I_W)) {
        kprintf("error: try to write raw dir\n");
        goto err;
    }
    if ((inode->i_type == I_REGULAR) && (rmode & I_W) && (flags & O_TRUNC)) {
        inode->i_size = 0;
        vfs_sync_inode(inode);
    }
    file = alloc_file();
    if (file == NULL) goto err;
    file->fd_dentry = dentry;
    file->fd_ops = dentry->d_inode->i_fop;
    file->fd_mapping = dentry->d_inode->i_mapping;
    file->fd_mode = rmode; // fd_mode: bit 0:read 1:write
    file->fd_pos = ((flags & O_APPEND) ? inode->i_size : 0);

    spinlock_release(&inode->lock);
    spinlock_release(&dentry->lock);
    vfs_put_dentry(dir);
    return file;

err:
    spinlock_release(&inode->lock);
    vfs_put_dentry(dentry);
no_dentry_out:
    vfs_put_dentry(dir);
    return NULL;
}

static inline void vfs_file_close(struct file_desc* file) {
    fput(file);
}

int kern_vfs_open(const char* path, int flags, int mode) {
    process_t* p_proc = proc_real(p_proc_current);
    int fd = -1, i;
    for (i = 0; i < NR_FILES; ++i) { // modified by mingxuan 2019-5-20
        if (p_proc->task.filp[i] == 0) {
            fd = i;
            break;
        }
    }
    if ((fd < 0) || (fd >= NR_FILES)) {
        kprintf("warn: filps for proc pid=%d is already full\n", proc2pid(p_proc));
        return -1;
    }
    // 此处拆解开始是因为 某些情况下用户对底层filp应当不可见(如opendir)
    struct file_desc* filp = vfs_file_open(path, flags, mode);
    if (filp == NULL) { return -1; }
    /* connects proc with file_descriptor */
    p_proc->task.filp[fd] = filp;
    return fd;
}

int kern_vfs_fsync(struct file_desc* file) {
    if (!file) { return -1; }
    struct inode* inode = file->fd_dentry->d_inode;
    //! FIXME: default value for ret is unspecific
    int ret = 0;
    spinlock_lock_or_yield(&inode->lock);
    if (inode->i_fop && inode->i_fop->fsync) { ret = inode->i_fop->fsync(file, 0); }
    spinlock_release(&inode->lock);
    return ret;
}

void fput(struct file_desc* file) {
    if (atomic_dec_and_test(&file->fd_count)) {
        kern_vfs_fsync(file);
        free_file(file);
    }
}

int kern_vfs_close(int fd) {
    process_t* p_proc = proc_real(p_proc_current);
    struct file_desc* file = p_proc->task.filp[fd];
    if (!file) { return -1; }
    vfs_file_close(file);
    p_proc->task.filp[fd] = 0;
    return 0;
}

int kern_vfs_lseek(int fd, int offset, int whence) {
    process_t* p_proc = proc_real(p_proc_current);
    struct file_desc* file = p_proc->task.filp[fd];
    if (!file) { return -1; }
    struct inode* inode = file->fd_dentry->d_inode;
    spinlock_lock_or_yield(&inode->lock);
    if (!inode_allow_lseek(inode->i_type)) { return -1; }
    ssize_t size = inode->i_size;
    spinlock_release(&inode->lock);
    u64 _offset = -1;
    switch (whence) {
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
    if ((int)_offset >= 0) {
        file->fd_pos = _offset;
        return _offset;
    }
    return -1;
}

int kern_vfs_read(int fd, char* buf, int count) {
    if (count < 0) { return -1; }
    if (fd < 0 || fd >= NR_FILES) { return -1; }
    struct file_desc* file = proc_real(p_proc_current)->task.filp[fd];
    if (file == NULL) { return -1; }
    if (!(file->fd_mode & I_R)) {
        kprintf("error: read: no permission\n");
        return -1;
    }
    if (buf == NULL) { return count == 0 ? 0 : -1; }

    struct inode* inode = file->fd_dentry->d_inode;
    if (inode->i_type == I_DIRECTORY) { return -1; /* -EISDIR */ }
    int cnt = 0;
    spinlock_lock_or_yield(&inode->lock);
    if (inode->i_fop && inode->i_fop->read) { cnt = inode->i_fop->read(file, count, buf); }
    if (cnt > 0) { inode->i_atime = current_timestamp; }
    spinlock_release(&inode->lock);
    return cnt;
}

int kern_vfs_write(int fd, const char* buf, int count) {
    if (count < 0) { return -1; }
    if (fd < 0 || fd >= NR_FILES) { return -1; }
    struct file_desc* file = proc_real(p_proc_current)->task.filp[fd];
    if (file == NULL) { return -1; }
    if (!(file->fd_mode & I_W)) {
        kprintf("error: write: no permission\n");
        return -1;
    }
    if (buf == NULL) { return count == 0 ? 0 : -1; }

    struct inode* inode = file->fd_dentry->d_inode;
    int cnt = 0;
    spinlock_lock_or_yield(&inode->lock);
    if (inode->i_fop && inode->i_fop->write) { cnt = inode->i_fop->write(file, count, buf); }
    if (cnt > 0) { inode->i_mtime = current_timestamp; }
    spinlock_release(&inode->lock);
    return cnt;
}

int kern_vfs_unlink(const char* path) {
    if (path == NULL) { return -1; }
    const size_t sz_path = strlen(path);
    if (sz_path == 0 || sz_path + 1 > MAX_PATH) { return -1; }

    //! FIXME: 如果不做 MAX_PATH 检查，路径长度溢出时 unlink 也会异常地返回
    //! 0，即下层方法仍存在安全隐患
    //! FIXME: 可以通过相对路径创建出绝对路径溢出 MAX_PATH 的问题件，该类文件
    //! 会导致 fs 崩溃，如 ls 遍历目录将导致 page fault

    char dir_path[MAX_PATH] = {0};
    const char* file_name = strip_dir_path(path, dir_path);
    struct dentry *dir = vfs_lookup(dir_path), *dentry = NULL;
    if (!dir) { return -1; }
    dentry = _do_lookup(dir, file_name, 0);
    if (!dentry) {
        spinlock_release(&dir->lock);
        return -1;
    }
    struct inode* inode = dentry->d_inode;
    spinlock_lock_or_yield(&inode->lock);
    if (inode->i_type == I_DIRECTORY) { goto err; }
    if (inode->i_op && inode->i_op->unlink) {
        spinlock_lock_or_yield(&dir->d_inode->lock);
        int state = inode->i_op->unlink(dir->d_inode, dentry);
        spinlock_release(&dir->d_inode->lock);
        if (state) { goto err; }
        inode->i_nlink--;
    }
    spinlock_release(&inode->lock);
    delete_dentry(dentry, dir);
    vfs_put_dentry(dentry);
    vfs_put_dentry(dir);
    return 0;
err:
    spinlock_release(&inode->lock);
    vfs_put_dentry(dentry);
    vfs_put_dentry(dir);
    return -1;
}

int kern_vfs_mknod(const char* path, int mode, int dev) {
    char dir_path[MAX_PATH] = {0};
    if ((!path) || strlen(path) == 0) { return -1; }
    const char* file_name = strip_dir_path(path, dir_path);
    struct dentry *dir = vfs_lookup(dir_path), *dentry = NULL;
    struct inode* inode = NULL;
    UNUSED(inode);
    if (!dir) { return -1; }
    dentry = _do_lookup(dir, file_name, 0);
    if (dentry) {
        init_special_inode(dentry->d_inode, mode & I_TYPE_MASK, mode & (~I_TYPE_MASK), dev);
    } else {
        dentry =
            vfs_create(dir->d_inode, file_name, mode & I_TYPE_MASK, mode & (~I_TYPE_MASK), dev);
        if (!dentry) {
            spinlock_release(&dir->lock);
            return -1;
        }
        spinlock_lock_or_yield(&dentry->lock);
        insert_sub_dentry(dir, dentry);
    }
    vfs_put_dentry(dentry);
    vfs_put_dentry(dir);
    return 0;
}

int kern_vfs_mkdir(const char* path, int mode) {
    if (path == NULL) { return -1; }
    size_t sz_path = strlen(path) + 1;
    if (sz_path == 0 || sz_path > MAX_PATH) { return -1; }
    char dir_path[MAX_PATH] = {0};
    const char* file_name = strip_dir_path(path, dir_path);
    struct dentry* dir = vfs_lookup(dir_path);
    struct dentry* dentry = NULL;
    if (!dir) { return -1; }
    dentry = _do_lookup(dir, file_name, 0);
    if (dentry) {
        vfs_put_dentry(dentry);
        vfs_put_dentry(dir);
        return -1;
    }
    dentry = vfs_create(dir->d_inode, file_name, I_DIRECTORY, mode & ~I_TYPE_MASK, 0);
    if (!dentry) {
        vfs_put_dentry(dir);
        return -1;
    }
    spinlock_lock_or_yield(&dentry->lock);
    insert_sub_dentry(dir, dentry);
    vfs_put_dentry(dentry);
    vfs_put_dentry(dir);
    return 0;
}

int kern_vfs_rmdir(const char* path) {
    char dir_path[MAX_PATH] = {};
    if (!path || strlen(path) == 0) { return -1; }
    if (strcmp(path, "/") == 0) {
        kprintf("rm root dir will damage system\n");
        return -1;
    }
    const char* file_name = strip_dir_path(path, dir_path);
    if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
        kprintf("path ended with . or .. is invalid\n");
        return -1;
    }
    struct dentry *dir = vfs_lookup(dir_path), *dentry = NULL;
    if (!dir) {
        return -1; // cant find parent
    }
    dentry = _do_lookup(dir, file_name, 0);
    if (!dentry) {
        vfs_put_dentry(dir);
        return -1; // cant find target dir
    }
    struct inode* inode = dentry->d_inode;
    spinlock_lock_or_yield(&inode->lock);
    if (dentry == vfs_root ||
        (dentry->d_vfsmount && dentry->d_vfsmount->mnt_root == dentry)) { // 不允许删除挂载点
        goto err;
    }
    if (inode->i_type != I_DIRECTORY) { goto err; }
    if (check_dir_entry_empty(dentry) != 0) { goto err; }
    if (inode->i_op && inode->i_op->rmdir) {
        spinlock_lock_or_yield(&dir->d_inode->lock);
        int state = inode->i_op->rmdir(dir->d_inode, dentry);
        spinlock_release(&dir->d_inode->lock);
        if (state) { goto err; }
        inode->i_nlink--;
    }
    spinlock_release(&inode->lock);
    struct dentry* sub;
    // clear sub dentry
    while (!list_empty(&dentry->d_subdirs)) {
        sub = list_front(&dentry->d_subdirs, struct dentry, d_list);
        delete_dentry(sub, dentry);
    }
    delete_dentry(dentry, dir);
    vfs_put_dentry(dentry);
    vfs_put_dentry(dir);
    return 0;
err:
    spinlock_release(&inode->lock);
    vfs_put_dentry(dentry);
    vfs_put_dentry(dir);
    return -1;
}

DIR* kern_vfs_opendir(const char* path) {
    struct file_desc* file = vfs_file_open(path, O_DIRECTORY | O_RDONLY, I_RWX);
    if (!file) { return NULL; }
    DIR* dirp = (DIR*)kern_malloc_4k();
    dirp->file = file;
    dirp->pos = 0;
    dirp->total = 0;
    dirp->init = 0;
    return dirp;
}

int kern_vfs_closedir(DIR* dirp) {
    struct file_desc* file = dirp->file;
    if (!file) { return -1; }
    vfs_file_close(file);
    kern_free_4k(dirp);
    return 0;
}

struct dirent* kern_vfs_readdir(DIR* dirp) {
    if (!dirp->init) {
        struct inode* inode = dirp->file->fd_dentry->d_inode;
        struct dirent* data_start = DIR_DATA(dirp);
        spinlock_lock_or_yield(&inode->lock);
        if (inode->i_fop && inode->i_fop->readdir) {
            dirp->total = inode->i_fop->readdir(dirp->file, SZ_4K - sizeof(DIR), data_start);
        }
        dirp->init = 1;
        spinlock_release(&inode->lock);
    }
    struct dirent* ent = NULL;
    if (dirp->pos < dirp->total) {
        ent = (struct dirent*)(((char*)DIR_DATA(dirp)) + dirp->pos);
        dirp->pos += ent->d_len;
        if (false) {
        } else if (strcmp(ent->d_name, "..") == 0) {
            // check parent needed for mount point
            struct dentry* dentry = dirp->file->fd_dentry;
            vfs_get_dentry(dentry);
            struct dentry* fa_dir = _do_lookup(dentry, ent->d_name, 1);
            assert(fa_dir != NULL);
            ent->d_ino = fa_dir->d_inode->i_no;
            vfs_put_dentry(fa_dir);
        } else if (strcmp(ent->d_name, ".") == 0) {
            ent->d_ino = dirp->file->fd_dentry->d_inode->i_no;
        }
    }
    return ent;
}

int kern_vfs_chdir(const char* path) {
    process_t* p_proc = proc_real(p_proc_current);
    struct dentry* dir = vfs_lookup(path);
    if (!dir) { return -1; }
    struct inode* inode = dir->d_inode;
    int err = -1;
    if (inode->i_type == I_DIRECTORY) {
        if (p_proc->task.cwd) { vfs_put_dentry(p_proc->task.cwd); }
        p_proc->task.cwd = dir;
        err = 0;
    }
    spinlock_release(&dir->lock);
    return err;
}

char* kern_vfs_getcwd(char* buf, int size) {
    if (NULL == buf || (-1 == (int)buf)) { return NULL; }
    process_t* p_proc = proc_real(p_proc_current);
    const bool truncated = vfs_get_path(p_proc->task.cwd, buf, size);
    return truncated ? NULL : buf;
}

int kern_vfs_stat(const char* path, struct stat* statbuf) {
    struct dentry* dentry = vfs_lookup(path);
    if (NULL == dentry) { return -1; }
    struct inode* inode = dentry->d_inode;
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
