/*
  Orangefs mount with FUSE
*/

#define FUSE_USE_VERSION 34

#include <fuse_lowlevel.h>
#include <linux/fs.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include "orangefs.h"
#include "orangefs_disk.h"
// #include <fuse_log.h>

static struct orangefs_opts
{
    char *disk;
    int show_help;
} options;

#define OPTION(t, p)                            \
    {                                           \
        t, offsetof(struct orangefs_opts, p), 1 \
    }
static const struct fuse_opt option_spec[] = {
    OPTION("-h", show_help),
    OPTION("--help", show_help),
    FUSE_OPT_END};

void orangefs_init(void *userdata, struct fuse_conn_info *conn)
{
    (void)conn;
    if (disk_open(options.disk) < 0)
    {
        fuse_log(FUSE_LOG_ERR, "orange: fail to open disk\n");
    }
    if (orangefs_fillsuper() < 0)
    {
        fuse_log(FUSE_LOG_ERR, "orange: wrong fs type\n");
    }
}

void orangefs_destory(void *userdata)
{
    orangefs_free();
}

void orangefs_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
    int res = orangefs_find((int)parent, name, NULL);
    if(0 != res) {
        struct fuse_entry_param e;
        memset(&e, 0, sizeof(e));
		e.ino = res;
		e.attr_timeout = 1.0;
		e.entry_timeout = 1.0;
		orangefs_stat(e.ino, &e.attr);
		fuse_reply_entry(req, &e);
    }else {
        fuse_reply_err(req, ENOENT);
    }
}

void orangefs_getattr(fuse_req_t req, fuse_ino_t ino,
                            struct fuse_file_info *fi)
{
    struct stat stbuf;
    memset(&stbuf, 0, sizeof(stbuf));
	if (orangefs_stat(ino, &stbuf) == -1)
		fuse_reply_err(req, ENOENT);
	else
		fuse_reply_attr(req, &stbuf, 1.0);
}

void orangefs_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    int pos;
    int res = orangefs_find((int)parent, name, &pos);
    if(0 != res) {
        struct orange_inode* pin = orangefs_iget(res);
        struct orange_inode* dir = orangefs_iget(parent);
        pin->deleted = 1;
        // fuse_log(FUSE_LOG_DEBUG, "write dirent(%d.%d, %s, %d)\n", parent, pos, " ", 0);
        orangefs_write_direntry(dir, pos, "", 0);
        fuse_reply_err(req, 0);
    }else {
        fuse_reply_err(req, ENOENT);
    }
}

void orangefs_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name){
    int pos;
    int res = orangefs_find((int)parent, name, &pos);
    if(0 != res) {
        struct orange_inode* pin = orangefs_iget(res);
        struct orange_inode* dir = orangefs_iget(parent);
        if(orangefs_dir_empty(pin) == 0){
            pin->deleted = 1;
            // fuse_log(FUSE_LOG_DEBUG, "write dirent(%d.%d, %s, %d)\n", parent, pos, " ", 0);
            orangefs_write_direntry(dir, pos, "", 0);
            fuse_reply_err(req, 0);
        }else {
            fuse_reply_err(req, ENOTEMPTY);
        }
    }else {
        fuse_reply_err(req, ENOENT);
    }
}

