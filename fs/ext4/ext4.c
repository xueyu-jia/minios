#include <fs/ext4/ext4.h>
#include <minios/clock.h>
#include <minios/assert.h>
#include <minios/vfs.h>
#include <minios/spinlock.h>
#include <klib/size.h>
#include <errno.h>
#include <string.h>

int ext4_sync_inode(struct inode* inode);
int ext4_get_block(struct inode* inode, u32 iblock, int create);
struct dentry* ext4_lookup(struct inode* dir, const char* filename);
struct buf_head* ext4_bread(struct inode* inode, ext4_lblk_t block, int create);
static void ext4_free_blocks(struct inode* inode);
void ext4_delete_inode(struct inode* inode);

/**
 * @brief 根据数据块大小计算块的实际大小
 *
 * @param s_log_block_size 数据块大小 (log2 表示)
 * @return u32 实际块大小
 */
static u32 ext_get_blocksize(u32 s_log_block_size) {
    u32 i = 0;
    u32 res = 1;
    for (i = 0; i < s_log_block_size; i++) { res = res * 2; }
    return res;
}

/**
 * @brief 检查给定字符是否为非法字符
 *
 * @param c 要检查的字符
 * @return int 非法字符返回 1，否则返回 0
 */
static inline int ext4_bad_char(char c) {
    // 不允许空字符和路径分隔符
    return (c == '\0' || c == '/');
}

/**
 * @brief 检查文件名是否合法
 *
 * @param name 要检查的文件名字符串
 * @return int 不合法返回 1，合法返回 0
 */
static int ext4_badname(const char* name) {
    if (name == NULL) return 1; // 空指针不合法

    for (const char* s = name; *s; s++) {
        // 判断是否包含非法字符
        if (ext4_bad_char(*s)) { return 1; }
    }

    return 0; // 文件名合法
}

/**
 * @brief 获取块组描述符
 *
 * @param sb 超级块
 * @param group 要获取的块组号
 * @param bh 存储块组描述符的缓冲区
 * @return struct ext4_group_desc* 块组描述符指针，失败返回 NULL
 *
 * @ATTENTION 未处理 flex_group 和 meta_group 情况
 */
struct ext4_group_desc* ext4_get_group_desc(struct super_block* sb, ext4_group_t group,
                                            struct buf_head** bh) {
    unsigned int group_desc;
    unsigned int offset;
    ext4_group_t ngroups = EXT4_TOTAL_GROUP_COUNT(sb);
    struct ext4_group_desc* desc;
    struct ext4_sb_info* sbi = EXT_SB(sb);

    if (group >= ngroups) { return NULL; }

    group_desc = group / EXT4_DESC_PER_BLOCK(sb);
    offset = group % EXT4_DESC_PER_BLOCK(sb);

    *bh = bread(sb->sb_dev, sbi->s_first_data_block + 1 + group_desc);
    if (!bh) { return NULL; }

    desc = (struct ext4_group_desc*)((char*)((*bh)->buffer) + offset * EXT4_DESC_SIZE(sb));

    return desc;
}

/**
 * @brief 为为新创建的文件或目录先找到一个有空闲 inode 和空闲 block
 * 的块组，优先查找父目录所属块组。查找失败则遍历所有块组，看哪个有空闲 inode 和
 * block，找到合适块组把块组号赋值给 group
 *
 * @param sb 超级块
 * @param parent 父目录 inode
 * @param group 找到的块组号
 * @return int
 *
 * @ATTENTION 未考虑 flex_group 和 meta_group
 */
static int find_group_other(struct super_block* sb, struct inode* parent, ext4_group_t* group) {
    struct ext4_sb_info* sbi = EXT_SB(sb);
    ext4_group_t ngroups = EXT4_TOTAL_GROUP_COUNT(sb);
    ext4_group_t parent_bg = (parent->i_no - 1) / sbi->s_inodes_per_group;
    struct ext4_group_desc* desc;

    // 先查找父目录块组，如果父目录块组有空闲 inode 和 block 则直接返回，该块组就是本次选中块组
    *group = parent_bg;
    struct buf_head* group_desc_bh_handle = NULL;
    desc = ext4_get_group_desc(sb, *group, &group_desc_bh_handle);

    bool has_free_inode = false;
    bool has_free_block = false;
    has_free_inode =
        (desc->bg_free_inodes_count_hi << 16 | desc->bg_free_inodes_count_lo) ? true : false;
    has_free_block =
        (desc->bg_free_blocks_count_hi << 16 | desc->bg_free_blocks_count_lo) ? true : false;

    if (desc && has_free_block && has_free_inode) {
        brelse(group_desc_bh_handle);
        return 0;
    }

    brelse(group_desc_bh_handle);
    // 执行到这里说明父目录所属块组没有空闲的 inode 和 block

    // TODO: 未验证
    // 计算 group 为下一个块组号
    *group = (*group + parent->i_no) % ngroups;

    // 遍历所有块组，找到有空闲 inode 和 block 的块组
    for (int i = 0; i < ngroups; ++i) {
        if (++*group >= ngroups) { *group = 0; }
        desc = ext4_get_group_desc(sb, *group, &group_desc_bh_handle);
        // 新找到的块组 group 有空闲的 inode 和 block，那它就是要找到的块组
        // ATTENTION: linux 源码先采用哈希搜索再采用遍历，并且遍历时不再考虑空闲 block
        // 限制，这里我们是直接遍历搜索的
        bool has_free_inode = false;
        bool has_free_block = false;
        has_free_inode =
            (desc->bg_free_inodes_count_hi << 16 | desc->bg_free_inodes_count_lo) ? true : false;
        has_free_block =
            (desc->bg_free_blocks_count_hi << 16 | desc->bg_free_blocks_count_lo) ? true : false;

        if (desc && has_free_block && has_free_inode) {
            brelse(group_desc_bh_handle);
            return 0;
        }
    }

    brelse(group_desc_bh_handle);
    return -1;
}

/**
 * @brief 找到一个合适的块组，从该块组分配一个空闲 inode
 *
 * @param dir 待创建文件/目录的父目录
 * @param mode vfs inode 的 mode
 * @param name 待创建文件/目录名
 * @param is_dir 是否是目录
 * @return struct inode*
 */
static struct inode* ext4_new_inode(struct inode* dir, int mode, const char* name, bool is_dir) {
    UNUSED(name);
    struct super_block* sb;
    struct ext4_sb_info* sbi;
    struct ext4_group_desc* gdp = NULL;
    struct buf_head* group_desc_bh = NULL;
    struct buf_head* inode_bitmap_bh = NULL;
    ext4_group_t ngroups, group;
    int ret2;
    unsigned long ino = 0; // 表示块组内 inode 的编号

    // 检查目录有效性
    if (!dir || !dir->i_nlink) { return (struct inode*)(-EPERM); }

    // 获取超级块和相关数据
    sb = dir->i_sb;
    sbi = EXT_SB(sb);
    ngroups = EXT4_TOTAL_GROUP_COUNT(sb);

    // 通过 vfs 分配 inode
    struct inode* inode = vfs_new_inode(sb);
    if (!inode) { return (struct inode*)(-ENOMEM); }
    // 获取 ext4_inode_info
    struct ext4_inode_info* ei = EXT_INODE(inode);

