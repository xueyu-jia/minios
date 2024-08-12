/**********************************************
 *			exec.c 		add by visual 2016.5.23
 *************************************************/

#include "type.h"
#include "const.h"
#include "string.h"
#include "elf.h"
#include "vfs.h"
#include "memman.h"
#include "pagetable.h"
#include "mmap.h"
#include "console.h"


PRIVATE int exec_load(u32 fd, const Elf32_Ehdr *Echo_Ehdr, const Elf32_Phdr *Echo_Phdr);
PRIVATE int exec_pcb_init(const char *path);
PRIVATE int exec_count_args(char* const* pstr, int *count);
PRIVATE int exec_copy_args(char* const* pstr, char** parr, char *dst, u32 offset);
PRIVATE int exec_replace_argv_and_envp(
    const char*     path,
    char* const*    argv,
    char* const*    envp
);

PUBLIC u32 do_execve(const char* path, char* const* argv, char* const* envp); // added by xyx&&wjh 2021.12.31
PUBLIC u32 kern_execve(const char* path, char* const* argv, char* const* envp); // added by xyx&&wjh 2021.12.31

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

PUBLIC u32 do_execve(const char* path, char* const* argv, char* const* envp) {
    return kern_execve(path, argv, envp);
}

/*   end added   */

/**
 * @brief   将参数和环境变量加载到进程的内存中
 * @note    会释放进程的所有内存，重新映射虚拟地址空间
 */
PRIVATE int exec_replace_argv_and_envp(
    const char*     path,
    char* const*    argv,
    char* const*    envp
)
{
    //  处理参数 argc 放在 main ebp + 8 (ArgLinBase); argv 放在 ebp + 12
    // low<--     ---stack---       -->high(base)
    //            user main stack  |        argc             | argv | envp | argv[0]... argv[argc-1]| 0 |... argv raw data | env data | |
    // main esp  |  call main rt   | StackLinBase/ArgLinBase |

    int argc, env_space, count = 0, space = 0;
    space     += exec_count_args(argv, &count);
    argc       = count;
    env_space  = space;
    space     += exec_count_args(envp, &count);
    space += (count + 5) * 4; // argc + argv + envp + 1(args end 0) + 1(env end 0)
    // 补充: 参数数量不能超过最大值
    if (argc > MAXARG || space > num_4K) goto close_on_error;

    // 使用临时分配的页保存新进程的参数，因为要复制的参数有可能就位于原进程的参数区
    char *tmp_buf = (char*)kern_kmalloc_4k();
    char **args_base_in_tmp     = (char **)(tmp_buf + 12);
    char **envs_base_in_tmp     = args_base_in_tmp + argc + 1;
    char  *args_raw_base_in_tmp = ((char *)(tmp_buf + 12)) + num_4B * (count + 2);

    // 将数据存入暂存区tmp_buf
    // 存入的地址应该是进程访问的页的地址而非临时页中数据的地址，所以有一步换算
    u32 offset = ArgLinBase - (u32)tmp_buf;
    *((int *)tmp_buf)          = argc;
    *((char **)(tmp_buf + 4)) = (char *)args_base_in_tmp + offset;
    *((char **)(tmp_buf + 8)) = (char *)envs_base_in_tmp + offset;
    exec_copy_args(argv, args_base_in_tmp, args_raw_base_in_tmp, offset);
    exec_copy_args(envp, envs_base_in_tmp, args_raw_base_in_tmp + env_space, offset);

    // 释放进程所有内存
    // 除了page dir全部可以释放
    int pid = p_proc_current->task.pid;
    memmap_clear(p_proc_current);
    free_all_pagetbl(pid);

    //重新初始化该进程的进程表信息（包括LDT）、线性地址布局、进程树属性
    exec_pcb_init(path);
    do_mmap(ArgLinBase, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, -1, 0);

    // 将暂存区的数据拷贝到进程新的内存
    memcpy((void *)ArgLinBase, tmp_buf, space);
    kern_kfree_4k(tmp_buf);
    return 0;

close_on_error:
    return -1;
}

/*======================================================================*
 *                          kern_exec		add by visual 2016.5.23
 *exec系统调用功能实现部分
 *======================================================================*/
PUBLIC u32 kern_execve(const char* path, char* const* argv, char* const* envp)
{
    u32 err_temp;
    STACK_FRAME *p_reg;

    /*******************打开文件************************/
    u32 fd = kern_vfs_open(path, O_RDONLY, 0);
    if (fd == -1)
        goto open_file_failed;

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

    err_temp = exec_replace_argv_and_envp(_path, argv, envp);
    if (0 != err_temp) {
        disp_str("exec:replace argv and envp failed");
        goto close_on_error;
    }

    /*************根据elf的program复制文件信息**************/
    if (-1 == exec_load(fd, Echo_Ehdr, Echo_Phdr)) {
        disp_str("exec_load error!\n");
        goto fatal_error;
    }

    /***********************代码、数据、堆、栈***************************/
    p_reg = proc_kstacktop(p_proc_current);
    p_reg->eip = Echo_Ehdr->e_entry;
    p_reg->esp = (u32)p_proc_current->task.memmap.stack_lin_base;
    do_mmap(p_proc_current->task.memmap.stack_lin_limit,
        p_proc_current->task.memmap.stack_lin_base-p_proc_current->task.memmap.stack_lin_limit,
        PROT_READ|PROT_WRITE, MAP_PRIVATE, -1, 0);

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

open_file_failed:
    disp_str(path);
    disp_str(":sys_exec open error!\n"); // added by mingxuan 2020-12-22
    return -1;

}


/**
 * @brief 将elf文件加载到进程的内存中
 */
PRIVATE int exec_load(
    u32 fd,
    const Elf32_Ehdr *Echo_Ehdr,
    const Elf32_Phdr *Echo_Phdr
)
{
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
PRIVATE int exec_pcb_init(const char *path) {
    // char *p_regs; // point to registers in the new kernel stack, added by xw,
                  // 17/12/11

    // 名称 状态 特权级 寄存器
    strcpy(p_proc_current->task.p_name, path); // 名称

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
    // p_proc_current->task.tree_info.type = TYPE_PROCESS;
    // //当前是进程还是线程 p_proc_current->task.tree_info.real_ppid = -1;
    // //亲父进程，创建它的那个进程 p_proc_current->task.tree_info.ppid = -1;
    // //当前父进程 p_proc_current->task.tree_info.child_p_num = 0;	//子进程数量
    // p_proc_current->task.tree_info.child_process[NR_CHILD_MAX];//子进程列表
    // p_proc_current->task.tree_info.child_t_num = 0;		//子线程数量
    // p_proc_current->task.tree_info.child_thread[NR_CHILD_MAX];//子线程列表
    p_proc_current->task.tree_info.text_hold = 1; // 是否拥有代码
    p_proc_current->task.tree_info.data_hold = 1; // 是否拥有数据
    p_proc_current->task.nice = 0;
    p_proc_current->task.weight=nice_to_weight[p_proc_current->task.nice+20];
    return 0;
}

PRIVATE int exec_count_args(char* const* pstr, int *count) {
    char *p     = NULL;
    int   space = 0;
    while (pstr && (p = *pstr++, p)) {
        space += strlen(p) + 1;
        (*count)++;
    }
    return space;
}

PRIVATE int exec_copy_args(char* const* pstr, char** parr, char *dst, u32 offset) {
    char *p   = NULL;
    while (pstr && (p = *pstr++, p)) {
        strcpy(dst, p);
        *(parr++)  = dst + offset;
        dst       += strlen(p) + 1;
    }
    *(parr++) = 0;
    return 0;
}