void orangefs_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
			fuse_ino_t newparent, const char *newname,
			unsigned int flags)
{
    int old_pos, new_pos, old_ino, new_ino;
    old_ino = orangefs_find(parent, name, &old_pos);
    new_ino = orangefs_find(newparent, newname, &new_pos);
    if(old_ino == 0 || name[0]=='\0' || newname[0]=='\0') {
        fuse_reply_err(req, ENOENT);
    }
    if(old_ino == new_ino){
        fuse_reply_err(req, 0);
    }
    switch (flags)
    {
    case RENAME_NOREPLACE:
        if(new_ino != 0) {
            fuse_reply_err(req, EEXIST);
        }else {
            // fuse_log(FUSE_LOG_DEBUG, "add dirent(%d, %s, %d)\n", newparent, newname, old_ino);
            orangefs_add_direntry(newparent, newname, old_ino);
            struct orange_inode *dir=orangefs_iget(parent);
            // fuse_log(FUSE_LOG_DEBUG, "write dirent(%d.%d, %s, %d)\n", parent, old_pos, " ", 0);
            orangefs_write_direntry(dir, old_pos, "", 0);
        }
        break;
    case RENAME_EXCHANGE:
        if(new_ino != 0) {
            struct orange_inode *newdir=orangefs_iget(newparent);
            orangefs_write_direntry(newdir, new_pos, newname, old_ino);
            struct orange_inode *dir=orangefs_iget(parent);
            orangefs_write_direntry(dir, old_pos, name, new_ino);
        }else {
            fuse_reply_err(req, ENOENT);
        }
        break;
    default:
        if(new_ino != 0) {
            struct orange_inode *newdir=orangefs_iget(newparent);
            struct orange_inode *origin=orangefs_iget(new_ino);
            origin->deleted = 1;
            orangefs_write_direntry(newdir, new_pos, newname, old_ino);
        }else{
            orangefs_add_direntry(newparent, newname, old_ino);
        }
        struct orange_inode *dir=orangefs_iget(parent);
        orangefs_write_direntry(dir, old_pos, "", 0);
        break;
    }
    fuse_reply_err(req, 0);
}

void orangefs_forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup)
{
    if(nlookup == 0){
        struct orange_inode* pin = orangefs_iget(ino);
        // fuse_log(FUSE_LOG_DEBUG, "forget %d\n", ino);
        // if(pin->deleted){
            // fuse_log(FUSE_LOG_DEBUG, "free [%d,%d)\n", pin->i_start_block, pin->i_start_block+pin->i_nr_blocks);
        // }
        orangefs_iforget_may_drop((int)ino, pin);
    }
    fuse_reply_none(req);
}


void orangefs_open(fuse_req_t req, fuse_ino_t ino,
		      struct fuse_file_info *fi)
{
    if (ino <= 0)
        fuse_reply_err(req, ENOENT);
    struct orange_inode* pin = orangefs_iget(ino);
    if (pin == 0)
        fuse_reply_err(req, ENOENT);
    if ((pin->i_mode&I_TYPE_MASK) == I_DIRECTORY)
		fuse_reply_err(req, EISDIR);
	else{
        fi->direct_io = 1;
		fuse_reply_open(req, fi);
    }
}

static int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
			     off_t off, size_t maxsize)
{
	if (off < bufsize)
		return fuse_reply_buf(req, buf + off,
				      min(bufsize - off, maxsize));
	else
		return fuse_reply_buf(req, NULL, 0);
}

void orangefs_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
		      struct fuse_file_info *fi)
{
    struct orange_inode* pin = orangefs_iget(ino);
    if (pin == 0)
        fuse_reply_err(req, ENOENT);
    if ((pin->i_mode&I_TYPE_MASK) == I_DIRECTORY)
		fuse_reply_err(req, EISDIR);
    if (off >= 0) 
    {
        off_t end = min(off + size, pin->i_size);
        char *buf = malloc(size);
        memset(buf, 0, size);
        disk_read(pin->i_start_block, off, end - off, buf);
        fuse_reply_buf(req, buf, end - off);
        free(buf);
    }
}

struct dirbuf {
	char *p;
	size_t size;
};

static void dirbuf_add(fuse_req_t req, struct dirbuf *b, const char *name,
		       fuse_ino_t ino)
{
	struct stat stbuf;
	size_t oldsize = b->size;
	b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
	b->p = (char *) realloc(b->p, b->size);
	memset(&stbuf, 0, sizeof(stbuf));
	stbuf.st_ino = ino;
	fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
			  b->size);
}

