#define MAX_mnt_table_length   10

extern int mount_open(char *pathname);

extern void update_mnttable();
typedef struct
{
    char *filename;
    int vfs_index;
    int flag;//free=0
    /* data */
}mount_table;