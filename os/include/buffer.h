int buf_write_block(int dev, int block, int pid, void *buf);
int buf_read_block(int dev, int block, int pid, void *buf);
void init_buffer(int num_block);
#define BUF_RD_BLOCK(dev,block_nr,fsbuf) buf_read_block(dev,block_nr,proc2pid(p_proc_current),fsbuf);
#define BUF_WR_BLOCK(dev,block_nr,fsbuf) buf_write_block(dev,block_nr,proc2pid(p_proc_current),fsbuf);
