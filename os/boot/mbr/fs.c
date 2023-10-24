#include "fs.h"
#include "type.h"
#include "fat32.h"
#include "orangefs.h"
#include "disk.h"
#include "loaderprint.h"
//读文件函数指针，根据不同的fs，有不同的实现；
// int (*read_file)(char* filename, void* buf);
int (*read)(u32 offset, u32 lenth, void *buf);
int (*open_file)(char *filename);

static bool is_fat32(){
    //判断是否是FAT32的方法：
    // boot扇区的第0x52开始的8的字节的值是否为{4641-5433-3220-2020}("FAT32   ")
    readsect(BUF_ADDR,bootPartStartSector);
    int fs_flage;
	fs_flage = *(int*)(BUF_ADDR + 0x52);
	if (fs_flage != 0x33544146)
	{
		return FALSE;
	}
	fs_flage = *(int*)(BUF_ADDR + 0x56);
	return fs_flage == 0x20202032;

}
static bool is_orangefs(){
    readsect(BUF_ADDR,bootPartStartSector+(BLOCK_SIZE/SECT_SIZE));
    super_block* orange_sb =  (super_block*)BUF_ADDR;
    return orange_sb->magic == MAGIC_V1;//orangefs的magic为MAGIC_V1 0x111

}
int init_fs(){
    if(is_fat32()){
        fat32_init();
        // read_file = fat32_read_file;
        read = fat32_read;
        open_file = fat32_open_file;
        return TRUE;
    }
    else if(is_orangefs()){
        orangefs_init();
        // read_file = orangefs_read_file;
        read = orangefs_read;
        open_file = orangefs_open_file;
        return TRUE;
    }
    else return FALSE;

}