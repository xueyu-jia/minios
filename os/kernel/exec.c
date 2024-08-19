/**********************************************
 *			exec.c 		add by visual 2016.5.23
 *************************************************/

#include <kernel/type.h>
#include <kernel/const.h>
#include <klib/string.h>
#include <kernel/elf.h>
#include <kernel/vfs.h>
#include <kernel/memman.h>
#include <kernel/pagetable.h>
#include <kernel/mmap.h>
#include <kernel/console.h>
#include <klib/assert.h>
#include <kernel/proto.h>
#include <kernel/exec.h>


PRIVATE int exec_load(u32 fd, const Elf32_Ehdr *Echo_Ehdr, const Elf32_Phdr *Echo_Phdr);
PRIVATE int exec_pcb_init(const char *path);
PRIVATE int exec_count_args(char* const* pstr, u32 *count);
PRIVATE int exec_copy_args(char* const* pstr, char** parr, char *dst, u32 offset);
PRIVATE int exec_replace_argv_and_envp(
    const char*     path,
    char* const*    argv,
    char* const*    envp
);
PRIVATE void* exec_read_and_load(int fd);
PRIVATE const char* extract_program_name(const char* __path);

PUBLIC u32 kern_execve(const char* path, char* const* argv, char* const* envp)
{
    int err_temp;
    STACK_FRAME *p_reg;

    /*******************打开文件************************/
    int fd = kern_vfs_open(path, O_RDONLY, 0);
    if (fd == -1)
        goto open_file_failed;

    char* _path = (char*)kern_kmalloc(strlen(path) + 1);
    strcpy(_path, path);
    const char* proc_name = extract_program_name(_path);

    // update argv and envp
    err_temp = exec_replace_argv_and_envp(proc_name, argv, envp);

    if (0 != err_temp) {
        disp_str("exec:replace argv and envp failed");
        goto close_on_error;
    }

    //! ATTENTION: from here, subsequent operations will massively
    //! modify the pcb's data and page tables, and the recovery of
    //! errors will also be extremely costly. therefore, we directly
    //! mark the errors that occur in the future as unrecoverable
    //! errors. when these errors occur, execve directly panic and does
    //! not return
    //! TODO: introduec a more advanced architecture to solve this kind
    //! of problem

    void* entry_point = exec_read_and_load(fd);//读取并加载可执行文件的内容，并关闭文件
    kern_vfs_close(fd);
    if(entry_point == NULL){
        disp_str("exec_load error!\n");
        goto fatal_error;
    }

    // update eip and esp, remap the stack
    p_reg = proc_kstacktop(p_proc_current);
    p_reg->eip = (u32)entry_point;
    p_reg->esp = (u32)p_proc_current->task.memmap.stack_lin_base;
    err_temp = do_mmap(
        p_proc_current->task.memmap.stack_lin_limit,
        p_proc_current->task.memmap.stack_lin_base-p_proc_current->task.memmap.stack_lin_limit,
        PROT_READ|PROT_WRITE, MAP_PRIVATE, -1, 0);
    if(-1 == err_temp){
        disp_str("do_mmap in exec error!\n");
        goto fatal_error;
    }
    return 0;

open_file_failed:
    disp_str(path);
    disp_str(":sys_exec open error!\n");
    return -1;

close_on_error:
    kern_kfree((u32)_path);
    kern_vfs_close(fd);
    return -1;

fatal_error:
    kern_kfree((u32)_path);
    do_exit(-1);
    //: can't reach
    return -1;

}

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

    u32 argc, env_space, count = 0, space = 0;
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
    kern_kfree_4k((u32)tmp_buf);
    return 0;

close_on_error:
    return -1;
}

PRIVATE void* exec_read_and_load(int fd)
{
    Elf32_Ehdr* elf_header = (Elf32_Ehdr*)kern_kmalloc(sizeof(Elf32_Ehdr));
    assert(elf_header != NULL);
    read_Ehdr(fd, elf_header, 0);

    Elf32_Phdr* elf_proghs = (Elf32_Phdr*)kern_kmalloc(sizeof(Elf32_Phdr) * elf_header->e_phnum);
    assert(elf_proghs != NULL);

    for (int i = 0; i < elf_header->e_phnum; i++) {
        u32 offset = elf_header->e_phoff + i * sizeof(Elf32_Phdr);//计算当前程序头在文件中的偏移量。
        read_Phdr(fd, elf_proghs + i, offset);//函数读取程序头的内容，将其存储在程序头表中的相应位置。
    }

    void* entry_point = (void*)elf_header->e_entry;
    int   resp        = exec_load(fd, elf_header, elf_proghs);

    kern_kfree((u32)elf_header);
    kern_kfree((u32)elf_proghs);

    return resp == 0 ? entry_point : NULL;
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
    const Elf32_Phdr *phdr;
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

/**
 * @brief   重新初始化进程的寄存器和特权级、线性分布地址布局
 */
PRIVATE int exec_pcb_init(const char *proc_name) {
    // char *p_regs; // point to registers in the new kernel stack, added by xw,
                  // 17/12/11

    // 名称 状态 特权级 寄存器
    strcpy(p_proc_current->task.p_name, proc_name); // 名称

    init_user_cpu_context(&p_proc_current->task.context, proc2pid((p_proc_current)));

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

PRIVATE int exec_count_args(char* const* pstr, u32 *count) {
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

/**
 * @brief   execv系统调用功能实现部分
 */
PUBLIC u32 sys_execv()
{
    return do_execve((const char*)get_arg(1), (char* const*)get_arg(2), NULL);
}

/**
 * @brief   execvp系统调用功能实现部分
 */
PUBLIC u32 sys_execvp()
{
    return do_execve((const char*)get_arg(1), (char* const*)get_arg(2), NULL);
}


PUBLIC u32 sys_execve()
{
    return do_execve((const char*)get_arg(1), (char* const*)get_arg(2), (char* const*)get_arg(3));
}

PUBLIC u32 do_execve(const char* path, char* const* argv, char* const* envp)
{
    return kern_execve(path, argv, envp);
}

PRIVATE const char* extract_program_name(const char* __path)
{
    // 找到最后一个斜杠的位置
    const char *last_slash = strrchr(__path, '/');
    // 根据最后一个斜杠的位置来确定程序名的起始位置
    const char *program_name = last_slash ? last_slash + 1 : __path;
    return program_name;
}
