/**********************************************
 *			exec.c 		add by visual 2016.5.23
 *************************************************/

#include "type.h"
#include "const.h"
#include "string.h"
#include "elf.h"
#include "vfs.h"   //added by xyx && wjh 2021-12-31
#include "memman.h"
#include "pagetable.h"

// PRIVATE u32 exec_elfcpy(u32 fd, Elf32_Phdr *Echo_Phdr);
PRIVATE u32
    exec_load(u32 fd, const Elf32_Ehdr *Echo_Ehdr, const Elf32_Phdr *Echo_Phdr);
PRIVATE int exec_pcb_init(char *path);
PRIVATE int exec_count_args(char **pstr, int *count);
PRIVATE int exec_copy_args(char **pstr, char **parr, char *dst, int dst_delta);

PUBLIC u32 do_execve(
    char *path, char *argv[], char *envp[]); // added by xyx&&wjh 2021.12.31
PUBLIC u32 kern_execve(
    char *path, char *argv[], char *envp[]); // added by xyx&&wjh 2021.12.31

/*    added by xyx&&wjh 2021.12.31  */
/*======================================================================*
 *                          sys_execv
 *execv系统调用功能实现部分
 *======================================================================*/
PUBLIC u32 sys_execv() {
    return do_execve(get_arg(1), get_arg(2), NULL);
}

/*======================================================================*
 *                          sys_execvp
 *execvp系统调用功能实现部分
 *======================================================================*/
PUBLIC u32 sys_execvp() {
    return do_execve(get_arg(1), get_arg(2), NULL);
}

PUBLIC u32 sys_execve() {
    return do_execve(get_arg(1), get_arg(2), get_arg(3));
}

PUBLIC u32 do_execve(char *path, char *argv[], char *envp[]) {
    return kern_execve(path, argv, envp);
}

/*   end added   */

/*======================================================================*
 *                          kern_exec		add by visual 2016.5.23
 *exec系统调用功能实现部分
 *======================================================================*/
