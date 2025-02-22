#include <type.h>
#include <disk.h>
#include <fat32.h>
#include <string.h>
#include <assert.h>

struct BPB bpb;
u32 fat_start_sec;
u32 data_start_sec;
u32 elf_off;
u32 fat_now_sec;
u32 elf_first;

void fat32_init();

int fat32_open_file(char *filename);
int fat32_read(u32 offset, u32 lenth, void *buf);

// 一个简单地文件描述符
struct fat32_fd {
    char *filename;
    u32 first_clus;
    u32 fat_start_sec;  // fat表起始扇区号
    u32 data_start_sec; // data起始扇区号
};
struct fat32_fd elf_fd;

struct BPB bpb;
u32 fat_start_sec;
u32 data_start_sec;
u32 elf_off;
u32 fat_now_sec;
u32 elf_first;

/*
获取下一个扇区号
扇区号 = 簇号*4/每个扇区的字节数 + （隐藏扇区数 +
保留扇区数）【相当于加上fat起始扇区号】 扇区偏移 = 簇号*4%每个扇区的字节数
*/
static u32 get_next_clus_number(u32 current_clus) {
    u32 sec = current_clus * 4 / SECTSIZE; // 需要乘4是因为每个簇号大小为4字节
    u32 off = current_clus * 4 % SECTSIZE;
    readsect((void *)BUF_ADDR, boot_part_lba + elf_fd.fat_start_sec + sec);
    return *(u32 *)(BUF_ADDR + off); // 返回下一个簇的簇号
}

// 将文件名转为大写 .转为"  "
static void fat32_uppercase_filename(char *src, char *dst) {
    char *p = src;
    int index = 0;
    int len = strlen(src) + 1;
    while (len) {
        if (*p >= 'a' && *p <= 'z') {
            dst[index++] = *p - 32;
        } else if (*p == '.') {
            //"."转化为两个空格
            dst[index++] = ' ';
            dst[index++] = ' ';
        } else {
            dst[index++] = *p;
            if (*p == '\0') return;
        }
        p++;
        len--;
    }
}
/*
 * 读入簇号对应的数据区的所有数据
 */
static void *read_cluster(void *dst, u32 current_clus) {
    // 计算簇号对应的数据区的簇号 = （簇号-2）*每个簇的扇区数+数据区起始扇区数
    int data_clus = current_clus - bpb.BPB_RootClus;
    int clus_start_sect = data_clus * bpb.BPB_SecPerClus + elf_fd.data_start_sec;
    // 根据每个簇包含的扇区数量将数据读入目的地址
    // for (int i = 0; i < bpb.BPB_SecPerClus; i++, dst += SECTSIZE)
    // readsect(dst, boot_part_lba+clus_start_sect + i);
    readsects(dst, boot_part_lba + clus_start_sect, bpb.BPB_SecPerClus);
    return dst + bpb.BPB_SecPerClus * SECTSIZE;
}

void fat32_init() {
    readsect((void *)&bpb, boot_part_lba);
    assert(bpb.BPB_BytsPerSec == SECTSIZE);
    // 计算fat表起始扇区号
    elf_fd.fat_start_sec = bpb.BPB_RsvdSecCnt;
    // 计算data起始扇区号
    elf_fd.data_start_sec = elf_fd.fat_start_sec + bpb.BPB_FATSz32 * bpb.BPB_NumFATs;
    elf_fd.first_clus = 0;
}

/*
 * @brief
 * @param
 * @return 0 if faile or the first clus number ot the file if success
 */
u32 fat32_find_file(char *filename) {
    u32 file_clus = 0;
    char filename_upper[strlen(filename) + 1];
    fat32_uppercase_filename(filename, filename_upper);
    filename = filename_upper;
    u32 root_clus = bpb.BPB_RootClus; // 根目录簇号
    while (root_clus < 0x0FFFFFF8) {
        void *read_end = read_cluster((void *)BUF_ADDR,
                                      root_clus); // 将根目录区的所有数据读入缓冲区，返回最后的地址
        // 遍历根目录所有文件，寻找kernel
        for (struct Directory_Entry *p = (void *)BUF_ADDR; (void *)p < read_end;
             p++) { // 结构体中存储文件描述信息
            if (strncmp(p->DIR_Name, filename, strlen(filename)) ==
                0) { // 比较字符串，判断是否找到kernel.bin
                file_clus =
                    (u32)p->DIR_FstClusHI << 16 | p->DIR_FstClusLO; // 记录kernel.bin所在的扇区号
                break;
            }
        }
        if (file_clus != 0) break;

        root_clus = get_next_clus_number(root_clus); // 循环寻找下一个簇
    }
    return file_clus;
}

/*
 * @brief 得到文件的第i个簇的簇号
 */
u32 fat32_find_clus_i(u32 first_clus, u32 clus_i) {
    u32 current_clus = first_clus;
    for (u32 i = 0; i < clus_i; ++i) { current_clus = get_next_clus_number(current_clus); }
    return current_clus;
}

/*
 * @brief 从文件的特定偏移量开始读数据
 * @param offset:在文件中的偏移量
 * @param buf必须大于等于lenth
 * @param lenth:读数据的长度
 * @note 请先用fat32_file_open()打开文件
 * 			lenth<=0 返回true
 */
int fat32_read(u32 offset, u32 lenth, void *buf) {
    if (lenth == 0) { return true; }

    ssize_t len = lenth;

    u32 clus_size = bpb.BPB_SecPerClus * SECTSIZE;

    u32 clus_i = offset / clus_size;
    u32 offset_in_clus = offset % clus_size;
    u32 current_clus = fat32_find_clus_i(elf_fd.first_clus, clus_i);
    ssize_t first_read_lenth = MIN(clus_size - offset_in_clus, len);

    read_cluster((void *)BUF_ADDR, current_clus);
    memcpy(buf, (void *)(BUF_ADDR + offset_in_clus), first_read_lenth);
    current_clus = get_next_clus_number(current_clus);
    len -= first_read_lenth;
    buf += first_read_lenth;

    while (len > 0 && current_clus < 0x0ffffff8) {
        const ssize_t read_len = MIN(clus_size, len);
        read_cluster((void *)BUF_ADDR, current_clus);
        memcpy(buf, (void *)BUF_ADDR, read_len);
        current_clus = get_next_clus_number(current_clus);
        buf += read_len;
        len -= read_len;
    }

    return true;
}

/*
 * @brief 通过文件名打开一个文件，维持一个简单的FD
 * @return 0 if faile or 1 for success
 */
int fat32_open_file(char *filename) {
    elf_fd.filename = filename;
    elf_fd.first_clus = fat32_find_file(filename);
    return (elf_fd.first_clus == 0 ? FALSE : TRUE);
}
