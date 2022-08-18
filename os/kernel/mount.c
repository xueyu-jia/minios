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
#include "../include/mount.h"

PUBLIC void update_mnttable();
PUBLIC mount_table mnt_table[MAX_mnt_table_length];
extern struct vfs  vfs_table[NR_FS];

PUBLIC int do_mount(const char *source, const char *target,
   const char *filesystemtype, unsigned long mountflags, const void *data);


PUBLIC int do_unmount(const char *target);

//oprate 
PUBLIC int mount_open(char *pathname);

PUBLIC void create_mountpoint(const char *pathname);

PUBLIC int alloc_mnttable(char *target);

PUBLIC void write_mnttable(int index, char *target);

PUBLIC void free_mnttable(char *target);





/*======================================================================*
                              sys_* 系列函数
 *======================================================================*/

PUBLIC int sys_mount(void *uesp)
{
    // return do_mount(get_arg(uesp, 1), get_arg(uesp, 2), get_arg(uesp, 3), get_arg(uesp, 4), get_arg(uesp, 5));
}

PUBLIC int sys_umount(void *uesp)
{
    // return do_umount(get_arg(uesp, 1));
}