PUBLIC u32 kern_execve(
    char *path, char *argv[], char *envp[]) // modified by mingxuan 2021-8-11
{
    u32 addr_lin;
    u32 err_temp;
    u32 pde_addr_phy, addr_phy_temp;

    STACK_FRAME *p_reg; // point to a register in the new kernel stack, added by xw,
                 // 17/12/11
    if (0 == path) {
        disp_color_str("exec: path ERROR!", 0x74);
        return -1;
    }

    /*******************打开文件************************/
    u32 fd = kern_vfs_open(path, O_RDONLY, 0);

    if (fd == -1) {
        disp_str(path);
        disp_str(":sys_exec open error!\n"); // added by mingxuan 2020-12-22
        return -1;
    }

    Elf32_Ehdr *Echo_Ehdr = kern_kmalloc(sizeof(Elf32_Ehdr));
    Elf32_Phdr *Echo_Phdr = kern_kmalloc(10 * sizeof(Elf32_Phdr));
    Elf32_Shdr *Echo_Shdr = kern_kmalloc(10 * sizeof(Elf32_Shdr));
    char       *_path     = kern_kmalloc(strlen(path) + 1);
    strcpy(_path, path);

    /*************获取elf信息**************/
    err_temp = read_elf(fd, Echo_Ehdr, Echo_Phdr, Echo_Shdr);

    if (0 != err_temp) {
        disp_str("not valid elf file");
        goto close_on_error;
    }

    //  处理参数 argc 放在 main ebp + 8 (ArgLinBase); argv 放在 ebp + 12
    // low<--     ---stack---       -->high(base)
    //   | 		user main stack		|   argc 					|  argv
    //   | envp	| argv[0]... argv[argc-1]| 0 |... argv raw data | env data | |
    //   main esp  |  call main rt	|	StackLinBase/ArgLinBase |
    int argc, env_space, arr = 0, space = 0;
    space     += exec_count_args(argv, &arr);
    argc       = arr;
    env_space  = space;
    space     += exec_count_args(envp, &arr);
    space += (arr + 5) * 4; // argc + argv + envp + 1(args end 0) + 1(env end 0)
    // 补充: 参数数量不能超过最大值
    if (argc > MAXARG || space > num_4K) { goto close_on_error; }
    char *page_buf =
        kern_kmalloc_4k(); // 使用临时分配的页保存新进程的参数，因为要复制的参数有可能就位于原进程的参数区
    char **args_base     = (char **)(page_buf + 12);
    char **envs_base     = args_base + argc + 1;
    char  *args_raw_base = ((char *)(page_buf + 12)) + num_4B * (arr + 2);
    int    delta =
        ArgLinBase
        - (u32)
            page_buf; // 存入的地址应该是进程访问的页的地址而非临时页中数据的地址，所以有一步换算
    *((int *)page_buf)          = argc;
    *((char ***)(page_buf + 4)) = (char *)args_base + delta;
    *((char ***)(page_buf + 8)) = (char *)envs_base + delta;
    exec_copy_args(argv, args_base, args_raw_base, delta);
    exec_copy_args(envp, envs_base, args_raw_base + env_space, delta);
    /*************释放进程内存****************/
    // 除了page dir全部可以释放
    int pid = p_proc_current->task.pid;
    // free_all_phypage(pid);
    memmap_clear(p_proc_current);
    free_all_pagetbl(pid);
    /*****************重新初始化该进程的进程表信息（包括LDT）、线性地址布局、进程树属性********************/
    exec_pcb_init(_path);
    do_mmap(ArgLinBase, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, -1, 0);
    // err_temp = ker_umalloc_4k(ArgLinBase, pid, PG_P | PG_USU | PG_RWW);
    /*************根据elf的program复制文件信息**************/
    if (-1 == exec_load(fd, Echo_Ehdr, Echo_Phdr)) {
        disp_str("exec_load error!\n");
        goto fatal_error;
    }
    memcpy((void *)ArgLinBase, page_buf, space);
    kern_kfree_4k(page_buf);


    /***********************代码、数据、堆、栈***************************/
    // 代码、数据已经处理，将eip重置即可
    // p_proc_current->task.regs.eip = Echo_Ehdr->e_entry;
    // //进程入口线性地址 deleted by lcy 2023.10.25

    // p_reg = (char *)(p_proc_current + 1); // added by xw, 17/12/11
    p_reg = proc_kstacktop(p_proc_current);
    //*((u32 *)(p_reg + EIPREG - P_STACKTOP)) = p_proc_current->task.regs.eip;
    ////added by xw, 17/12/11  deleted by lcy 2023.10.25
    // *((u32 *)(p_reg + EIPREG - P_STACKTOP)) = Echo_Ehdr->e_entry;
    p_reg->eip = Echo_Ehdr->e_entry;
    // test_kbud_mem_size();

    // 栈deleted by lcy 2023.10.25
    // p_proc_current->task.regs.esp =
    // (u32)p_proc_current->task.memmap.stack_lin_base; //栈地址最高处
    //*((u32 *)(p_reg + ESPREG - P_STACKTOP)) = p_proc_current->task.regs.esp;
    ////added by xw, 17/12/11
    p_reg->esp = (u32)p_proc_current->task.memmap.stack_lin_base;
    // *((u32 *)(p_reg + ESPREG - P_STACKTOP)) =
    //     (u32)p_proc_current->task.memmap
    //         .stack_lin_base; // added by lcy 2023.10.25

    // u32 stack_lin_base = addr_lin=p_proc_current->task.memmap.stack_lin_base
    // - num_4K;	//added vy mingxuan 2021-8-24
    // for (addr_lin = p_proc_current->task.memmap.stack_lin_limit;
    //      addr_lin < UPPER_BOUND_4K(p_proc_current->task.memmap.stack_lin_base);
    //      addr_lin += num_4K) {
    //     // 进程栈上分配物理页，所以调用ker_umalloc_4k,  wang 2021.8.27
    //     err_temp = ker_umalloc_4k(
    //         addr_lin,
    //         p_proc_current->task.pid,
    //         PG_P | PG_USU | PG_RWW); // edited by wang 2021.8.27
    //     if (err_temp != 0) {
    //         disp_color_str("kernel_main Error:lin_mapping_phy", 0x74);

    //         goto close_on_error;
    //     }
    // }
    do_mmap(p_proc_current->task.memmap.stack_lin_limit, 
        p_proc_current->task.memmap.stack_lin_base-p_proc_current->task.memmap.stack_lin_limit,
        PROT_READ|PROT_WRITE, MAP_PRIVATE, -1, 0);

    // disp_color_str("\n[exec success:",0x72);//灰底绿字
    // disp_color_str(path,0x72);//灰底绿字
    // disp_color_str("]",0x72);//灰底绿字
    // disp_free();	//for test, added by mingxuan 2021-1-7
    kern_kfree(_path);
    kern_kfree(Echo_Ehdr);
    kern_kfree(Echo_Phdr);
    kern_kfree(Echo_Shdr);
    kern_vfs_close(fd);
    return 0;
fatal_error:    // 对于已经执行load及之后的错误已经不能恢复，故将当前进程退出
    kern_kfree(_path);
    kern_kfree(Echo_Ehdr);
    kern_kfree(Echo_Phdr);
    kern_kfree(Echo_Shdr);
    kern_vfs_close(fd);
    do_exit(-1);
close_on_error:
    kern_kfree(_path);
    kern_kfree(Echo_Ehdr);
    kern_kfree(Echo_Phdr);
    kern_kfree(Echo_Shdr);
    kern_vfs_close(fd);
    return -1;
}

