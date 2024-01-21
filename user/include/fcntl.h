#ifndef FCNTL_H
#define FCNTL_H

#define MAX_DNAME_LEN 32
#define	MAX_PATH	256
// octol
#define	O_CREAT		004
#define O_RDONLY	000
#define O_WRONLY	001
#define	O_RDWR		002
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

// permission
#define I_RWX	7
#define I_R	1
#define I_W	2
#define I_X	4

/* inode type (octal, lower 12 bits reserved) */
#define I_TYPE_MASK     0170000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY     0040000
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE	0010000

struct dirent{
	int d_ino;
	char d_name[MAX_DNAME_LEN];
};

struct dirstream;
typedef struct dirstream DIR;

#endif