    // 为新创建的文件或目录找到一个有空闲 inode 和 block 的块组
    // TODO: 这里可以优化，对目录和文件使用不同的查找算法
    // if (is_dir) {
    //     ret2 = find_group_orlov();
    // } else {
    //     ret2 = find_group_other();
    // }
    ret2 = find_group_other(sb, dir, &group);

    // TODO：linux 源码在这里记录了记录最近一次分配的 inode 所属的块组
    // EXT4_I(dir)->i_last_alloc_group = group;
    // 到这里 group 表示本次分配的 inode 所在的块组
    if (ret2 == -1) { goto out; }

    // 通常下面的循环只会进行一次，但是如果非常不巧，我们选中的块组的最后一个 inode
    // 被占用了，我们就需要再找一遍块组。但是这种情况应该是非常少见的
    for (int i = 0; i < ngroups; ++i, ino = 0) {
        // 由块组编号 group 得到块组描述符结构 ext4_group_desc，并且令 group_desc_bh
        // 指向保存块组描述符数据的物理块映射的 bh
        gdp = ext4_get_group_desc(sb, group, &group_desc_bh);
        bool has_free_inode = false;
        has_free_inode =
            (gdp->bg_free_inodes_count_hi << 16 | gdp->bg_free_inodes_count_lo) ? true : false;
        if (!has_free_inode) {
            if (++group >= ngroups) { group = 0; }
            continue;
        }
        // TODO: 由于在 i386 下，这里只考虑了低 32 位地址，所以 inode_bitmap_lo 就是 inode_bitmap
        // 的地址
        u32 bitmap_lo = gdp->bg_inode_bitmap_lo;
        brelse(group_desc_bh);
        inode_bitmap_bh = bread(sb->sb_dev, bitmap_lo);
        if (!inode_bitmap_bh) {
            brelse(inode_bitmap_bh);
            goto out;
        }
    }

