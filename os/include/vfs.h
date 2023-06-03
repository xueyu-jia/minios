/**********************************************************
*	vfs.h       //added by mingxuan 2019-5-17
***********************************************************/

//#define NR_DEV 10
#define NR_FS 16		//modified by mingxuan 2020-10-18
#define DEV_NAME_LEN 15
//#define NR_fs 3
#define NR_FS_OP 3		//modified by mingxuan 2020-10-18
#define NR_SB_OP 2		//added by mingxuan 2020-10-30

//#define FILE_MAX_LEN 512*4	//最大长度为4个扇区
#define FILE_MAX_LEN 512*16		//最大长度为16个扇区(8KB)

/* //deleted by mingxuan 2020-10-18
//设备表	
struct device{
    char * dev_name; 			//设备名
    struct file_op * op;          //指向操作表的一项
    int  dev_num;                //设备号
};
*/
// Replace struct device, added by mingxuan 2020-10-18
struct vfs{
    char * fs_name; 			//设备名
    struct file_op * op;        //指向操作表的一项
    //int  dev_num;             //设备号	//deleted by mingxuan 2020-10-29

	struct super_block *sb;		//added by mingxuan 2020-10-29
	struct sb_op *s_op;			//added by mingxuan 2020-10-29
	int used;                   //added by ran
};

PUBLIC int sys_open();
PUBLIC int sys_close();
PUBLIC int sys_read();
PUBLIC int sys_write();
PUBLIC int sys_lseek();
PUBLIC int sys_unlink();
PUBLIC int sys_create();
PUBLIC int sys_delete();
PUBLIC int sys_opendir();
PUBLIC int sys_createdir();
PUBLIC int sys_deletedir();
PUBLIC int sys_readdir();
PUBLIC int sys_chdir(); //added by ran
PUBLIC int sys_getcwd(); //added by ran

PUBLIC int do_vopen(const char *path, int flags);
PUBLIC int do_vclose(int fd);
PUBLIC int do_vread(int fd, char *buf, int count);
PUBLIC int do_vwrite(int fd, const char *buf, int count);
PUBLIC int do_vunlink(const char *path);
PUBLIC int do_vlseek(int fd, int offset, int whence);
PUBLIC int do_vcreate(char *pathname);
PUBLIC int do_vdelete(char *path);
PUBLIC int do_vopendir(char *dirname);
PUBLIC int do_vcreatedir(char *dirname);
PUBLIC int do_vdeletedir(char *dirname);
PUBLIC int do_vchdir(const char *path); //added by ran
//PUBLIC char* do_vgetcwd(char *buf, int size); //added by ran
PUBLIC int do_vgetcwd(char *buf, int size); //modified by mingxuan 2021-8-15

PUBLIC int set_vfstable(u32 device, char *target);
PUBLIC struct vfs* vfs_alloc_vfs_entity();
PUBLIC int get_index(char path[]);
PUBLIC void init_vfs();
int sys_CreateFile();
int sys_DeleteFile();
int sys_OpenFile();
int sys_CloseFile();
int sys_WriteFile();
int sys_ReadFile();
int sys_OpenDir();
int sys_CreateDir();
int sys_DeleteDir();
int sys_ListDir();

//文件系统的操作函数
//modified by sundong 2023.5.19 部分接口中添加了superblock参数
struct file_op{
	int (*create)   (struct super_block *,const char*);
	int (*open)    (struct super_block *,const char* ,int);
	int (*close)   (int);
	int (*read)    (int,void * ,int);
	int (*write)   (int,const void* ,int);
	int (*lseek)   (int,int,int);
	int (*unlink)  (struct super_block *,const char*);
    int (*delete) (struct super_block *,const char*);
	int (*opendir) (struct super_block *,const char *);
	int (*createdir) (struct super_block *,const char *);
	int (*deletedir) (struct super_block *,const char *);
	int (*readdir) (struct super_block *,char*, int*, char*);
	int (*chdir) (struct super_block *,const char*); //added by ran
	//int tag;
	// union {
	// 	struct {
    // 		int (*create)   (const char*);
	// 		int (*open)    (const char* ,int);
	// 		int (*close)   (int);
	// 		int (*read)    (int,void * ,int);
	// 		int (*write)   (int ,const void* ,int);
	// 		int (*lseek)   (int ,int ,int);
	// 		int (*unlink)  (const char*);
    // 		int (*delete) (const char*);
	// 		int (*opendir) (const char *);
	// 		int (*createdir) (const char *);
	// 		int (*deletedir) (const char *);
	// 		//int (*chdir) (const char*); //added by ran
	// 	};
	// 	struct {
	// 		int (*CreateFile)(SUPER_BLOCK*,PCHAR);
	// 		int (*OpenFile)(SUPER_BLOCK*,PCHAR,UINT);
	// 		int (*CloseFile)(SUPER_BLOCK*,int);
	// 		int (*ReadFile)(SUPER_BLOCK*,int,BYTE[],DWORD);
	// 		int (*WriteFile)(SUPER_BLOCK*,int,BYTE[],DWORD);
	// 		int (*LSeek)(int,int,int);
	// 		int (*DeleteFile)(SUPER_BLOCK*,PCHAR);
	// 		int (*OpenDir)(PCHAR);
	// 		int (*CreateDir)(SUPER_BLOCK*,PCHAR);
	// 		int (*DeleteDir)(SUPER_BLOCK*,PCHAR);
	// 		int (*ChangeDir)(SUPER_BLOCK*,PCHAR);
	// 		int (*ReadDir)(SUPER_BLOCK*,PCHAR,DWORD[], PCHAR);
	// 	};
	// };
};

//added by mingxuan 2020-10-29
struct sb_op{
	void (*read_super_block) (int);
	struct super_block* (*get_super_block) (int);
};