/*    added by xyx&&wjh  2021-12-31  */

// PRIVATE char *exec_path(char* path) 此函数已无引用，故注释
// {
// 	u32 arg_type = FILE_ATTR;
// 	char default_fs_name[DEV_NAME_LEN] = DEFAULT_PATH;
// 	int fsname_len = strlen(default_fs_name);

// 	int pathlen = strlen(path);
// 	int len = (pathlen < DEV_NAME_LEN) ? pathlen : DEV_NAME_LEN;
// 	//disp_int(len);
// 	int i;
//     for(i=0;i<len;i++){
//         if( path[i] == '/'){
//             arg_type = PATH_ATTR;
//             break;
//         }
//     }

// 	if(arg_type == FILE_ATTR)
// 	{
// 		for(i=len - 1; i>=0; i--)
//         	path[i + fsname_len] = path[i];
// 		for(i=0; i<fsname_len; i++)
//         	path[i] = default_fs_name[i];
//     	path[len + fsname_len] = '\0';
// 	}
// 	return path;
// }
/*   end added   */

/*======================================================================*
 *                          exec_elfcpy		add by visual 2016.5.23
 *复制elf中program到内存中
 *======================================================================*/
// 此函数弃用 jiangfeng 2024.5, exec改为mmap
// PRIVATE u32 exec_elfcpy(
//     u32 fd,
//     Elf32_Phdr *
//         Echo_Phdr) // 这部分代码将来要移动到exec.c文件中，包括下面execve()中的一部分
// {
//     u32 lin_addr  = Echo_Phdr->p_vaddr;
//     u32 lin_limit = Echo_Phdr->p_vaddr + Echo_Phdr->p_memsz;

//     u32 file_offset = Echo_Phdr->p_offset;
//     u32 file_limit  = Echo_Phdr->p_offset + Echo_Phdr->p_filesz;

//     u32 lin_file_limit =
//         Echo_Phdr->p_vaddr + Echo_Phdr->p_filesz; // added by mingxuan 2021-3-16

//     // modified by mingxuan 2021-1-29, start
//     char *buf = kern_kmalloc_4k();
//     ; // added by mingxuan 2020-12-14

//     // added by mingxuan 2020-12-14
//     // 给lin_addr建立页映射, mingxuan
//     //	for (; lin_addr < lin_limit; lin_addr++, file_offset++)
//     // 20240402: 此处如果不能保证elf中的地址4K对齐，会导致最高一部分没有分配空间
//     // 如lin_addr=0x8051FF4, lin_limit=0x805C220 末尾0x805C000~0x805C220未分配
//     // 下面的复制数据同样是这样的写法,
//     // 但是复制数据是应当按照elf中的位置完全对应的，最后一次复制也做了处理，所以没有问题
//     // for (; lin_addr < lin_limit; lin_addr+=num_4K, file_offset+=num_4K)
//     // 页表操作移到exec_load

//     lin_addr    = Echo_Phdr->p_vaddr;  // added by mingxuan 2020-12-14
//     file_offset = Echo_Phdr->p_offset; // added by mingxuan 2020-12-14

