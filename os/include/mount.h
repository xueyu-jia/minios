#include "type.h"

#define MAX_mnt_table_length   10

extern int mount_open(char *pathname, int flags);

// extern void update_mnttable();
typedef struct
{
    char filename[16];
    u32 vfs_index;
    u32 dev;
    u32 used;//free=0
    /* data */
}mount_table;