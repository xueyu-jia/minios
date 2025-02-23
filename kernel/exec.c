#include <minios/exec.h>
#include <minios/console.h>
#include <minios/memman.h>
#include <minios/vfs.h>
#include <minios/exit.h>
#include <minios/elf.h>
#include <minios/assert.h>
#include <minios/mmap.h>
#include <minios/page.h>
#include <minios/layout.h>
#include <minios/syscall.h>
#include <klib/stddef.h>
#include <klib/size.h>
#include <string.h>

static int exec_load(u32 fd, const elf32_hdr_t* eh, const elf32_phdr_t* ph);
static int exec_pcb_init(const char* path);
static int exec_count_args(char* const* s, u32* count);
static int exec_copy_args(char* const* s, char** parr, char* dst, u32 offset);
static int exec_replace_argv_and_envp(const char* path, char* const* argv, char* const* envp);
static void* exec_read_and_load(int fd);
static const char* extract_program_name(const char* __path);

int kern_execve(const char* path, char* const* argv, char* const* envp) {
    int err_code = 0;
    stack_frame_t* stack_frame = NULL;
    char* path_cloned = NULL;

    //! TODO: support flag O_CLOEXEC

    //! TODO: allow suffixes like .exe,.com,.bin,.elf
    int fd = kern_vfs_open(path, O_RDONLY, 0);
    if (fd == -1) { goto open_file_failed; }
    struct inode* inode = p_proc_current->task.filp[fd]->fd_dentry->d_inode;
    if ((inode->i_type & I_TYPE_MASK) != I_REGULAR || (inode->i_mode & I_X) == 0) {
        goto permission_denied;
    }

    path_cloned = kern_kmalloc(strlen(path) + 1);
    strcpy(path_cloned, path);
    const char* proc_name = extract_program_name(path_cloned);

    // update argv and envp
    err_code = exec_replace_argv_and_envp(proc_name, argv, envp);

    if (0 != err_code) {
        kprintf("error: execve: failed to replace argv and envp\n");
        goto close_on_error;
    }

    //! ATTENTION: from here, subsequent operations will massively
    //! modify the pcb's data and page tables, and the recovery of
    //! errors will also be extremely costful. therefore, we directly
    //! mark the errors that occur in the future as unrecoverable
    //! errors. when these errors occur, execve directly panic and does
    //! not return
    //! TODO: introduce a more advanced architecture to solve this kind
    //! of problem

    void* entry_point = exec_read_and_load(fd); // 读取并加载可执行文件的内容，并关闭文件
    kern_vfs_close(fd);
    if (entry_point == NULL) {
        kprintf("error: execve: failed to load elf: %s\n", path);
        goto fatal_error;
    }

    // update eip and esp, remap the stack
    stack_frame = proc_kstacktop(p_proc_current);
    stack_frame->eip = (u32)entry_point;
    stack_frame->esp = (u32)p_proc_current->task.memmap.stack_lin_base;
    err_code = kern_mmap_safe(
        p_proc_current->task.memmap.stack_lin_limit,
        p_proc_current->task.memmap.stack_lin_base - p_proc_current->task.memmap.stack_lin_limit,
        PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    if (-1 == err_code) {
        kprintf("error: execve: failed to setup stack\n");
        goto fatal_error;
    }
    return 0;

open_file_failed:
    kprintf("error: execve: failed to open file: %s\n", path);
    return -1;

permission_denied:
    kprintf("error: execve: permission denied: %s\n", path);
    goto close_on_error;

close_on_error:
    if (path_cloned != NULL) { kern_kfree(path_cloned); }
    kern_vfs_close(fd);
    return -1;

fatal_error:
    kern_kfree(path_cloned);
    do_exit(-1);
    //! can't reach
    return -1;
}

/**
 * @brief   将参数和环境变量加载到进程的内存中
 * @note    会释放进程的所有内存，重新映射虚拟地址空间
 */
static int exec_replace_argv_and_envp(const char* path, char* const* argv, char* const* envp) {
    // 处理参数 argc 放在 main ebp + 8 (ArgLinBase); argv 放在 ebp + 12
    // low<--     ---stack---       -->high(base)
    //           user main stack  |        argc             | argv | envp |
    //           argv[0]... argv[argc-1]| 0 |... argv raw data | env data | |
    // main esp  |  call main rt   | StackLinBase/ArgLinBase |

    u32 argc, env_space, count = 0, space = 0;
    space += exec_count_args(argv, &count);
    argc = count;
    env_space = space;
    space += exec_count_args(envp, &count);
    space += (count + 5) * 4; // argc + argv + envp + 1(args end 0) + 1(env end 0)
    // 补充: 参数数量不能超过最大值
    if (argc > MAXARG || space > SZ_4K) goto close_on_error;

    // 使用临时分配的页保存新进程的参数，因为要复制的参数有可能就位于原进程的参数区
    char* tmp_buf = kern_kmalloc(SZ_4K);
    char** args_base_in_tmp = (char**)(tmp_buf + 12);
    char** envs_base_in_tmp = args_base_in_tmp + argc + 1;
    char* args_raw_base_in_tmp = ((char*)(tmp_buf + 12)) + SZ_4 * (count + 2);

    // 将数据存入暂存区tmp_buf
    // 存入的地址应该是进程访问的页的地址而非临时页中数据的地址，所以有一步换算
    u32 offset = ArgLinBase - (u32)tmp_buf;
    *((int*)tmp_buf) = argc;
    *((char**)(tmp_buf + 4)) = (char*)args_base_in_tmp + offset;
    *((char**)(tmp_buf + 8)) = (char*)envs_base_in_tmp + offset;
    exec_copy_args(argv, args_base_in_tmp, args_raw_base_in_tmp, offset);
    exec_copy_args(envp, envs_base_in_tmp, args_raw_base_in_tmp + env_space, offset);

    // 释放进程所有内存
    // 除了page dir全部可以释放
    int pid = p_proc_current->task.pid;
    memmap_clear(p_proc_current);
    free_all_pagetbl(pid);

    // 重新初始化该进程的进程表信息（包括LDT）、线性地址布局、进程树属性
    exec_pcb_init(path);
    kern_mmap_safe(ArgLinBase, PGSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);

    // 将暂存区的数据拷贝到进程新的内存
    memcpy((void*)ArgLinBase, tmp_buf, space);
    kern_kfree(tmp_buf);
    return 0;

close_on_error:
    return -1;
}

static void* exec_read_and_load(int fd) {
    elf32_hdr_t* elf_header = kern_kmalloc(sizeof(elf32_hdr_t));
    assert(elf_header != NULL);
    elf_read_header(fd, elf_header, 0);

    elf32_phdr_t* elf_proghs = kern_kmalloc(sizeof(elf32_phdr_t) * elf_header->e_phnum);
    assert(elf_proghs != NULL);

    for (int i = 0; i < elf_header->e_phnum; ++i) {
        u32 offset =
            elf_header->e_phoff + i * sizeof(elf32_phdr_t); // 计算当前程序头在文件中的偏移量。
        elf_read_phdr(fd, elf_proghs + i,
                      offset); // 函数读取程序头的内容，将其存储在程序头表中的相应位置。
    }

    void* entry_point = (void*)elf_header->e_entry;
    int resp = exec_load(fd, elf_header, elf_proghs);

    kern_kfree(elf_header);
    kern_kfree(elf_proghs);

    return resp == 0 ? entry_point : NULL;
}

/**
 * @brief 将elf文件加载到进程的内存中
 */
static int exec_load(u32 fd, const elf32_hdr_t* eh, const elf32_phdr_t* ph) {
    u32 ph_num;

    if (0 == eh->e_phnum) {
        kprintf("exec_load: elf ERROR!");
        return -1;
    }
    u32 text_lin_base = 0, text_lin_limit = 0, data_lin_base = 0, data_lin_limit = 0;
    const elf32_phdr_t* phdr;
    for (ph_num = 0, phdr = ph; ph_num < eh->e_phnum; ph_num++, phdr++) {
        if (phdr->p_type == PT_LOAD) {
            if (phdr->p_flags == 0x6) { // have read/write permission
                if (data_lin_base) {
                    data_lin_base = MIN(data_lin_base, phdr->p_vaddr);
                    data_lin_limit = MAX(data_lin_limit, phdr->p_vaddr + phdr->p_memsz);
                } else {
                    data_lin_base = phdr->p_vaddr;
                    data_lin_limit = phdr->p_vaddr + phdr->p_memsz;
                }
            } else if (phdr->p_flags & 0x4) { // no write, can read
                if (text_lin_base) {
                    text_lin_base = MIN(text_lin_base, phdr->p_vaddr);
                    text_lin_limit = MAX(text_lin_limit, phdr->p_vaddr + phdr->p_memsz);
                } else {
                    text_lin_base = phdr->p_vaddr;
                    text_lin_limit = phdr->p_vaddr + phdr->p_memsz;
                }
            }
        }
    }

    // new mmu
    for (ph_num = 0, phdr = ph; ph_num < eh->e_phnum; ph_num++, phdr++) {
        if (phdr->p_type == PT_LOAD) {
            // exec_elfcpy(fd, phdr);
            u32 prot = 0;
            if (phdr->p_flags & 4) prot |= PROT_READ;
            if (phdr->p_flags & 2) prot |= PROT_WRITE;
            if (phdr->p_flags & 1) prot |= PROT_EXEC;
            u32 in_page_offset = phdr->p_vaddr & 0xFFF;
            u32 mem_start = phdr->p_vaddr - in_page_offset;
            u32 mem_size = phdr->p_memsz + in_page_offset;
            u32 file_size = phdr->p_filesz + in_page_offset;
            u32 addr;
            u32 file_page_size = ROUNDUP(file_size, PGSIZE);
            if (file_page_size) {
                addr = kern_mmap_safe(mem_start, file_page_size, prot, MAP_PRIVATE | MAP_FIXED, fd,
                                      phdr->p_offset - in_page_offset);
                if (addr != mem_start) { kprintf("error: execve mmap file failed!"); }
            }
            if (mem_size > file_size) {          // bss
                if (mem_size > file_page_size) { // bss 超过elf映射的页，额外分配匿名页
                    addr = kern_mmap_safe(mem_start + file_page_size, mem_size - file_page_size,
                                          PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, -1, 0);
                    if (addr != mem_start + file_page_size) {
                        kprintf("error: execve mmap bss failed!");
                    }
                }
                memset((void*)(mem_start + file_size), 0, file_page_size - file_size);
            }
        }
    }
    p_proc_current->task.memmap.data_lin_base = data_lin_base;
    p_proc_current->task.memmap.data_lin_limit = data_lin_limit;
    p_proc_current->task.memmap.text_lin_base = text_lin_base;
    p_proc_current->task.memmap.text_lin_limit = text_lin_limit;
    return 0;
}

/**
 * @brief   重新初始化进程的寄存器和特权级、线性分布地址布局
 */
static int exec_pcb_init(const char* proc_name) {
    // 名称 状态 特权级 寄存器
    strcpy(p_proc_current->task.p_name, proc_name);
    init_user_cpu_context(&p_proc_current->task.context, proc2pid((p_proc_current)));

    // 进程表线性地址布局部分，text、data已经在前面初始化了
    p_proc_current->task.memmap.vpage_lin_base = VpageLinBase;
    p_proc_current->task.memmap.vpage_lin_limit = VpageLinBase;
    p_proc_current->task.memmap.heap_lin_base = HeapLinBase;
    p_proc_current->task.memmap.heap_lin_limit = HeapLinBase;
    p_proc_current->task.memmap.stack_child_limit = StackLinLimitMAX;
    p_proc_current->task.memmap.stack_lin_base = StackLinBase;
    p_proc_current->task.memmap.stack_lin_limit = StackLinBase - SZ_16K;
    p_proc_current->task.memmap.arg_lin_base = ArgLinBase;
    p_proc_current->task.memmap.arg_lin_limit = ArgLinLimitMAX;
    p_proc_current->task.memmap.kernel_lin_base = KernelLinBase; // 内核基址
    p_proc_current->task.memmap.kernel_lin_limit = KernelLinBase + kernel_size;
    list_init(&p_proc_current->task.memmap.vma_map);
    init_mem_page(&p_proc_current->task.memmap.anon_pages, MEMPAGE_AUTO);

    p_proc_current->task.tree_info.text_hold = true;
    p_proc_current->task.tree_info.data_hold = true;

    p_proc_current->task.nice = 0;
    p_proc_current->task.weight = nice_to_weight[p_proc_current->task.nice + 20];

    return 0;
}

static int exec_count_args(char* const* s, u32* count) {
    char* p = NULL;
    int space = 0;
    while (s && (p = *s++, p)) {
        space += strlen(p) + 1;
        (*count)++;
    }
    return space;
}

static int exec_copy_args(char* const* s, char** parr, char* dst, u32 offset) {
    char* p = NULL;
    while (s && (p = *s++, p)) {
        strcpy(dst, p);
        *parr++ = dst + offset;
        dst += strlen(p) + 1;
    }
    *parr++ = 0;
    return 0;
}

static const char* extract_program_name(const char* path) {
    const char* last_slash = strrchr(path, '/');
    const char* program_name = last_slash ? last_slash + 1 : path;
    return program_name;
}