//     u32 for_flag; // added by mingxuan 2021-3-16
//     for_flag = 0; // added by mingxuan 2021-8-8

//     // for(  ; lin_addr<lin_limit ; lin_addr+=num_4K,file_offset+=num_4K )	//
//     // modified by mingxuan 2020-12-14
//     for (; lin_addr < lin_limit
//            && file_offset < file_limit;) // modified by mingxuan 2021-3-16
//     {
//         for_flag = 1; // 表示进入了for循环，离开for循环时需要做变量调整 //added
//                       // by mingxuan 2021-3-16

//         // 以4K个字节为一个单位进行拷贝.
//         // 剩余的字节数小于4K字节，则一次全拷完剩余的字节, mingxuan if(
//         // lin_limit-lin_addr >= num_4K )	// modified by mingxuan 2020-12-14
//         if (lin_file_limit - lin_addr
//             >= num_4K) // modified by mingxuan 2021-3-16
//         {
//             kern_vfs_lseek(
//                 fd, file_offset, SEEK_SET); // modified by mingxuan 2021-8-19
//             kern_vfs_read(fd, buf, num_4K);
//             memcpy(lin_addr, buf, num_4K); // modified by mingxuan 2020-12-14
//             file_offset += num_4K;
//             lin_addr    += num_4K;
//         } else { // 剩余的字节数小于4K字节，则一次全拷完剩余的字节, mingxuan

//             u32 left_size = file_limit - file_offset;
//             kern_vfs_lseek(
//                 fd, file_offset, SEEK_SET); // modified by mingxuan 2021-8-19
//             kern_vfs_read(fd, buf, left_size);
//             memcpy(lin_addr, buf, left_size); // modified by mingxuan 2021-3-16
//             file_offset += left_size;         // added by mingxuan 2021-3-16
//             lin_addr    += left_size;
//         }
//     }
//     // modified by mingxuan 2021-1-29, end

//     // added by mingxuan 2021-3-16
//     //  if (for_flag == 1) //表示进入了for循环，离开for循环时需要做变量调整
//     //  {
//     //  	//减4K的原因是，最后一次for循环在判断时会自动加上4K
//     //  	lin_addr -= num_4K;	   //added by mingxuan 2021-3-16
//     //  	file_offset -= num_4K; //added by mingxuan 2021-3-16
//     //  }
//     //  后面 lin_addr 和 file_offset都不再使用 20240402
//     // added by mingxuan 2021-3-16
//     if (lin_limit - lin_file_limit > 0) {
//         memset(lin_file_limit, 0, lin_limit - lin_file_limit);
//     }
//     kern_kfree_4k(buf);

//     return 0;
// }

/*======================================================================*
 *                          exec_load		add by visual 2016.5.23
 *根据elf的program复制文件信息
 *======================================================================*/