void orangefs_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
			 struct fuse_file_info *fi)
{
    if (ino <= 0)
		fuse_reply_err(req, ENOENT);
	else {
        struct orange_inode *dir = orangefs_iget(ino);
        if(dir == NULL) {
            fuse_reply_err(req, ENOENT);
        }
        if((dir->i_mode&I_TYPE_MASK) != I_DIRECTORY) {
            fuse_reply_err(req, ENOTDIR);
        }
		struct dirbuf b;
        struct orange_direntry dirent;
		memset(&b, 0, sizeof(b));
        memset(&dirent, 0, ORANGE_DIRENT_SIZE);
		dirbuf_add(req, &b, ".", ino);
		dirbuf_add(req, &b, "..", ino);
        off_t p = 0;
        for(int blk=dir->i_start_block; 
            blk < dir->i_start_block+dir->i_nr_blocks; blk++)
        {
            for(int pos=0; pos < ORANGE_BLK_SIZE && p + pos < dir->i_size; pos += ORANGE_DIRENT_SIZE) 
            {
                disk_read(blk, pos, ORANGE_DIRENT_SIZE, &dirent);
                if(dirent.inode_nr == 0) {
                    continue;
                }
                dirbuf_add(req, &b, dirent.name, dirent.inode_nr);
            }
            p += ORANGE_BLK_SIZE;
        }
		reply_buf_limited(req, b.p, b.size, off, size);
		free(b.p);
	}
}

void orangefs_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size)
{
    if (ino <= 0)
		fuse_reply_err(req, ENOENT);
	else {
        if(size == 0) {
            fuse_reply_xattr(req, 0);
        }else{
            fuse_reply_buf(req, "", 0);
        }
	}
}

void orangefs_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
		       size_t size, off_t off, struct fuse_file_info *fi)
{
    struct orange_inode* pin = orangefs_iget(ino);
    if (pin == 0)
        fuse_reply_err(req, ENOENT);
    if ((pin->i_mode&I_TYPE_MASK) == I_DIRECTORY)
		fuse_reply_err(req, EISDIR);
    if (pin->i_nr_blocks == 0) {
        orangefs_allocsector(pin);
        orangefs_syncinode((int)ino, pin);
    }
    if (off >= 0 && off + size < pin->i_nr_blocks*ORANGE_BLK_SIZE) 
    {
        off_t end = max(max(off + size, pin->i_size), off);
        // fuse_log(FUSE_LOG_DEBUG, "write:%ld\n", end-off);
        disk_write(pin->i_start_block, off, end - off, (void*)buf);
        if(pin->i_size < end){
            pin->i_size = end;
            orangefs_syncinode(ino, pin);
        }
        // fuse_log(FUSE_LOG_DEBUG, "sync:%ld (size %d)\n", ino, pin->i_size);
        fuse_reply_write(req, end - off);
    }else {
        fuse_reply_err(req, ENOSPC);
    }
}

void orangefs_create(fuse_req_t req, fuse_ino_t parent, const char *name,
			mode_t mode, struct fuse_file_info *fi) 
{
    int ino = orangefs_find(parent, name, NULL);
    if(ino != 0) {
        fuse_reply_err(req, EEXIST);
    }
    ino = orangefs_allocinode();
    struct orange_inode *pin = orangefs_iget(ino);
    pin->i_nr_blocks = 0;
    pin->i_size = 0;
    pin->i_mode = I_REGULAR;
    orangefs_syncinode(ino, pin);
    if(orangefs_add_direntry((int)parent, name, ino) == 0) {
        struct fuse_entry_param e;
        memset(&e, 0, sizeof(e));
		e.ino = ino;
		e.attr_timeout = 1.0;
		e.entry_timeout = 1.0;
		orangefs_stat(e.ino, &e.attr);
        fi->direct_io = 1;
        fuse_reply_create(req, &e, fi);
    }else {
        fuse_reply_err(req, ENOSPC);
    }
}

void orangefs_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
		       mode_t mode)
{
    int ino = orangefs_find(parent, name, NULL);
    if(ino != 0) {
        fuse_reply_err(req, EEXIST);
    }
    ino = orangefs_allocinode();
    struct orange_inode *pin = orangefs_iget(ino);
    pin->i_size = 0;
    pin->i_mode = I_DIRECTORY;
    orangefs_allocsector(pin);
    orangefs_syncinode(ino, pin);
    if(orangefs_add_direntry((int)parent, name, ino) == 0) {
        // orangefs_add_direntry(ino, ".", ino);
        // orangefs_add_direntry(ino, "..", parent);
        struct fuse_entry_param e;
        memset(&e, 0, sizeof(e));
		e.ino = ino;
		e.attr_timeout = 1.0;
		e.entry_timeout = 1.0;
		orangefs_stat(e.ino, &e.attr);
        fuse_reply_entry(req, &e);
    }else {
        fuse_reply_err(req, ENOSPC);
    }
}


