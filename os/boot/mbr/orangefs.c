#include "orangefs.h"
#include "disk.h"
#include "loaderprint.h"
#include "string.h"
super_block sb;
//遍历根目录，找到filename 对应的inode id 
static int search_file(char *filename){
    //1个boot扇区 1个superblock扇区 nr_imap_sects个inode map扇区 nr_smap_sects个sector map扇区
    int inode_array_start_sect=bootPartStartSector+(1+1+sb.nr_imap_sects+sb.nr_smap_sects)*(BLOCK_SIZE/SECT_SIZE);
    //读出存储1号inode 的扇区 1号inode是根目录的inode
    readsect(BUF_ADDR,inode_array_start_sect);
    inode root_inode;
    memcpy(&root_inode,BUF_ADDR,INODE_SIZE);
    //lprintf("i_size %d i_start_sect %d i_nr_sects %d\n",root_inode.i_size,root_inode.i_start_sect,root_inode.i_nr_sects);
    for (int i = 0; i < root_inode.i_nr_sects*(BLOCK_SIZE/SECT_SIZE); i++)
    {
        readsect(BUF_ADDR,bootPartStartSector+root_inode.i_start_sect*(BLOCK_SIZE/SECT_SIZE)+i);
        dir_entry* de = (dir_entry*)(BUF_ADDR);
        for (int j = 0; j < SECTSIZE/DIR_ENTRY_SIZE; j++)
        {
/*             lprintf(de->name);
            lprintf("\n"); */
            if(strcmp(de->name,filename)==0){
                return de->inode_nr;
            }
            de++;
        }
    }
    return 0;
}
//根据inode id，返回inode
static inode get_inode(int inode_id){
    inode target_inode;
    //1个boot扇区 1个superblock扇区 nr_imap_sects个inode map扇区 nr_smap_sects个sector map扇区
    int inode_array_start_sect=bootPartStartSector+(1+1+sb.nr_imap_sects+sb.nr_smap_sects)*(BLOCK_SIZE/SECT_SIZE);
    int nr_inode_per_sect = SECTSIZE/INODE_SIZE;
    int inode_sect = inode_array_start_sect+(inode_id-1)/nr_inode_per_sect;
    int inode_offset_in_sect = (inode_id-1)%nr_inode_per_sect;
    readsect(BUF_ADDR,inode_sect);
    memcpy(&target_inode,BUF_ADDR+(INODE_SIZE*inode_offset_in_sect),INODE_SIZE);
    return target_inode;

}
void orangefs_init(){
    //superblock初始化
    readsect(BUF_ADDR,bootPartStartSector+(BLOCK_SIZE/SECT_SIZE));
    memcpy(&sb,BUF_ADDR,SUPER_BLOCK_SIZE);
    //lprintf("nr inodes %d nr 1st sect %d  nr_sects %d \n",sb.nr_inodes,sb.n_1st_sect,sb.nr_sects);
    
}
//读文件到dst的内存中
int orangefs_read_file(char *filename,void *dst){
    int inode_id = search_file(filename);
    if(inode_id==0||inode_id>sb.nr_inodes){
        lprintf("error : can not find inode!\n");
        return FALSE;
    }
    //lprintf("inode id %d\n",inode_id);
    inode target_inode = get_inode(inode_id);
    //lprintf("inode i_start_sect %d i size  %d \n",target_inode.i_start_sect,target_inode.i_size);
    //文件实际占用的扇区的数量(上取整)
    int num_sect =  target_inode.i_size%SECTSIZE == 0?target_inode.i_size/SECTSIZE:target_inode.i_size/SECTSIZE+1;
    //lprintf("isize %d numn sect %d\n", target_inode.i_size,target_inode.i_size/SECTSIZE);
    readsects(dst, bootPartStartSector+target_inode.i_start_sect*(BLOCK_SIZE/SECT_SIZE), num_sect);
/*      for (int i = 0; i < num_sect+1; i++)
    {
        readsect(dst+i*SECTSIZE,bootPartStartSector+target_inode.i_start_sect+i);
    }  */
    return TRUE;
}