PRIVATE u32 exec_load(
    u32 fd, const Elf32_Ehdr *Echo_Ehdr, const Elf32_Phdr *Echo_Phdr) {
    u32 ph_num;

    if (0 == Echo_Ehdr->e_phnum) {
        disp_color_str("exec_load: elf ERROR!", 0x74);
        return -1;
    }
    u32 text_lin_base = 0, text_lin_limit = 0, data_lin_base = 0,
        data_lin_limit = 0;
    Elf32_Phdr *phdr;
    for (ph_num = 0, phdr = Echo_Phdr; ph_num < Echo_Ehdr->e_phnum;
         ph_num++, phdr++) {
        if (phdr->p_type == ELF_LOAD) {
            if (phdr->p_flags == 0x6) { // have read/write permission
                if (data_lin_base) {
                    data_lin_base = min(data_lin_base, phdr->p_vaddr);
                    data_lin_limit =
                        max(data_lin_limit, phdr->p_vaddr + phdr->p_memsz);
                } else {
                    data_lin_base  = phdr->p_vaddr;
                    data_lin_limit = phdr->p_vaddr + phdr->p_memsz;
                }
            } else if (phdr->p_flags & 0x4) { // no write, can read
                if (text_lin_base) {
                    text_lin_base = min(text_lin_base, phdr->p_vaddr);
                    text_lin_limit =
                        max(text_lin_limit, phdr->p_vaddr + phdr->p_memsz);
                } else {
                    text_lin_base  = phdr->p_vaddr;
                    text_lin_limit = phdr->p_vaddr + phdr->p_memsz;
                }
            }
        }
    }
    u32 lin_addr;
    // for (lin_addr  = data_lin_base; lin_addr < UPPER_BOUND_4K(data_lin_limit);
    //      lin_addr += num_4K) {
    //     ker_umalloc_4k(
    //         lin_addr, p_proc_current->task.pid, PG_P | PG_USU | PG_RWW);
    //     // disp_int(lin_addr);
    //     // disp_str("=>");
    //     // disp_int(get_page_phy_addr(p_proc_current->task.pid, lin_addr));
    //     // disp_str("\n");
    // }
    // for (lin_addr  = text_lin_base; lin_addr < UPPER_BOUND_4K(text_lin_limit);
    //      lin_addr += num_4K) {
    //     ker_umalloc_4k(
    //         lin_addr, p_proc_current->task.pid, PG_P | PG_USU | PG_RWR);
    // }
    // for (ph_num = 0, phdr = Echo_Phdr; ph_num < Echo_Ehdr->e_phnum;
    //      ph_num++, phdr++) {
    //     if (phdr->p_type == ELF_LOAD) { 
    //         exec_elfcpy(fd, phdr);
    //     }
    // }
    // new mmu
    for (ph_num = 0, phdr = Echo_Phdr; ph_num < Echo_Ehdr->e_phnum;
         ph_num++, phdr++) {
        if (phdr->p_type == ELF_LOAD) { 
            // exec_elfcpy(fd, phdr);
            u32 prot = 0;
            if(phdr->p_flags & 4) prot |= PROT_READ;
            if(phdr->p_flags & 2) prot |= PROT_WRITE;
            if(phdr->p_flags & 1) prot |= PROT_EXEC;
            u32 in_page_offset = phdr->p_vaddr & 0xFFF; // 进行4K对齐
            u32 mem_start = phdr->p_vaddr - in_page_offset;
            u32 mem_size = phdr->p_memsz + in_page_offset;
            u32 file_size = phdr->p_filesz + in_page_offset;
            u32 addr;
            u32 file_page_size = UPPER_BOUND_4K(file_size);
            if(file_page_size){
                addr = do_mmap(mem_start, file_page_size, prot, 
                    MAP_PRIVATE|MAP_FIXED, fd, phdr->p_offset - in_page_offset);
                if(addr != mem_start) {
                    disp_str("error: execve mmap file failed!");
                }
            }
            if(mem_size > file_size) { // bss
                if(mem_size > file_page_size){// bss 超过elf映射的页，额外分配匿名页
                    addr = do_mmap(mem_start + file_page_size, mem_size - file_page_size, 
                        PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED, -1, 0);
                    if(addr != mem_start + file_page_size) {
                        disp_str("error: execve mmap bss failed!");
                    }
                }
                memset((void*)(mem_start + file_size), 0, file_page_size - file_size);
            }
        }
    }
    p_proc_current->task.memmap.data_lin_base  = data_lin_base;
    p_proc_current->task.memmap.data_lin_limit = data_lin_limit;
    p_proc_current->task.memmap.text_lin_base  = text_lin_base;
    p_proc_current->task.memmap.text_lin_limit = text_lin_limit;
    return 0;
}

/*======================================================================*
 *                          exec_init		add by visual 2016.5.23
 * 重新初始化寄存器和特权级、线性地址布局
 *======================================================================*/