static const struct fuse_lowlevel_ops orangefs_oper = {
    .init = orangefs_init,
    .destroy = orangefs_destory,
    .lookup = orangefs_lookup,
    .getattr = orangefs_getattr,
    .listxattr = orangefs_listxattr,
    .readdir = orangefs_readdir,
    .open = orangefs_open,
    .read = orangefs_read,
    .write = orangefs_write,
    .create = orangefs_create,
    .mkdir = orangefs_mkdir,
    .unlink = orangefs_unlink,
    .rmdir = orangefs_rmdir,
    .forget = orangefs_forget,
    .rename = orangefs_rename,
};

static void show_help(const char *progname)
{
    printf("usage: %s [options] <device> <mountpoint>\n\n", progname);
}

static int orangefs_opt_proc(void *data, const char *arg, int key,
                             struct fuse_args *outargs)
{
    (void)data;
    (void)outargs;

    switch (key)
    {
    case FUSE_OPT_KEY_OPT:
        return 1;
    case FUSE_OPT_KEY_NONOPT:
        if (!options.disk)
        {
            options.disk = strdup(arg);
            return 0; // discard input disk (first argv), second is mountpoint
        }
        return 1;
    default:
        fprintf(stderr, "internal error\n");
        abort();
    }
}

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;
    struct fuse_cmdline_opts opts;
    struct fuse_loop_config config;
    int ret = -1;
    options.disk = NULL;
    /* Parse options */
    if (fuse_opt_parse(&args, &options, option_spec, orangefs_opt_proc) == -1)
        return 1;

    if (!options.disk)
    {
        show_help(argv[0]);
        exit(1);
    }
    if (fuse_parse_cmdline(&args, &opts) != 0)
        return 1;
    if (opts.show_help)
    {
        show_help(argv[0]);
        fuse_cmdline_help();
        fuse_lowlevel_help();
        ret = 0;
        goto err_out1;
    }
    else if (opts.show_version)
    {
        printf("FUSE library version %s\n", fuse_pkgversion());
        fuse_lowlevel_version();
        ret = 0;
        goto err_out1;
    }

    if (opts.mountpoint == NULL)
    {
        show_help(argv[0]);
        ret = 1;
        goto err_out1;
    }
    #ifdef FUSE_OLD
    se = fuse_lowlevel_new(&args, &orangefs_oper, sizeof(orangefs_oper), NULL);
    struct fuse_chan *ch = fuse_mount(opts.mountpoint, &args);
    fuse_session_add_chan(se, ch);
    #else
    se = fuse_session_new(&args, &orangefs_oper,
                          sizeof(orangefs_oper), NULL);
    fuse_session_mount(se, opts.mountpoint);
    #endif

    if (fuse_set_signal_handlers(se) != 0)
        goto err_out;
    fuse_daemonize(opts.foreground);

    /* Block until ctrl+c or fusermount -u */
    if (opts.singlethread)
        ret = fuse_session_loop(se);
    else
    {
        config.clone_fd = opts.clone_fd;
        config.max_idle_threads = opts.max_idle_threads;
        #ifdef FUSE_OLD
        ret = fuse_session_loop_mt(se);
        #else
        ret = fuse_session_loop_mt(se, &config);
        #endif
    }
err_out:
    fuse_remove_signal_handlers(se);
    #ifdef FUSE_OLD
    fuse_session_remove_chan(ch);
    fuse_session_destroy(se);
    fuse_unmount(opts.mountpoint, ch);
    #else
    fuse_session_destroy(se);
    fuse_session_unmount(se);
    #endif
err_out1:
    free(opts.mountpoint);
    fuse_opt_free_args(&args);
    return ret ? 1 : 0;
}