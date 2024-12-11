#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "list.h"
typedef u_int32_t u32;
typedef u_int8_t u8;
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define ORANGE_MAGIC 0x111

struct orange_sb {
    u32 magic;             /**< Magic number */
    u32 nr_inodes;         /**< How many inodes */
    u32 nr_blocks;         /**< How many blocks */
    u32 nr_imap_blocks;    /**< How many inode-map blocks */
    u32 nr_smap_blocks;    /**< How many sector-map blocks */
    u32 n_1st_block;       /**< Number of the 1st data block */
    u32 nr_inode_blocks;   /**< How many inode blocks */
    u32 root_inode;        /**< Inode nr of root directory */
    u32 inode_size;        /**< INODE_SIZE */
    u32 inode_isize_off;   /**< Offset of `struct inode::i_size' */
    u32 inode_start_off;   /**< Offset of `struct inode::i_start_sect' */
    u32 dir_ent_size;      /**< DIR_ENTRY_SIZE */
    u32 dir_ent_inode_off; /**< Offset of `struct dir_entry::inode_nr' */
    u32 dir_ent_fname_off; /**< Offset of `struct dir_entry::name' */
};

struct orange_inode {
    u32 i_mode;        /**< Accsess mode */
    u32 i_size;        /**< File size */
    u32 i_start_block; /**< The first block of the data */
    u32 i_nr_blocks;   /**< How many blocks the file occupies */
    u8 _unused[15];    /**< Stuff for alignment */
    u8 i_mnt_index;    /**the index in mnt_table when the inode is mountpoint*/
    // only in orangefsutils mem inode
    u8 deleted; // fuse no interface for delete inode, we use this to mark inode
                // unlinked to free sec

    pthread_mutex_t lock;
};

#define ORANGE_INODE_SIZE 32
#define I_TYPE_MASK 0170000
#define I_REGULAR 0100000
#define I_DIRECTORY 0040000

struct inode_cache {
    int ino;
    struct orange_inode *iptr;
    struct list_node ilist;
};

#define MAX_FILENAME_LEN 12

struct orange_direntry {
    int inode_nr;                /**< inode nr. */
    char name[MAX_FILENAME_LEN]; /**< Filename */
};

#define ORANGE_DIRENT_SIZE 16
#define ORANGE_BLK_SIZE 4096
#define ORANGE_FILE_BLK 256
int orangefs_fillsuper();
void orangefs_free();
struct orange_inode *orangefs_iget(int ino);
void orangefs_syncinode(int ino, struct orange_inode *inode);
void orangefs_iforget_may_drop(int ino, struct orange_inode *inode);
int orangefs_allocinode();
void orangefs_allocsector(struct orange_inode *inode);
int orangefs_find(int parent, const char *name, int *rt_pos);
void orangefs_write_direntry(struct orange_inode *dir, int pos,
                             const char *name, int ino);
int orangefs_add_direntry(int parent, const char *name, int ino);
int orangefs_dir_empty(struct orange_inode *dir);
int orangefs_stat(int ino, struct stat *buf);
