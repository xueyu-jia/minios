extern int (*read_file)(char*,void*);
extern struct FD (*open) (char*);

extern struct FD elf_fd;
int init_fs();
