/**********************************************************
*	vfs.c       //added by mingxuan 2019-5-17
***********************************************************/
/*
#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "fs_misc.h"
#include "vfs.h"
#include "fat32.h"
*/
#include "../include/type.h"
#include "../include/const.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/proc.h"
#include "../include/global.h"
#include "../include/proto.h"
#include "../include/fs_const.h"
#include "../include/hd.h"
#include "../include/fs.h"
#include "../include/fs_misc.h"
#include "../include/vfs.h"
#include "../include/fat32.h"
#include "../include/mount.h"
//PRIVATE struct device  device_table[NR_DEV];  //deleted by mingxuan 2020-10-18
//PRIVATE struct vfs  vfs_table[NR_FS];   //modified by mingxuan 2020-10-18
PUBLIC struct vfs  vfs_table[NR_FS]; //modified by ran
extern mount_table mnt_table[MAX_mnt_table_length];
PUBLIC struct file_desc f_desc_table[NR_FILE_DESC];
PUBLIC struct super_block super_block[NR_SUPER_BLOCK]; //added by mingxuan 2020-10-30

//PRIVATE struct file_op f_op_table[NR_fs]; //文件系统操作表
PRIVATE struct file_op f_op_table[NR_FS_OP]; //modified by mingxuan 2020-10-18
PRIVATE struct sb_op   sb_op_table[NR_SB_OP];   //added by mingxuan 2020-10-30

PRIVATE int strcmp(const char *s1, const char *s2);

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
    f_op_table[0].tag = 0;

    // table[1] for orange
    f_op_table[1].open = real_open;
    f_op_table[1].close = real_close;
    f_op_table[1].write = real_write;
    f_op_table[1].lseek = real_lseek;
    f_op_table[1].unlink = real_unlink;
    f_op_table[1].read = real_read;
    f_op_table[1].createdir = real_createdir;
    f_op_table[1].deletedir = real_deletedir;
    f_op_table[1].tag = 0;

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
    f_op_table[2].create = create_adapter;
    f_op_table[2].delete = delete_adapter;
    f_op_table[2].open = open_adapter;
    f_op_table[2].close = close_adapter;
    f_op_table[2].write = write_adapter;
    f_op_table[2].read = read_adapter;
    f_op_table[2].lseek = LSeek;
    f_op_table[2].opendir = opendir_adapter;
    f_op_table[2].createdir = createdir_adapter;
    f_op_table[2].deletedir = deletedir_adapter;
    f_op_table[2].readdir = readdir_adapter;
    f_op_table[2].chdir = chdir_adapter;
    f_op_table[2].tag = 1;

}

//added by mingxuan 2020-10-30
PUBLIC void init_super_block_table(){
    struct super_block * sb = super_block;						//deleted by mingxuan 2020-10-30

    //super_block[0] is tty0, super_block[1] is tty1, uper_block[2] is tty2
    for(; sb < &super_block[3]; sb++) {
        sb->sb_dev =  DEV_CHAR_TTY;
        sb->fs_type = TTY_FS_TYPE;
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
    for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)			//deleted by mingxuan 2020-10-30
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

    //fat32 and tty
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
    vfs_table[0].sb = &super_block[0];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[0].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30
    vfs_table[0].used = 1;

    // tty1
    //device_table[1].dev_name="dev_tty1";
    //device_table[1].op =&f_op_table[0];
    vfs_table[1].fs_name = "dev_tty1"; //modifed by mingxuan 2020-10-18
    vfs_table[1].op = &f_op_table[0];
    vfs_table[1].sb = &super_block[1];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[1].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30
    vfs_table[1].used = 1;

    // tty2
    //device_table[2].dev_name="dev_tty2";
    //device_table[2].op=&f_op_table[0];
    vfs_table[2].fs_name = "dev_tty2"; //modifed by mingxuan 2020-10-18
    vfs_table[2].op = &f_op_table[0];
    vfs_table[2].sb = &super_block[2];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[2].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30
    vfs_table[2].used = 1;

    // orangefs
    vfs_table[3].fs_name = "orange";
    vfs_table[3].op = &f_op_table[1];
    vfs_table[3].sb = &super_block[3];
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
        vfs_table[i].sb = &super_block[i];
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
    
    
    pvfs->fs_name = (char*)kmalloc(12);
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
        sb_index = init_fat32fs(device);
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

    pvfs->sb = &super_block[sb_index];
    pvfs->used = 1;

    return pvfs - vfs_table;
}


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
//PRIVATE int get_index(char path[]){
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
    return do_vopen(get_arg(1), get_arg(2));
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
PUBLIC int do_vopen(const char *path, int flags) {
    return kern_vopen(path, flags);
}

/*======================================================================*
                              do_v* 系列函数
 *======================================================================*/
//PUBLIC int do_vopen(const char *path, int flags) {
PUBLIC int kern_vopen(const char *path, int flags) {    //modified by mingxuan 2021-8-15

    int pathlen = strlen(path);
    char pathname[MAX_PATH];
//    char pathnamebackup[MAX_PATH];

    strcpy(pathname,path);
/*     pathname[pathlen] = 0;
    strcpy(pathnamebackup,path);
    pathnamebackup[pathlen] = 0; */
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

    if (pvfs->op->tag)
    {
        strcpy(pathname, path);
    }
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

    if (pvfs->op->tag)
    {
        strcpy(pathname, path);
    }
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

    if (pvfs->op->tag)
    {
        strcpy(pathname, path);
    }
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

    if (pvfs->op->tag)
    {
        strcpy(pathname, path);
    }
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


PRIVATE int strcmp(const char * s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = s1;
	const char * p2 = s2;

	for (; *p1 && *p2; p1++,p2++) {
		if (*p1 != *p2) {
			break;
		}
	}

	return (*p1 - *p2);
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