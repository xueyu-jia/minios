#pragma once

#include <minios/proc.h>
#include <minios/dev.h>
#include <klib/stdint.h>

#define MBR_PART_TABLE_OFFSET 0x1be

enum {
    MBR_PART_NONE = 0x00,
    MBR_PART_EXT = 0x05,
};

typedef struct mbr_part_ent {
    u8 boot_ind;
    u8 start_head;
    u8 start_sector;
    u8 start_cyl;
    u8 sys_id;
    u8 end_head;
    u8 end_sector;
    u8 end_cyl;
    u32 start_sect;
    u32 nr_sects;
} mbr_part_ent_t;

enum {
    HD_CMD_OPEN,
    HD_CMD_CLOSE,
    HD_CMD_READ,
    HD_CMD_WRITE,
    HD_CMD_IOCTL,
};

typedef struct {
    int type;
    int dev;
    int pid;
    size_t pos;
    size_t cnt;
    void *buf;
} hd_messge_t;

typedef struct hd_cmd {
    u8 features;
    u8 count;
    u8 count_LBA48;
    u8 lba_low;
    u8 lba_low_LBA48;
    u8 lba_mid;
    u8 lba_mid_LBA48;
    u8 lba_high;
    u8 lba_high_LBA48;
    u8 device;
    u8 command;
} hd_cmd_t;

#define HD_DEV_GET_DRIVE(dev) DEV_MAJOR(dev)
#define HD_DEV_GET_INDEX(dev) ((DEV_MINOR(dev) >> 0) & 0xff)
#define HD_DEV_GET_PART(dev) ((DEV_MINOR(dev) >> 8) & 0xff)
#define HD_DEV_MAKE(drv, index, part) DEV_MAKE_ID(drv, (((part) & 0xff) << 8) | ((index) & 0xff))
#define HD_DEV_MAKE_BLK(drv, index) HD_DEV_MAKE(drv, index, 0xff)
#define HD_DEV_IS_BLK(dev) (HD_DEV_GET_PART(dev) == 0xff)
#define HD_DEV_TO_BLK(dev) HD_DEV_MAKE_BLK(HD_DEV_GET_DRIVE(dev), HD_DEV_GET_INDEX(dev))

#define NR_DRIVES 8

#define NR_DRIVE_PARTS 255
#define NR_MBR_PRIM_PART 4
#define NR_MBR_EXT_PART (NR_DRIVE_PARTS - NR_MBR_PRIM_PART)

typedef struct hd_part_info {
    u32 base;
    u32 size;
    int fs_type;
} hd_part_info_t;

typedef struct hd_info {
    int type;
    int index;
    int open_cnt;
    size_t nr_parts;
    hd_part_info_t parts[NR_DRIVE_PARTS + 1];
} hd_info_t;

#define HDQUE_MAXSIZE 64

typedef struct hd_rdwt_info {
    hd_messge_t *msg;
    process_t *proc;
    struct hd_rdwt_info *next;
} hd_rwinfo_t;

typedef struct {
    hd_rwinfo_t *front;
    hd_rwinfo_t *rear;
} hd_queue_t;

extern hd_info_t hd_drvs[NR_DRIVES];

void init_hd();
void hd_open_and_init();

void hd_service();

hd_info_t *hd_lookup(int dev);
hd_part_info_t *hd_lookup_part(int dev);
hd_info_t *hd_get_slot();

int hd_get_fstype(int dev);
int hd_make_part_dev(int drive, int part, int fs_type);
int hd_find_dev_of_fstype(int drive, int fs_type);

void hd_open(int drive);
void hd_close(int drive);
void hd_ioctl(hd_messge_t *p);
void hd_rdwt(hd_messge_t *p);
void hd_rdwt_sched(hd_messge_t *p);

void rw_blocks(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf);
void rw_blocks_sched(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf);

int orangefs_identify(int drive, u32 nr_sect);
int fat32_identify(int drive, u32 nr_sect);
int ext4_identify(int drive, u32 nr_sect);