PRIVATE int exec_pcb_init(char *path) {
    // char *p_regs; // point to registers in the new kernel stack, added by xw,
                  // 17/12/11

    // 名称 状态 特权级 寄存器
    strcpy(p_proc_current->task.p_name, path); // 名称
    // p_proc_current->task.stat = READY;         // 状态
    // p_proc_current->task.ldts[0].attr1 =
    //     DA_C | PRIVILEGE_USER << 5; // 特权级修改为用户级
    // p_proc_current->task.ldts[1].attr1 =
    //     DA_DRW | PRIVILEGE_USER << 5; // 特权级修改为用户级

    // /***************copy registers data****************************/
    // // copy registers data to the bottom of the new kernel stack
    // // added by xw, 17/12/11
    // // p_regs                             = (char *)(p_proc_current + 1);
    // // p_regs                            -= P_STACKTOP;
    // p_proc_current->task.esp_save_int  = proc_kstacktop(p_proc_current);
    // p_proc_current->task.esp_save_int->cs = common_cs | RPL_USER;
    // p_proc_current->task.esp_save_int->ds = common_ds | RPL_USER;
    // p_proc_current->task.esp_save_int->es = common_es | RPL_USER;
    // p_proc_current->task.esp_save_int->fs = common_fs | RPL_USER;
    // p_proc_current->task.esp_save_int->ss = common_ss | RPL_USER;
    // p_proc_current->task.esp_save_int->gs = common_gs | RPL_USER;
    // p_proc_current->task.esp_save_int->eflags = 0x202; /* IF=1,bit2 永远是1 */
    proc_init_ldt_kstack(p_proc_current, RPL_USER); //多个功能 都用到的就抽象出来吧，别造轮子了 modified 2024.05.06
    // memcpy(p_regs, (char *)p_proc_current, 18 * 4);

    // 进程表线性地址布局部分，text、data已经在前面初始化了
    p_proc_current->task.memmap.vpage_lin_base = VpageLinBase; // 保留内存基址
    p_proc_current->task.memmap.vpage_lin_limit = VpageLinBase; // 保留内存界限
    p_proc_current->task.memmap.heap_lin_base  = HeapLinBase; // 堆基址
    p_proc_current->task.memmap.heap_lin_limit = HeapLinBase; // 堆界限
    p_proc_current->task.memmap.stack_child_limit = StackLinLimitMAX; // add by visual 2016.5.27
    p_proc_current->task.memmap.stack_lin_base = StackLinBase; // 栈基址
    p_proc_current->task.memmap.stack_lin_limit = StackLinBase - 0x4000; // 栈界限（使用时注意栈的生长方向）
    p_proc_current->task.memmap.arg_lin_base = ArgLinBase; // 参数内存基址
    p_proc_current->task.memmap.arg_lin_limit = ArgLinLimitMAX; // added byxyx&&wjh  2021-12-31
    p_proc_current->task.memmap.kernel_lin_base = KernelLinBase; // 内核基址
    p_proc_current->task.memmap.kernel_lin_limit = KernelLinBase + kernel_size; // 内核大小初始化为8M
    list_init(&p_proc_current->task.memmap.vma_map);
	init_mem_page(&p_proc_current->task.memmap.anon_pages, MEMPAGE_AUTO);
    // 进程树属性,只要改两项，其余不用改
    // p_proc_current->task.info.type = TYPE_PROCESS;
    // //当前是进程还是线程 p_proc_current->task.info.real_ppid = -1;
    // //亲父进程，创建它的那个进程 p_proc_current->task.info.ppid = -1;
    // //当前父进程 p_proc_current->task.info.child_p_num = 0;	//子进程数量
    // p_proc_current->task.info.child_process[NR_CHILD_MAX];//子进程列表
    // p_proc_current->task.info.child_t_num = 0;		//子线程数量
    // p_proc_current->task.info.child_thread[NR_CHILD_MAX];//子线程列表
    p_proc_current->task.info.text_hold = 1; // 是否拥有代码
    p_proc_current->task.info.data_hold = 1; // 是否拥有数据
    p_proc_current->task.nice = 0;
    p_proc_current->task.weight=nice_to_weight[p_proc_current->task.nice+20];
    return 0;
}

PRIVATE int exec_count_args(char **pstr, int *count) {
    char *p     = NULL;
    int   space = 0;
    while (pstr && (p = *pstr++, p)) {
        space += strlen(p) + 1;
        (*count)++;
    }
    return space;
}

PRIVATE int exec_copy_args(char **pstr, char **parr, char *dst, int dst_delta) {
    char *p   = NULL;
    int   len = 0;
    while (pstr && (p = *pstr++, p)) {
        strcpy(dst, p);
        *(parr++)  = dst + dst_delta;
        dst       += strlen(p) + 1;
    }
    *(parr++) = 0;
    return 0;
}