    while (1) {
        // 遍历 bitmap(inode_bitmap_bh->buffer)，找到一个空闲 inode
        u8* inode_bitmap = (u8*)inode_bitmap_bh->buffer;
        u32 block_size = EXT4_BLOCK_SIZE(sb); // 获取块大小
        bool found = false;

        // 遍历每个字节
        for (u32 byte_nr = 0; byte_nr < block_size; byte_nr++) {
            if (inode_bitmap[byte_nr] == 0xFF) continue; // 如果字节中所有位都被占用，跳过

            // 遍历字节中的每一位
            for (u32 bit_nr = 0; bit_nr < 8; bit_nr++) {
                if (!(inode_bitmap[byte_nr] & (1 << bit_nr))) {
                    ino = byte_nr * 8 + bit_nr; // 计算得到空闲 inode 编号
                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        // TODO: 未验证
        if (!found || ino >= sbi->s_inodes_per_group ||
            (group == 0 && (ino + 1) < sbi->s_first_ino)) {
            // 当前组中没有找到空闲 inode，或者找到的 inode 编号大于块组中最大 inode
            // 数量，或者找到的 inode 编号小于 first_ino，尝试下一个组
            if (++group >= ngroups) { group = 0; }
        } else {
            // 标记找到的inode为已使用
            inode_bitmap[ino / 8] |= (1 << (ino % 8));
            mark_buff_dirty(inode_bitmap_bh);
            brelse(inode_bitmap_bh);
            // ino 从 1 开始计数，bitmap 从 0 开始，所以要加 1
            ino++;
            // 退出 while循环
            break;
        }
    }

    // 更新相关的 bg desc 属性
    u32 group_desc = group / EXT4_DESC_PER_BLOCK(sb);
    u32 offset = group % EXT4_DESC_PER_BLOCK(sb);

    group_desc_bh = bread(sb->sb_dev, sbi->s_first_data_block + 1 + group_desc);
    struct ext4_group_desc* p_ext_desc =
        (struct ext4_group_desc*)((char*)group_desc_bh->buffer + offset * EXT4_DESC_SIZE(sb));
    p_ext_desc->bg_free_inodes_count_lo--;
    if (is_dir) { p_ext_desc->bg_used_dirs_count_lo++; }
    mark_buff_dirty(group_desc_bh);
    brelse(group_desc_bh);

    // 空闲 inode 数减一
    // TODO: 理论上这里应该要读出超级块然后改超级块磁盘上的相应位置的数据
    sbi->s_free_inodes_count--;
    // if (is_dir) { sbi->s_dirs_count++; }

    // 初始化 vfs inode
    inode->i_no = group * sbi->s_inodes_per_group + ino;
    inode->i_mtime = inode->i_atime = inode->i_crtime = current_timestamp;
    inode->i_mode = mode;
    inode->i_sb = dir->i_sb;
    inode->i_dev = dir->i_sb->sb_dev;
    inode->i_type = is_dir ? I_DIRECTORY : I_REGULAR;
    inode->i_op = &ext4_inode_ops;
    inode->i_fop = &ext4_file_ops;

    // 初始化 vfs inode info
    ei->i_bg_block = group;
    // 下面要为这个 inode 新建并维护一个 extent 树
    // TODO: 这里的树只有一个根和四片叶子
    // 1、先找到一个空闲的 Data block，如果没有就 create 一个
    int ret = 0;
    ret = ext4_get_block(inode, 0, 1);
    if (ret == -1) {
        assert("ext4 could not get a new block" && true);
        goto out;
    }
    ext4_lblk_t block = (ext4_lblk_t)ret;

    // 如果是目录，需要初始化它的第一个 ext4_dir_entry_2
    if (is_dir) {
        buf_head* bh = NULL;
        bh = bread(sb->sb_dev, block);
        // 维护前 24 字节
        // 当前目录
        struct ext4_dir_entry_2 dot;
        dot.inode = inode->i_no;
        dot.rec_len = 12;
        dot.name_len = 1;
        dot.file_type = 2;
        memset(&dot.name, '.', 1 * sizeof(char));

        // 父目录
        struct ext4_dir_entry_2 dotdot;
        dotdot.inode = dir->i_no;
        dotdot.rec_len = 12;
        dotdot.name_len = 2;
        dotdot.file_type = 2;
        memset(&dotdot.name, '.', 2 * sizeof(char));

        memcpy(bh->buffer, &dot, 12);
        memcpy(bh->buffer + 12, &dotdot, 12);
        // 从 24 字节开始是一个 ext4_dir_entry_2 结构
        struct ext4_dir_entry_2* de = (struct ext4_dir_entry_2*)((char*)bh->buffer + 24);
        de->inode = 0;
        de->rec_len = EXT4_BLOCK_SIZE(sb) - 24;
        mark_buff_dirty(bh);
        brelse(bh);
    }

    // 2、把这个 Data block 分配给 inode, 创建并维护这个 inode 的 extent 树
    struct ext4_inode_info* new_ei = EXT_INODE(inode);
    struct ext4_inode_info* dir_info = EXT_INODE(dir);
    struct ext4_extent_header* extent_header = (struct ext4_extent_header*)(new_ei->i_block);
    extent_header->eh_magic = 0xF30A;
    extent_header->eh_entries = 1;
    extent_header->eh_max = 4;
    extent_header->eh_depth = 0;
    extent_header->eh_generation = 0;

    struct ext4_extent* extent = (struct ext4_extent*)((char*)new_ei->i_block + 12);
    extent->ee_block = 0;
    extent->ee_len = 1;
    extent->ee_start_lo = block;
    extent->ee_start_hi = 0;

    new_ei->i_size = 1 * EXT4_BLOCK_SIZE(sb);
    new_ei->i_blocks_lo = 8;
    new_ei->i_links_count = is_dir ? 2 : 1;
    if (is_dir) dir_info->i_links_count++;

out:
    // TODO: 这里应该根据不同的错误情况返回不同 err
    return inode;
}

/**
 * @brief 把 dentry 和 inode 对应的文件或目录添加到父目录的 ext4_dir_entry_2中
 *
 * @param dentry 待创建的目录或文件
 * @param dir 父目录
 * @return int NULL if failed
 *
 * @note The inode part of 'de' is left at 0 - which means you
 * may not sleep between calling this and putting something into
 * the entry, as someone else might have used it while you slept.
 *
 * @ATTENTION 未考虑 inline data，dir index 特性
 */
static int ext4_add_entry(struct dentry* dentry, struct inode* dir) {
    struct inode* inode = dentry->d_inode;
    struct buf_head* bh = NULL;
    struct ext4_dir_entry_2* de;
    struct super_block* sb;

    unsigned blocksize;
    ext4_lblk_t block, blocks;

    sb = dir->i_sb;
    blocksize = EXT4_BLOCK_SIZE(sb);

    const unsigned short namelen = strlen(dentry_name(dentry));
    if (namelen == 0) { return -EINVAL; }
    if (ext4_lookup(dir, dentry_name(dentry))) { return -EEXIST; }

    // TODO: 这里 linux 源码对 inline data 和 dir index 特殊处理，但是我们暂时不考虑

    blocks = dir->i_size / blocksize + 1;
    bool found = false;
    // 下面遍历所有父目录逻辑块地址 0~blocks，找出一个空闲的 ext4_dir_entry_2
    // 结构，最后把本次的子文件/目录写进去，从而建立子文件/目录和父目录的联系
    for (block = 0; block < blocks; ++block) {
        bh = ext4_bread(dir, block, 0);
        if (!bh) { return -EIO; }
        // 遍历块中的所有目录项
        // ATTENTION: 这里从 24 开始，因为一个块的前 24 字节是保留信息
        de = (struct ext4_dir_entry_2*)((u8*)bh->buffer + 24);
        while ((u8*)de < (u8*)bh->buffer + blocksize) {
            // 按 4 字节对齐
            unsigned short reclen = namelen + 8 + 4 - (namelen) % 4;
            // 如果当前目录项是最后一个目录项
            if ((u8*)de + de->rec_len == (u8*)bh->buffer + blocksize) {
                // 当前最后一个目录项的真正的rec_len
                unsigned short tail_rec_len = de->name_len + 8 + 4 - (de->name_len) % 4;
                if (de->rec_len - tail_rec_len >= reclen) {
                    // 除去当前最后一个目录项剩下的字节空间
                    unsigned short rest_len = de->rec_len - tail_rec_len;
                    de->rec_len = tail_rec_len;
                    de = EXT4_NEXT_DIR_ENTRY_2(de);
                    // 到这里 de 已经是下一个也是最后一个目录项了
                    de->rec_len = rest_len;
                    de->inode = inode->i_no;
                    de->name_len = namelen;
                    de->file_type = inode->i_type;

                    char* name = (char*)de + 8;
                    memcpy(name, dentry_name(dentry), namelen);
                    dir->i_mtime = dir->i_crtime = current_timestamp;
                    mark_buff_dirty(bh);
                    found = true;
                } else {
                    found = false;
                }
                break;
            }
            // 到这里说明不是最后一个目录项，需要找到一个空闲的 ext4_dir_entry_2 结构
            if (de->inode == 0) {
                // 到这里说明这个 ext4_dir_entry_2 结构是空闲的，可以写入新的文件/目录信息
                // 还需要判断够不够放下新的文件/目录信息
                if (de->rec_len >= reclen) {
                    // 可以放下新的文件/目录信息
                    de->inode = inode->i_no;
                    de->name_len = namelen;
                    de->file_type = inode->i_type;
                    char* name = (char*)de + 8;
                    memcpy(name, dentry_name(dentry), namelen);
                    dir->i_mtime = dir->i_crtime = current_timestamp;
                    mark_buff_dirty(bh);
                    found = true;
                    break;
                }
                if (de->rec_len == 0) {
                    // 防止无限循环，因为如果 inode 和 rec_len 都为 0，那么就会一直在这个全 0 的 de
                    assert("ext4_add_entry: both rec_len and inode are 0" && true);
                    break;
                }
            }
            de = EXT4_NEXT_DIR_ENTRY_2(de);
        }
        // 如果找到了空闲的 ext4_dir_entry_2 结构，就退出循环
        if (found) { break; }

        // 到这里，没有找到空闲的 ext4_dir_entry_2 结构，释放 bh，继续下一个块
        brelse(bh);
    }

    // 如果遍历完父目录的所有块都没有找到空闲的 ext4_dir_entry_2 结构，那么就需要再申请一个块
    if (found) {
        brelse(bh);
        return 0;
    } else {
        // TODO: 申请一个新块
        return -1;
    }
}

/**
 * @brief 释放一个 inode 并更新相关元数据
 *
 * @param inode 要释放的 inode 对象
 * @return void
 *
 * @details
 * 1. 计算要释放的 inode 所属的块组。
 * 2. 更新 inode 位图，清除对应的位。
 * 3. 更新块组描述符中的空闲 inode 和目录计数。
 * 4. 更新超级块中的空闲 inode 计数。
 */
static void ext4_free_inode(struct inode* inode) {
    struct super_block* sb = inode->i_sb;
    struct ext4_sb_info* sbi = EXT_SB(sb);
    buf_head* bh = NULL;

    // 释放inode
    // 计算inode所在的块组
    u32 group = (inode->i_no - 1) / sbi->s_inodes_per_group;
    u32 offset = (inode->i_no - 1) % sbi->s_inodes_per_group;
    u32 bit_offset = offset % 8;
    u32 byte_offset = offset / 8;

    // 读取块组描述符
    u32 gd_block = sbi->s_first_data_block + 1;
    struct ext4_group_desc* gdp;
    bh = bread(sb->sb_dev, gd_block);
    gdp = (struct ext4_group_desc*)bh->buffer + group;

    // 更新inode位图
    u32 imap_block = gdp->bg_inode_bitmap_lo;
    brelse(bh);

    bh = bread(sb->sb_dev, imap_block);
    u8* bitmap = (u8*)bh->buffer;
    bitmap[byte_offset] &= ~(1 << bit_offset); // 清除位图中对应的位
    mark_buff_dirty(bh);
    brelse(bh);

    // 更新块组描述符中的空闲inode计数和目录计数
    bh = bread(sb->sb_dev, gd_block);
    gdp = (struct ext4_group_desc*)bh->buffer + group;
    gdp->bg_free_inodes_count_lo++;
    gdp->bg_used_dirs_count_lo--; // 减少目录计数
    mark_buff_dirty(bh);
    brelse(bh);
    // 更新超级块中的空闲inode计数
    sbi->s_free_inodes_count++;
}

/**
 * @brief 释放 inode 所引用的空闲块并更新相关元数据
 *
 * @param inode 要释放的 inode 对象
 * @return void
 *
 * @details
 * 1. 遍历 inode 的 extent 表，获取每个 extent 的起始块号和块长度。
 * 2. 对于每个块：
 *    - 读取该块，清零其内容。
 *    - 标记块为脏以确保后续写回磁盘。
 * 3. 更新块位图：
 *    - 读取块组描述符，获取块位图所在块号。
 *    - 读取块位图，将释放的块在位图中标记为未使用。
 *    - 标记块位图为脏以确保后续写回磁盘。
 * 4. 更新块组描述符中的空闲块计数：
 *    - 读取块组描述符。
 *    - 增加空闲块计数。
 *    - 标记块组描述符为脏以确保后续写回磁盘。
 * 5. 更新超级块中的空闲块计数。
 */
static void ext4_free_blocks(struct inode* inode) {
    struct super_block* sb = inode->i_sb;
    struct ext4_sb_info* sbi = EXT_SB(sb);
    buf_head* bh = NULL;

    struct ext4_inode_info* ei = EXT_INODE(inode);
    struct ext4_extent_header* e_header = (struct ext4_extent_header*)(ei->i_block);

    for (int i = 1; i <= e_header->eh_entries; i++) {
        struct ext4_extent* e_extent = (struct ext4_extent*)(((u8*)ei->i_block) + i * 12);
        u16 block_len = e_extent->ee_len;
        u32 start_block_num = e_extent->ee_start_lo;

        // 计算块组和组内偏移
        u32 group = start_block_num / sbi->s_blocks_per_group;
        u32 start_offset = start_block_num % sbi->s_blocks_per_group;
        u32 end_offset = start_offset + block_len - 1;

        for (u32 j = start_offset; j <= end_offset; j++) {
            bh = bread(sb->sb_dev, j);
            memset(bh->buffer, 0, 4096);
            mark_buff_dirty(bh);
            brelse(bh);
        }

        // 读取块组描述符获取块位图位置
        u32 gd_block = sbi->s_first_data_block + 1;
        struct ext4_group_desc* gdp;
        bh = bread(sb->sb_dev, gd_block);
        gdp = (struct ext4_group_desc*)bh->buffer + group;
        u32 bitmap_block = gdp->bg_block_bitmap_lo;
        brelse(bh);

        // 读取块位图
        bh = bread(sb->sb_dev, bitmap_block);
        u8* bitmap = (u8*)bh->buffer;

        // 计算位图中的字节范围
        u32 start_byte = start_offset / 8;
        u32 end_byte = end_offset / 8;
        u32 start_bit = start_offset % 8;
        u32 end_bit = end_offset % 8;

        // 如果在同一个字节内
        if (start_byte == end_byte) {
            u8 mask = ((1 << (end_bit - start_bit + 1)) - 1) << start_bit;
            bitmap[start_byte] &= ~mask;
        } else {
            // 处理第一个字节
            if (start_bit != 0) {
                bitmap[start_byte] &= (0xFF << start_bit);
                start_byte++;
            }

            // 处理中间的完整字节
            memset(&bitmap[start_byte], 0, end_byte - start_byte);

            // 处理最后一个字节
            if (end_bit != 7) { bitmap[end_byte] &= ((1 << (8 - end_bit)) - 1); }
        }

        mark_buff_dirty(bh);
        brelse(bh);

        // 更新块组描述符中的空闲块计数
        bh = bread(sb->sb_dev, gd_block);
        gdp = (struct ext4_group_desc*)bh->buffer + group;
        gdp->bg_free_blocks_count_lo += block_len;
        mark_buff_dirty(bh);
        brelse(bh);

        // 更新超级块中的空闲块计数
        sbi->s_free_blocks_count_lo += block_len;
    }
}

/**
 * @brief 检查目录是否为空
 *
 * @param dir 要检查的目录 inode 对象
 * @return int 空返回 0，非空返回 -1 或 -2
 */
static int ext4_check_dir_empty(struct inode* dir) {
    // 定义相关信息
    struct super_block* sb = dir->i_sb;
    struct ext4_inode_info* ei = EXT_INODE(dir);
    struct ext4_extent* ex = (struct ext4_extent*)((u8*)ei->i_block + 12);
    buf_head* bh = NULL;

    u32 start_block = ex->ee_start_lo;
    bh = bread(sb->sb_dev, start_block);

    // 每一块的前24个字节为保留信息 来标识这个块所有者(inode)等相关信息
    struct ext4_dir_entry_2* de = (struct ext4_dir_entry_2*)((u8*)bh->buffer + 24);

    // 遍历块中的所有目录项
    while ((char*)de < (char*)bh->buffer + sb->sb_blocksize) {
        // 跳过无效的目录项
        if (de->inode == 0 || de->rec_len == 0) {
            brelse(bh);
            return 0;
        }

        char* name = (char*)de + 8;
        // 比较文件名
        if (!strncmp(name, ".", de->name_len) || !strncmp(name, "..", de->name_len)) {
            // 找到匹配的目录项
            continue;
        }

        // 移动到下一个目录项
        de = EXT4_NEXT_DIR_ENTRY_2(de);
        brelse(bh);
        return -1;
    }
    // 错误码以供调试
    return -2;
}

/**
 * @brief 删除目录项并释放相关资源
 *
 * @param dir 目录 inode 对象
 * @param dentry 目录项指针
 * @return int 成功返回 0，失败返回 -1
 */
static int ext4_unlink_ino(struct inode* dir, struct dentry* dentry) {
    struct inode* inode = dentry->d_inode;
    struct super_block* sb = dir->i_sb;
    buf_head* bh = NULL;

    // 找到目录项并删除
    struct ext4_inode_info* ei = EXT_INODE(dir);
    struct ext4_extent_header* eh = (struct ext4_extent_header*)ei->i_block;

    // 遍历目录的所有数据块
    for (int i = 1; i <= eh->eh_entries; i++) {
        struct ext4_extent* ex = (struct ext4_extent*)(((u8*)ei->i_block) + i * 12);
        u32 start_block = ex->ee_start_lo;
        u32 block_len = ex->ee_len;

        // 读取每个数据块
        for (u32 j = 0; j < block_len; j++) {
            bh = bread(sb->sb_dev, start_block + j);
            // 每一块的前24个字节为保留信息
            struct ext4_dir_entry_2* de = (struct ext4_dir_entry_2*)((u8*)bh->buffer + 24);

            // 遍历块中的所有目录项
            while ((char*)de < (char*)bh->buffer + sb->sb_blocksize) {
                // TODO: 这里的判断条件是不是要改成name判断
                if (de->inode == inode->i_no) {
                    // 找到目标目录项，将其标记为删除
                    memset(de, 0, 4);
                    memset((char*)de + 6, 0, de->rec_len - 6);
                    mark_buff_dirty(bh);
                    brelse(bh);
                    goto found;
                }

                // 移动到下一个目录项
                if (de->rec_len == 0) break;
                de = EXT4_NEXT_DIR_ENTRY_2(de);
            }
            brelse(bh);
        }
    }
    return -1; // 未找到目录项

found:
    // 释放目录占用的数据块
    ext4_free_blocks(inode);

    ext4_free_inode(inode);
    ext4_sync_inode(inode);
    return 0;
}

/**
 * @brief 读取目录文件的内容并填充目录项
 *
 * @param file   指向要读取的目录文件的文件描述符
 * @param count  用户提供的缓冲区剩余大小
 * @param start  指向用户提供的缓冲区，用于存储目录项
 * @return int   实际填充的字节数
 *
 * @note
 * - 遍历目录的所有数据块，逐个读取块内容并处理目录项。
 * - 跳过无效或空的目录项。
 * - 当缓冲区空间不足时，停止填充并返回已填充的字节数。
 */
int ext4_readdir(struct file_desc* file, unsigned int count, struct dirent* start) {
    struct inode* dir = file->fd_dentry->d_inode;
    struct super_block* sb = dir->i_sb;
    struct ext4_inode_info* dir_info = EXT_INODE(dir);
    struct ext4_extent_header* eh = (struct ext4_extent_header*)dir_info->i_block;
    buf_head* bh = NULL;
    struct dirent* dent = start;

    // 遍历目录的所有数据块
    for (int i = 1; i <= eh->eh_entries; i++) {
        struct ext4_extent* ex = (struct ext4_extent*)(((u8*)dir_info->i_block) + i * 12);
        u32 start_block = ex->ee_start_lo;
        u32 block_len = ex->ee_len;

        // 读取每个数据块
        for (u32 j = 0; j < block_len; j++) {
            bh = bread(sb->sb_dev, start_block + j);
            struct ext4_dir_entry_2* de = (struct ext4_dir_entry_2*)((u8*)bh->buffer);

            // 遍历块中的所有目录项
            while ((char*)de < (char*)bh->buffer + sb->sb_blocksize) {
                // 跳过无效的目录项
                if (de->inode == 0 && de->rec_len == 0) {
                    brelse(bh);
                    goto done;
                }
                if (de->inode == 0) {
                    de = (struct ext4_dir_entry_2*)((char*)de + de->rec_len);
                    continue;
                }
                if (count < dirent_len(de->name_len)) {
                    brelse(bh);
                    return (u32)dent - (u32)start;
                }
                char* name = (char*)de + 8;

                dirent_fill(dent, de->inode, de->name_len);
                // ATTENTION: 这里是要加1留0的
                strncpy(dent->d_name, name, de->name_len + 1);
                count -= dent->d_len;
                dent = dirent_next(dent);
                // 移动到下一个目录项
                de = EXT4_NEXT_DIR_ENTRY_2(de);
            }
            brelse(bh);
        }
    }
done:
    return (u32)dent - (u32)start;
}

/**
 * @brief 获取 inode 的第 iblock 个数据块在设备的块号(块大小由超级块 blocksize
 * 指定)，如果指定的数据块不存在且 create 为
 * 1，则实际文件系统进行数据块的分配。最终如果成功返回非负的设备块号，失败返回-1
 *
 * @param inode
 * @param iblock
 * @param create
 * @return PUBLIC
 */
int ext4_get_block(struct inode* inode, u32 iblock, int create) {
    struct ext4_extent_header* extent_header;
    // 1、找到 ext4_extent_header
    extent_header = (struct ext4_extent_header*)(EXT_INODE(inode)->i_block);
    // 2、找到当前的 block 所代表的逻辑块号映射的物理块号
    // TODO: 通过 extent 树查询，这里先假设只有四个块被映射
    for (int i = 0; i < extent_header->eh_entries; ++i) {
        struct ext4_extent* extent = (struct ext4_extent*)((u8*)extent_header + 12 + i * 12);
        if (extent->ee_block <= iblock && extent->ee_block + extent->ee_len >= iblock) {
            // 3、读取物理块，iblock 的物理块号是通过 extent
            // 映射的一段逻辑块到一段物理块计算的，我们的 iblock
            // 表示在这一段逻辑块的块号，对应的物理块号就是这一段逻辑块对应的第一个物理块的块号再加上
            // iblock，之后减去这一段逻辑块的起始地址（相当于计算这个 iblock
            // 所在的那一段逻辑块内的偏移）
            return extent->ee_start_lo + iblock - extent->ee_block;
        }
    }

    // 到这里说明没有对应的物理块，如果 create 不为 0 就需要新增，为 0 返回 -1
    if (!create) { return -1; }

    // 到这里说明需要新增
    // TODO: 假设新增后也不超过 4 个 ext4_extent
    if (extent_header->eh_entries >= 4) {
        // 没有多余空间了
        return -1;
    }

    // 新分配的 extent
    struct ext4_extent* extent =
        (struct ext4_extent*)((u8*)extent_header + 12 + extent_header->eh_entries * 12);

    // 下面查找有空闲物理块的块组
    struct super_block* sb = inode->i_sb;
    struct ext4_sb_info* sbi = EXT_SB(sb);
    // 优先考虑 inode 所在块组
    ext4_group_t group = EXT_INODE(inode)->i_bg_block;
    ext4_group_t ngroups = EXT4_TOTAL_GROUP_COUNT(sb);
    struct ext4_group_desc* desc;
    struct buf_head* group_desc_bh_handle = NULL;
    desc = ext4_get_group_desc(sb, group, &group_desc_bh_handle);
    bool has_free_block = false;
    has_free_block =
        (desc->bg_free_blocks_count_hi << 16 | desc->bg_free_blocks_count_lo) ? true : false;

    if (!has_free_block) {
        brelse(group_desc_bh_handle);
        // 如果 inode 所在块组没有空闲块，就遍历所有块组找到有空闲块的块组
        for (int i = 0; i < ngroups; ++i) {
            desc = ext4_get_group_desc(sb, group, &group_desc_bh_handle);
            has_free_block = false;
            has_free_block = (desc->bg_free_blocks_count_hi << 16 | desc->bg_free_blocks_count_lo)
                                 ? true
                                 : false;
            if (!has_free_block) {
                if (++group >= ngroups) { group = 0; }
                desc = ext4_get_group_desc(sb, group, &group_desc_bh_handle);
                has_free_block =
                    (desc->bg_free_blocks_count_hi << 16 | desc->bg_free_blocks_count_lo) ? true
                                                                                          : false;
            } else {
                break;
            }
        }
    }

    // NOTE: 这里报错码可能不是这个
    if (!has_free_block) { return -ENOSPC; }

    // 到这里 group 就是找到的有空闲块的块组的块组号
    // 遍历数据块位图，找到一个空闲块
    u32 block_bitmap_block = desc->bg_block_bitmap_lo;
    brelse(group_desc_bh_handle);
    buf_head* block_bitmap_bh = bread(sb->sb_dev, block_bitmap_block);
    u8* block_bitmap = (u8*)block_bitmap_bh->buffer;
    u32 block_size = EXT4_BLOCK_SIZE(sb); // 获取块大小
    ext4_lblk_t bno = 0;
    bool found = false;

    // 遍历每个字节
    for (u32 byte_nr = 0; byte_nr < block_size; byte_nr++) {
        if (block_bitmap[byte_nr] == 0xFF) continue; // 如果字节中所有位都被占用，跳过

        // 遍历字节中的每一位
        for (u32 bit_nr = 0; bit_nr < 8; bit_nr++) {
            if (!(block_bitmap[byte_nr] & (1 << bit_nr))) {
                bno = byte_nr * 8 + bit_nr;
                found = true;
                break;
            }
        }
        if (found) break;
    }

    // 标记找到的 block 为已使用
    block_bitmap[bno / 8] |= (1 << (bno % 8));
    mark_buff_dirty(block_bitmap_bh);
    brelse(block_bitmap_bh);

    // 到这里查找到了空闲块以及其在块组中的块偏移量 bno
    // 更新相关的 bg desc 属性
    desc = ext4_get_group_desc(sb, group, &group_desc_bh_handle);
    desc->bg_free_blocks_count_lo--;
    mark_buff_dirty(group_desc_bh_handle);
    brelse(group_desc_bh_handle);

    // 更新超级块中的空闲块计数
    sbi->s_free_blocks_count_lo--;

    // 更新 extent
    extent->ee_block = iblock;
    extent->ee_len = 1;
    extent->ee_start_hi = 0;
    extent->ee_start_lo = group * sbi->s_blocks_per_group + bno;

    return group * sbi->s_blocks_per_group + bno;
}

/**
 * @brief 读取 inode 对应的父目录中的逻辑块，以 buf_head 结构返回
 *
 * @param inode 父目录 inode
 * @param block 逻辑块号
 * @param create
 * @return struct buf_head*
 */
struct buf_head* ext4_bread(struct inode* inode, ext4_lblk_t block, int create) {
    struct buf_head* bh;
    // 1、找到当前的 block 所代表的逻辑块号映射的物理块号
    int block_no = -1;
    block_no = ext4_get_block(inode, block, create);
    if (block_no == -1) {
        // 到这里说明没找到
        return NULL;
    }
    // 2、读取该物理块
    bh = bread(inode->i_sb->sb_dev, block_no);
    return bh;
}

/**
 * @brief 删除一个目录项（仅适用于空目录）
 *
 * @param dir     父目录的 inode 对象
 * @param dentry  要删除的目录项
 * @return int    成功返回 0，失败返回 -1
 *
 * @note
 * - 首先检查目录是否为空，仅空目录允许删除。
 * - 调用 `ext4_unlink_ino` 执行目录项的解除链接操作。
 */
int ext4_rmdir(struct inode* dir, struct dentry* dentry) {
    if (ext4_check_dir_empty(dentry->d_inode) != 0) { return -1; }

    return ext4_unlink_ino(dir, dentry);
}

/**
 * @brief 在 dir 中创建文件, dentry 中保存要创建的文件名, dentry 中 inode 为 NULL,
 * 如果创建成功, 由实例文件系统调用 VFS 接口分配新的 inode 并进行初始化, mode
 * 为要创建的文件权限
 *
 * @param dir
 * @param dentry
 * @param mode
 * @return int
 */
int ext4_create(struct inode* dir, struct dentry* dentry, int mode) {
    char* name = dentry_name(dentry);
    int err = 0;
    if (ext4_badname(name)) { return -1; }
    bool is_dir = false;
    // 创建一个新的 inode
    struct inode* inode = ext4_new_inode(dir, mode, name, is_dir);
    // TODO: 上面这个函数有时候会返回一个负的错误码，需要进一步处理，这里简单判断一下
    if ((u32)inode > 0) {
        dentry->d_inode = inode;
        // 把 dentry 和 inode 对应的文件或目录添加到父目录的 ext4_dir_entry_2 中
        err = ext4_add_entry(dentry, dir);
        ext4_sync_inode(dir);
        ext4_sync_inode(inode);
    }
    return err;
}

/**
 * @brief 创建一个新的目录
 *
 * @param dir     父目录的 inode 对象
 * @param dentry  要创建的目录项
 * @param mode    新目录的权限模式
 * @return int    成功返回 0，失败返回错误码
 *
 * @note
 * - 检查父目录的链接数是否达到最大值。
 * - 检查目录名是否有效。
 * - 分配一个新的 inode 并与目录项相关联。
 * - 将目录项添加到父目录中，并同步父目录和新目录的 inode。
 */
int ext4_mkdir(struct inode* dir, struct dentry* dentry, int mode) {
    struct inode* inode;
    int err = 0;

    if (dir->i_nlink >= EXT4_LINK_MAX) { return -EMLINK; }

    // 创建一个新的 inode
    char* name = dentry_name(dentry);
    if (ext4_badname(name)) { return -1; }
    bool is_dir = false;
    is_dir = true;
    inode = ext4_new_inode(dir, mode, name, is_dir);
    if ((u32)inode > 0) {
        dentry->d_inode = inode;
        // 把 dentry 和 inode 对应的文件或目录添加到父目录的 ext4_dir_entry_2 中
        err = ext4_add_entry(dentry, dir);
        ext4_sync_inode(dir);
        ext4_sync_inode(inode);
    }
    return err;
}

/**
 * @brief 删除文件
 *
 * @param dir     父目录的 inode 对象
 * @param dentry  要删除的目录项
 * @return int    成功返回 0，失败返回错误码
 *
 * @note
 * - 调用 `ext4_unlink_ino` 执行具体的删除操作。
 */
int ext4_unlink(struct inode* dir, struct dentry* dentry) {
    return ext4_unlink_ino(dir, dentry);
    return 0;
}

/**
 * @brief 查找目录中的文件或子目录
 *
 * @param dir       要查找的父目录的 inode 对象
 * @param filename  要查找的文件或目录名
 * @return struct dentry*  成功返回匹配的 dentry，失败返回 NULL
 *
 * @note
 * - 检查给定的 inode 是否为目录。
 * - 遍历目录的所有数据块，逐个块读取目录项。
 * - 如果找到匹配的文件名，返回对应的 dentry；否则返回 NULL。
 */
struct dentry* ext4_lookup(struct inode* dir, const char* filename) {
    struct super_block* sb = dir->i_sb;
    struct ext4_inode_info* ei = EXT_INODE(dir);
    struct ext4_extent_header* eh = (struct ext4_extent_header*)ei->i_block;
    buf_head* bh = NULL;
    struct inode* inode = NULL;

    // 检查是否为目录
    if (dir->i_type != I_DIRECTORY) { return NULL; }
    if (dir->i_type != I_DIRECTORY) { return NULL; }

    // 遍历目录的所有数据块
    for (int i = 1; i <= eh->eh_entries; i++) {
        struct ext4_extent* ex = (struct ext4_extent*)(((u8*)ei->i_block) + i * 12);
        u32 start_block = ex->ee_start_lo;
        u32 block_len = ex->ee_len;

        // 读取每个数据块
        for (u32 j = 0; j < block_len; j++) {
            bh = bread(sb->sb_dev, start_block + j);
            // 每一块的前24个字节为保留信息 来标识这个块所有者(inode)等相关信息
            struct ext4_dir_entry_2* de = (struct ext4_dir_entry_2*)((u8*)bh->buffer + 24);

            // 遍历块中的所有目录项
            while ((char*)de < (char*)bh->buffer + sb->sb_blocksize) {
                // 跳过无效的目录项
                if (de->inode == 0 && de->rec_len == 0) {
                    brelse(bh);
                    return NULL;
                }

                char* name = (char*)de + 8;
                // 比较文件名
                if (de->name_len == strlen(filename) && !strncmp(name, filename, de->name_len)) {
                    // 找到匹配的目录项
                    inode = vfs_get_inode(sb, de->inode);
                    brelse(bh);
                    return vfs_new_dentry(filename, inode);
                }

                // 移动到下一个目录项
                de = EXT4_NEXT_DIR_ENTRY_2(de);
            }
            brelse(bh);
        }
    }

    return NULL;
}

/**
 * @brief 删除指定的 inode 并释放其相关资源
 *
 * @param inode  要删除的 inode 对象
 * @return void
 *
 * @note
 * - 计算 inode 所在的块组和偏移位置。
 * - 读取块组描述符，更新 inode 位图，清除该 inode 在位图中的对应位。
 * - 更新块组描述符中的空闲 inode 计数。
 * - 更新超级块中的空闲 inode 计数。
 * - 如果 inode 是目录，还需更新块组描述符中的目录计数。
 * - 调用 `ext4_free_blocks` 释放 inode 所关联的所有数据块。
 * - 清空 inode 的元数据，包括大小、权限模式、类型和链接计数等。
 */
void ext4_delete_inode(struct inode* inode) {
    struct super_block* sb = inode->i_sb;
    struct ext4_sb_info* sbi = EXT_SB(sb);
    buf_head* bh = NULL;

    // 计算inode所在的块组
    u32 group = (inode->i_no - 1) / sbi->s_inodes_per_group;
    u32 offset = (inode->i_no - 1) % sbi->s_inodes_per_group;
    u32 bit_offset = offset % 8;
    u32 byte_offset = offset / 8;

    // 读取块组描述符
    u32 gd_block = sbi->s_first_data_block + 1;
    struct ext4_group_desc* gdp;
    bh = bread(sb->sb_dev, gd_block);
    gdp = (struct ext4_group_desc*)bh->buffer + group;

    // 更新inode位图
    u32 imap_block = gdp->bg_inode_bitmap_lo;
    brelse(bh);

    bh = bread(sb->sb_dev, imap_block);
    u8* bitmap = (u8*)bh->buffer;
    bitmap[byte_offset] &= ~(1 << bit_offset); // 清除位图中对应的位
    mark_buff_dirty(bh);
    brelse(bh);

    // 更新块组描述符中的空闲inode计数
    bh = bread(sb->sb_dev, gd_block);
    gdp = (struct ext4_group_desc*)bh->buffer + group;
    u16 free_inodes_count = gdp->bg_free_inodes_count_lo;
    free_inodes_count++;
    gdp->bg_free_inodes_count_lo = free_inodes_count;
    mark_buff_dirty(bh);
    brelse(bh);

    // 更新超级块中的空闲inode计数
    sbi->s_free_inodes_count++;

    // 如果是目录，更新块组描述符中的目录计数
    if (inode->i_type == I_DIRECTORY) {
        bh = bread(sb->sb_dev, gd_block);
        gdp = (struct ext4_group_desc*)bh->buffer + group;
        u16 used_dirs_count = gdp->bg_used_dirs_count_lo;
        if (used_dirs_count > 0) {
            used_dirs_count--;
            gdp->bg_used_dirs_count_lo = used_dirs_count;
            mark_buff_dirty(bh);
        }
        brelse(bh);
    }

    // 释放所有关联的数据块 本质就是将块号的相应的块位图的位置 置为0
    // TODO: 这里假定没有用到 extent_idx 后续了解了 extent_idx的机制后 要加上
    ext4_free_blocks(inode);

    // 清空inode数据
    inode->i_size = 0;
    inode->i_mode = 0;
    inode->i_type = 0;
    inode->i_nlink = 0;
}

void ext4_put_inode(struct inode* inode) {
    UNUSED(inode);
}

/**
 * @brief 同步 inode 到磁盘
 *
 * @param inode  要同步的 inode 对象
 * @return int   返回同步操作的结果，成功时返回 0，失败返回错误码
 *
 * @note
 * - 将 inode 的元数据（如权限、大小、时间戳等）刷新到磁盘。
 * - 确保 inode 所关联的数据块和块位图在磁盘上的状态一致。
 */
int ext4_sync_inode(struct inode* inode) {
    // 需要用到的变量
    struct ext4_inode* pinode;
    struct super_block* sb = inode->i_sb;
    struct ext4_sb_info* sbi = EXT_SB(sb);
    struct ext4_inode_info* ei = EXT_INODE(inode);
    buf_head* bh = NULL;

    // 计算inode所在的块组和块
    u32 i_group = (inode->i_no - 1) / sbi->s_inodes_per_group;
    u32 i_offset = (inode->i_no - 1) % sbi->s_inodes_per_group;

    // 读取块组描述符
    u32 gd_block = sbi->s_first_data_block + 1; // GDT紧跟在超级块后面
    struct ext4_group_desc* gdp;
    bh = bread(sb->sb_dev, gd_block);
    if (!bh) { return -1; }
    gdp = (struct ext4_group_desc*)bh->buffer;

    // 获取指定 inode 所在的块号
    u32 inode_table = gdp[i_group].bg_inode_table_lo;
    u32 i_node_block = inode_table + i_offset * sbi->s_inode_size / sb->sb_blocksize;
    brelse(bh);

    // 读取 inode 的信息
    bh = bread(sb->sb_dev, i_node_block);
    if (!bh) { return -1; }
    pinode = (struct ext4_inode*)((char*)bh->buffer + i_offset * sbi->s_inode_size);

    // 替换新的有效数据，这里 pinode 是 ext4_inode
    pinode->i_mode = ((inode->i_mode & I_R) ? S_IROTH | S_IRGRP | S_IRUSR : 0) |
                     ((inode->i_mode & I_W) ? S_IWUSR : 0) |
                     ((inode->i_mode & I_X) ? S_IXOTH | S_IXUSR | S_IXGRP : 0);
    pinode->i_mode = pinode->i_mode | inode->i_type;
    // pinode->i_mode = (inode->i_type == I_DIRECTORY) ? 0x41ed : 0x81a4;
    // pinode->i_size_lo = (u32)(inode->i_size & 0xFFFFFFFF);
    // pinode->i_size_high = (u32)(inode->i_size >> 32);
    pinode->i_atime = inode->i_atime;
    pinode->i_mtime = inode->i_mtime;
    pinode->i_flags = EXT4_EXTENTS_FL;
    pinode->i_blocks_lo = ei->i_blocks_lo;
    pinode->i_dtime = ei->i_dtime;
    pinode->i_size_lo = ei->i_size;
    pinode->i_links_count = ei->i_links_count;

    struct ext4_extent_header* new_ext4_eheader =
        (struct ext4_extent_header*)EXT_INODE(inode)->i_block;
    struct ext4_extent_header* pext4_eheader = (struct ext4_extent_header*)(pinode->i_block);

    pext4_eheader->eh_depth = new_ext4_eheader->eh_depth;
    pext4_eheader->eh_entries = new_ext4_eheader->eh_entries;
    pext4_eheader->eh_generation = new_ext4_eheader->eh_generation;
    pext4_eheader->eh_magic = new_ext4_eheader->eh_magic;
    pext4_eheader->eh_max = new_ext4_eheader->eh_max;

    for (int i = 0; i < new_ext4_eheader->eh_entries; i++) {
        struct ext4_extent* new_ext4_extent =
            (struct ext4_extent*)((char*)(EXT_INODE(inode)->i_block) + 12 + i * 12);
        struct ext4_extent* pre_ext4_extent =
            (struct ext4_extent*)((char*)pinode->i_block + 12 + i * 12);
        pre_ext4_extent->ee_block = new_ext4_extent->ee_block;
        pre_ext4_extent->ee_len = new_ext4_extent->ee_len;
        pre_ext4_extent->ee_start_hi = new_ext4_extent->ee_start_hi;
        pre_ext4_extent->ee_start_lo = new_ext4_extent->ee_start_lo;
    }

    mark_buff_dirty(bh);
    brelse(bh);

    return 0;
}

/**
 * @brief 从磁盘读取 inode 的元数据
 *
 * @param inode  要读取的 inode 对象
 * @return void
 *
 * @note
 * - 从磁盘加载 inode 的元数据到内存。
 * - 读取 inode 的相关信息（如权限、大小、时间戳等）并将其填充到 vfs `inode` 结构中。
 * - 该函数确保 inode 从磁盘的最新状态被加载到内存，以供后续操作使用。
 */
void ext4_read_inode(struct inode* inode) {
    struct super_block* sb = inode->i_sb;
    struct ext4_sb_info* sbi = EXT_SB(sb);
    buf_head* bh = NULL;

    // 计算inode所在的块组和块
    u32 i_group = (inode->i_no - 1) / sbi->s_inodes_per_group;
    u32 i_offset = (inode->i_no - 1) % sbi->s_inodes_per_group;

    // 读取块组描述符
    u32 gd_block = sbi->s_first_data_block + 1; // GDT紧跟在超级块后面
    struct ext4_group_desc* gdp;
    bh = bread(sb->sb_dev, gd_block);
    gdp = (struct ext4_group_desc*)bh->buffer;

    // NOTE: 根据EXT4的 gd 定义 应该是根据两个 u32 共同得到 inode_table
    // 这样的话 inode_table 的块号的类型应该是 u64, 但是 bread 函数传参的时候只接受 u32
    // 的块号 这里姑且认为磁盘没那么大 分不到那么大的块号 只用低 32 位
    u32 inode_table = gdp[i_group].bg_inode_table_lo;
    u32 i_node_block = inode_table + i_offset * sbi->s_inode_size / sb->sb_blocksize;
    brelse(bh);

    // 读取inode数据
    bh = bread(sb->sb_dev, i_node_block);

    struct ext4_inode* inode_info =
        (struct ext4_inode*)((char*)bh->buffer + i_offset * sbi->s_inode_size);

    if (inode->i_no == EXT_ROOT_INO) {
        inode->i_mode = I_RWX;
        inode->i_type = I_DIRECTORY;
        inode->i_crtime = 0;
        inode->i_mtime = 0;
        inode->i_atime = 0;
    } else {
        // 这里的 imode 是 ext4 inode 的 i_mode
        u16 imode = inode_info->i_mode;
        inode->i_mode =
            (imode & S_IROTH ? I_R : 0) | (imode & S_IWOTH ? I_W : 0) | (imode & S_IXOTH ? I_X : 0);
        inode->i_type = (imode & S_IFDIR) ? I_DIRECTORY : I_REGULAR;
        inode->i_size = inode_info->i_size_lo | ((u64)inode_info->i_size_high << 32);
        inode->i_crtime = inode_info->i_crtime;
        inode->i_mtime = inode_info->i_mtime;
        inode->i_atime = inode_info->i_atime;
    }
    for (int i = 0; i < EXT4_N_BLOCKS; i++) {
        EXT_INODE(inode)->i_block[i] = inode_info->i_block[i];
    }
    EXT_INODE(inode)->i_bg_block = i_group;
    inode->i_op = &ext4_inode_ops;
    inode->i_fop = &ext4_file_ops;

    brelse(bh);
}

/**
 * @brief 填充超级块信息，mount时会进行调用
 *
 * @param sb   指向超级块的指针，存储文件系统的全局信息
 * @param dev  设备号，指定超级块的存储设备
 * @return int 返回操作结果，成功时返回 0，失败返回错误码
 *
 * @note
 * - 该函数从指定设备读取超级块的数据，并填充到 `sb` 结构中。
 * - 超级块包含了文件系统的全局信息，如块大小、inode 数量、空闲块和 inode 的计数等。
 */
int ext4_fill_superblock(struct super_block* sb, int dev) {
    buf_head* bh = bread(dev, 0);

    struct ext4_sb_info* sbi = EXT_SB(sb);

    sbi->s_inodes_count = *(u32*)(bh->buffer + 1024);
    sbi->s_blocks_count_lo = *(u32*)(bh->buffer + 1028);

    sbi->s_free_blocks_count_lo = *(u32*)(bh->buffer + 1036);
    sbi->s_free_inodes_count = *(u32*)(bh->buffer + 1040);

    sbi->s_first_data_block = *(u32*)(bh->buffer + 1044);
    sbi->s_log_block_size = *(u32*)(bh->buffer + 1048);
    sbi->s_log_cluster_size = *(u32*)(bh->buffer + 1052);
    sbi->s_blocks_per_group = *(u32*)(bh->buffer + 1056);

    sbi->s_inodes_per_group = *(u32*)(bh->buffer + 1064);
    sbi->s_inode_size = *(u16*)(bh->buffer + 1112);
    sbi->s_desc_size = *(u16*)(bh->buffer + 1024 + 0xfe);

    sbi->s_magic = *(u16*)(bh->buffer + 1080);
    sbi->s_block_group_nr = *(u16*)(bh->buffer + 1114);
    sbi->s_first_ino = *(u32*)(bh->buffer + 1024 + 0x54);
    sbi->s_mtime = sbi->s_wtime = current_timestamp;

    sb->sb_dev = dev;
    sb->fs_type = FS_TYPE_EXT4;
    sb->sb_op = &ext4_sb_ops;
    sb->sb_blocksize = SZ_1K * ext_get_blocksize(sbi->s_log_block_size);

    struct ext4_group_desc* desc = NULL;
    struct buf_head* group_desc_bh_handle = NULL;
    desc = ext4_get_group_desc(sb, 0, &group_desc_bh_handle);
    u32 inode_table = desc->bg_inode_table_lo;
    brelse(group_desc_bh_handle);

    // 读取根目录的inode，前 11 个保留的 inode 按顺序存储
    struct ext4_inode* pext4_inode = NULL;
    struct ext4_inode_info* ei = NULL;
    struct buf_head* inode_bh = bread(dev, inode_table);
    pext4_inode = (struct ext4_inode*)((u8*)inode_bh->buffer + (2 - 1) * sbi->s_inode_size);

    struct inode* ext4_root = vfs_get_inode(sb, 2);
    ei = EXT_INODE(ext4_root);
    ei->i_blocks_lo = pext4_inode->i_blocks_lo;
    ei->i_size = pext4_inode->i_size_lo;
    ei->i_links_count = pext4_inode->i_links_count;
    ext4_root->i_dev = dev;
    ext4_root->i_sb = sb;
    ext4_root->i_type = I_DIRECTORY;

    sb->sb_root = vfs_new_dentry("/", ext4_root);
    sb->sb_root->d_op = &ext4_dentry_ops;
    brelse(bh);
    spinlock_init(&sb->lock, "EXT");

    return 0;
}

void ext4_query_size_info(struct fs_size_info* info) {
    info->sb_size = sizeof(struct ext4_sb_info);
    info->inode_size = sizeof(struct ext4_inode_info);
}

struct superblock_operations ext4_sb_ops = {
    .query_size_info = ext4_query_size_info,
    .fill_superblock = ext4_fill_superblock,
    .read_inode = ext4_read_inode,
    .sync_inode = ext4_sync_inode,
    .put_inode = ext4_put_inode,
    .delete_inode = ext4_delete_inode,
};

struct inode_operations ext4_inode_ops = {
    .lookup = ext4_lookup,
    .create = ext4_create,
    .unlink = ext4_unlink,
    .mkdir = ext4_mkdir,
    .rmdir = ext4_rmdir,
    .get_block = ext4_get_block,
};

struct dentry_operations ext4_dentry_ops = {
    .compare = stricmp,
};

struct file_operations ext4_file_ops = {
    .read = generic_file_read,
    .write = generic_file_write,
    .fsync = generic_file_fsync,
    .readdir = ext4_readdir,
};
