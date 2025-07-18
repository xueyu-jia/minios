#include <disk.h>
#include <fat32.h>
#include <fs.h>
#include <terminal.h>
#include <orangefs.h>
#include <type.h>

// 读文件函数指针，根据不同的fs，有不同的实现；
//  int (*read_file)(char* filename, void* buf);
int (*read)(u32 offset, u32 lenth, void *buf);
int (*open_file)(char *filename);

static bool is_fat32() {
    // 判断是否是FAT32的方法：
    // boot扇区的第0x52开始的8的字节的值是否为{4641-5433-3220-2020}("FAT32   ")
    readsect((void *)BUF_ADDR, boot_part_lba);
    int fs_flage;
    fs_flage = *(int *)(BUF_ADDR + 0x52);
    if (fs_flage != 0x33544146) { return FALSE; }
    fs_flage = *(int *)(BUF_ADDR + 0x56);
    return fs_flage == 0x20202032;
}

static bool is_orangefs() {
    readsect((void *)BUF_ADDR, boot_part_lba + (BLOCK_SIZE / SECT_SIZE));
    super_block *orange_sb = (super_block *)BUF_ADDR;
    return orange_sb->magic == MAGIC_V1; // orangefs的magic为MAGIC_V1 0x111
}

int init_fs() {
    if (false) {
    } else if (is_fat32()) {
        fat32_init();
        read = fat32_read;
        open_file = fat32_open_file;
        return true;
    } else if (is_orangefs()) {
        orangefs_init();
        read = orangefs_read;
        open_file = orangefs_open_file;
        return true;
    }
    return false;
}
