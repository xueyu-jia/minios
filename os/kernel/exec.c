/**********************************************
*			exec.c 		add by visual 2016.5.23
*************************************************/

#include "type.h"
#include "const.h"
#include "string.h"
#include "elf.h"
#include "vfs.h"//added by xyx && wjh 2021-12-31
#include "buddy.h"// added by wang 2021.8.27
#include "memman.h"
#include "pagetable.h"

PRIVATE u32 exec_elfcpy(u32 fd, Elf32_Phdr *Echo_Phdr);
PRIVATE u32 exec_load(u32 fd, const Elf32_Ehdr *Echo_Ehdr, const Elf32_Phdr *Echo_Phdr);
PRIVATE int exec_pcb_init(char *path);

PUBLIC u32 do_execve(char *path, char *argv[], char *envp[ ]);//added by xyx&&wjh 2021.12.31
PUBLIC u32 kern_execve(char *path, char *argv[], char *envp[ ]);//added by xyx&&wjh 2021.12.31


/*    added by xyx&&wjh 2021.12.31  */
/*======================================================================*
*                          sys_execv		
*execv系统调用功能实现部分
*======================================================================*/
PUBLIC u32 sys_execv()
{
	return do_execve(get_arg(1), get_arg(2), NULL);
}

/*======================================================================*
*                          sys_execvp		
*execvp系统调用功能实现部分
*======================================================================*/
PUBLIC u32 sys_execvp()
{
	return do_execve(get_arg(1), get_arg(2), NULL);
}

PUBLIC u32 sys_execve()
{
	return do_execve(get_arg(1), get_arg(2), get_arg(3));
}

PUBLIC u32 do_execve(char *path, char *argv[], char *envp[ ])
{
	return kern_execve(path, argv, envp);
}
/*   end added   */


/*======================================================================*
*                          kern_exec		add by visual 2016.5.23
*exec系统调用功能实现部分
*======================================================================*/
PUBLIC u32 kern_execve(char *path, char *argv[], char *envp[ ]) //modified by mingxuan 2021-8-11
{
	u32 addr_lin;
	u32 err_temp;
	u32 pde_addr_phy, addr_phy_temp;

	char *p_reg; //point to a register in the new kernel stack, added by xw, 17/12/11

	if (0 == path)
	{
		disp_color_str("exec: path ERROR!", 0x74);
		return -1;
	}

	/*******************打开文件************************/
	u32 fd = kern_vfs_open(path, O_RDONLY);

	if (fd == -1)
	{
		disp_str("sys_exec open error!\n"); //added by mingxuan 2020-12-22
		return -1;
	}

	Elf32_Ehdr *Echo_Ehdr = kern_kmalloc(sizeof(Elf32_Ehdr));
	Elf32_Phdr *Echo_Phdr = kern_kmalloc(10 * sizeof(Elf32_Phdr));
	Elf32_Shdr *Echo_Shdr = kern_kmalloc(10 * sizeof(Elf32_Shdr));

	/*************获取elf信息**************/
	read_elf(fd, Echo_Ehdr, Echo_Phdr, Echo_Shdr); //注意第一个取了地址，后两个是数组，所以没取地址，直接用了数组名

    //原有execve传参重构，自己写一个吧 2023.12.25
	// 处理参数 argc 放在 ebp + 8 (ArgLinBase - 4); argv 放在 ebp + 12
	// err_temp = ker_umalloc_4k(ArgLinBase, p_proc_current->task.pid, PG_P | PG_USU | PG_RWW);
	// low<--               ---stack---           -->high(base)
	//  | user main ebp | _start rt |        argc           |  argv    | argv[0]... argv[argc-1]|... argv raw data
	//  |               |           | StackLinBase(pcb esp) |ArgLinBase|
	int argc = 0, len = 0, arg_content = 0;
	for(char** p = argv; p != 0 && *p != 0; p++){
		arg_content += strlen(*p) + 1;
		argc++;
	}
	// 补充: 参数数量不能超过最大值
	if(argc > MAXARG || ((argc + 2)*4 + arg_content > num_4K)) {
		goto close_on_error;
	}
	char** args_base = (char**)(ArgLinBase + 4);
	char* args_raw_base = ((char*)args_base) + num_4B * (argc + 1);
	// *(int*)(ArgLinBase-4) = argc;
	*(char***)(ArgLinBase) = args_base;
	for(char** p = argv; p != 0 && *p != 0; p++){
		len = strlen(*p);
		strcpy(args_raw_base, *p);
		*(args_base++) = args_raw_base;
		args_raw_base += len + 1;
	}
	*args_base = 0;
	
	/*************释放进程内存****************/
	//目前还没有实现 思路是：数据、代码根据text_info和data_info属性决定释放深度，其余内存段可以完全释放
	int pid = p_proc_current->task.pid;
	free_seg_phypage(pid, MEMMAP_TEXT);
	free_seg_phypage(pid, MEMMAP_DATA);
	/*************根据elf的program复制文件信息**************/
	//disp_free();	//for test, added by mingxuan 2021-1-7
	if (-1 == exec_load(fd, Echo_Ehdr, Echo_Phdr))
	{
		disp_str("exec_load error!\n"); //added by mingxuan 2020-12-22

		//如果该进程是通过fork得到的，exec加载失败后，该进程的state还是READY，调度器还会选中它。
		//因此需要把state设置为IDLE，否则会发生缺页。mingxuan 2021-1-30
		//p_proc_current->task.stat = IDLE; //added by mingxuan 2021-1-30

		//提醒父进程中的wait可以回收该进程，否则父进程会一直wait, mingxuan 2021-1-30
		//p_proc_current->task.we_flag = ZOMBY; //added by mingxuan 2021-1-30
		// p_proc_current->task.stat = ZOMBY;//modified by dongzhangqi 2023-6-2
		//因proc_stat和we_flag的改动而改动

		//enable_int();	//使用关中断的方法解决对sys_exec的互斥 //added by mingxuan 2021-1-31

		goto close_on_error;
	}
	//disp_free();	//for test, added by mingxuan 2021-1-7

	/*****************重新初始化该进程的进程表信息（包括LDT）、线性地址布局、进程树属性********************/
	exec_pcb_init(path);
	free_seg_phypage(pid, MEMMAP_VPAGE);
	free_seg_phypage(pid, MEMMAP_HEAP);
	free_seg_phypage(pid, MEMMAP_STACK);
	/***********************代码、数据、堆、栈***************************/
	//代码、数据已经处理，将eip重置即可
	//p_proc_current->task.regs.eip = Echo_Ehdr->e_entry;						 //进程入口线性地址 deleted by lcy 2023.10.25

	p_reg = (char *)(p_proc_current + 1);									 //added by xw, 17/12/11
	//*((u32 *)(p_reg + EIPREG - P_STACKTOP)) = p_proc_current->task.regs.eip; //added by xw, 17/12/11  deleted by lcy 2023.10.25
	*((u32 *)(p_reg + EIPREG - P_STACKTOP)) = Echo_Ehdr->e_entry;	

	//test_kbud_mem_size();

	//栈deleted by lcy 2023.10.25
	//p_proc_current->task.regs.esp = (u32)p_proc_current->task.memmap.stack_lin_base; //栈地址最高处
	//*((u32 *)(p_reg + ESPREG - P_STACKTOP)) = p_proc_current->task.regs.esp;		 //added by xw, 17/12/11
	*((u32 *)(p_reg + ESPREG - P_STACKTOP)) = (u32)p_proc_current->task.memmap.stack_lin_base; //added by lcy 2023.10.25


	//u32 stack_lin_base = addr_lin=p_proc_current->task.memmap.stack_lin_base - num_4K;	//added vy mingxuan 2021-8-24
	for (addr_lin = p_proc_current->task.memmap.stack_lin_limit; 
		addr_lin < UPPER_BOUND_4K(p_proc_current->task.memmap.stack_lin_base); addr_lin += num_4K)
	{
		//进程栈上分配物理页，所以调用ker_umalloc_4k,  wang 2021.8.27
		err_temp = ker_umalloc_4k(addr_lin, p_proc_current->task.pid, PG_P | PG_USU | PG_RWW); //edited by wang 2021.8.27
		if (err_temp != 0)
		{
			disp_color_str("kernel_main Error:lin_mapping_phy", 0x74);

			goto close_on_error;
		}
	}

	*(int*)(ArgLinBase-4) = argc;

	kern_vfs_close(fd);
	//disp_color_str("\n[exec success:",0x72);//灰底绿字
	//disp_color_str(path,0x72);//灰底绿字
	//disp_color_str("]",0x72);//灰底绿字
	//disp_free();	//for test, added by mingxuan 2021-1-7

	return 0;
fatal_error:
	kern_kfree(Echo_Ehdr);
	kern_kfree(Echo_Phdr);
	kern_kfree(Echo_Shdr);
	kern_exit(-1);
close_on_error:
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
PRIVATE u32 exec_elfcpy(u32 fd, Elf32_Phdr *Echo_Phdr) // 这部分代码将来要移动到exec.c文件中，包括下面execve()中的一部分
{
	u32 lin_addr = Echo_Phdr->p_vaddr;
	u32 lin_limit = Echo_Phdr->p_vaddr + Echo_Phdr->p_memsz;

	u32 file_offset = Echo_Phdr->p_offset;
	u32 file_limit = Echo_Phdr->p_offset + Echo_Phdr->p_filesz;

	u32 lin_file_limit = Echo_Phdr->p_vaddr + Echo_Phdr->p_filesz; //added by mingxuan 2021-3-16

	// modified by mingxuan 2021-1-29, start
	char *buf = kern_kmalloc_4k();; // added by mingxuan 2020-12-14

	// added by mingxuan 2020-12-14
	// 给lin_addr建立页映射, mingxuan
	//	for (; lin_addr < lin_limit; lin_addr++, file_offset++)
	// 20240402: 此处如果不能保证elf中的地址4K对齐，会导致最高一部分没有分配空间
	// 如lin_addr=0x8051FF4, lin_limit=0x805C220 末尾0x805C000~0x805C220未分配
	// 下面的复制数据同样是这样的写法, 但是复制数据是应当按照elf中的位置完全对应的，最后一次复制也做了处理，所以没有问题
	// for (; lin_addr < lin_limit; lin_addr+=num_4K, file_offset+=num_4K)
	// 页表操作移到exec_load

	lin_addr = Echo_Phdr->p_vaddr;	  // added by mingxuan 2020-12-14
	file_offset = Echo_Phdr->p_offset; // added by mingxuan 2020-12-14

	u32 for_flag; //added by mingxuan 2021-3-16
	for_flag = 0; //added by mingxuan 2021-8-8

	//for(  ; lin_addr<lin_limit ; lin_addr+=num_4K,file_offset+=num_4K )	// modified by mingxuan 2020-12-14
	for (; lin_addr < lin_limit && file_offset < file_limit; ) // modified by mingxuan 2021-3-16
	{
		for_flag = 1; //表示进入了for循环，离开for循环时需要做变量调整 //added by mingxuan 2021-3-16

		//以4K个字节为一个单位进行拷贝. 剩余的字节数小于4K字节，则一次全拷完剩余的字节, mingxuan
		//if( lin_limit-lin_addr >= num_4K )	// modified by mingxuan 2020-12-14
		if (lin_file_limit - lin_addr >= num_4K) // modified by mingxuan 2021-3-16
		{					
			kern_vfs_lseek(fd, file_offset, SEEK_SET); //modified by mingxuan 2021-8-19
			kern_vfs_read(fd, buf, num_4K);
			memcpy(lin_addr, buf, num_4K); //modified by mingxuan 2020-12-14
			file_offset += num_4K;
			lin_addr += num_4K;
		}
		else
		{ //剩余的字节数小于4K字节，则一次全拷完剩余的字节, mingxuan

			u32 left_size = file_limit - file_offset;
			kern_vfs_lseek(fd, file_offset, SEEK_SET); //modified by mingxuan 2021-8-19
			kern_vfs_read(fd, buf, left_size);
			memcpy(lin_addr, buf, left_size); //modified by mingxuan 2021-3-16
			file_offset += left_size; //added by mingxuan 2021-3-16
			lin_addr += left_size;
		}
	}
	// modified by mingxuan 2021-1-29, end

	//added by mingxuan 2021-3-16
	// if (for_flag == 1) //表示进入了for循环，离开for循环时需要做变量调整
	// {
	// 	//减4K的原因是，最后一次for循环在判断时会自动加上4K
	// 	lin_addr -= num_4K;	   //added by mingxuan 2021-3-16
	// 	file_offset -= num_4K; //added by mingxuan 2021-3-16
	// }
	// 后面 lin_addr 和 file_offset都不再使用 20240402
	//added by mingxuan 2021-3-16
	if (lin_limit - lin_file_limit > 0)
	{
		memset(lin_file_limit, 0, lin_limit - lin_file_limit);
	}
	kern_kfree_4k(buf);

	return 0;
}

/*======================================================================*
*                          exec_load		add by visual 2016.5.23
*根据elf的program复制文件信息
*======================================================================*/
PRIVATE u32 exec_load(u32 fd, const Elf32_Ehdr *Echo_Ehdr, const Elf32_Phdr *Echo_Phdr)
{
	u32 ph_num;

	if (0 == Echo_Ehdr->e_phnum)
	{
		disp_color_str("exec_load: elf ERROR!", 0x74);
		return -1;
	}
	u32 text_lin_base = 0, text_lin_limit = 0, data_lin_base = 0, data_lin_limit = 0;
	Elf32_Phdr* phdr;
	for (ph_num = 0, phdr = Echo_Phdr; ph_num < Echo_Ehdr->e_phnum; ph_num++, phdr++) {
		if (phdr->p_type == ELF_LOAD) {
			if(phdr->p_flags == 0x6) { // have read/write permission
				if(data_lin_base) {
					data_lin_base = min(data_lin_base, phdr->p_vaddr);
					data_lin_limit = max(data_lin_limit, phdr->p_vaddr + phdr->p_memsz);
				} else {
					data_lin_base = phdr->p_vaddr;
					data_lin_limit = phdr->p_vaddr + phdr->p_memsz;
				}
			} else if(phdr->p_flags & 0x4){ // no write, can read
				if(text_lin_base) {
					text_lin_base = min(text_lin_base, phdr->p_vaddr);
					text_lin_limit = max(text_lin_limit, phdr->p_vaddr + phdr->p_memsz);
				} else {
					text_lin_base = phdr->p_vaddr;
					text_lin_limit = phdr->p_vaddr + phdr->p_memsz;
				}
			}
		}
	}
	u32 lin_addr;
	for (lin_addr = data_lin_base; lin_addr < UPPER_BOUND_4K(data_lin_limit); lin_addr += num_4K)
	{
		ker_umalloc_4k(lin_addr,p_proc_current->task.pid, PG_P | PG_USU | PG_RWW);
		// disp_int(lin_addr);
		// disp_str("=>");
		// disp_int(get_page_phy_addr(p_proc_current->task.pid, lin_addr));
		// disp_str("\n");
	}
	for (lin_addr = text_lin_base; lin_addr < UPPER_BOUND_4K(text_lin_limit); lin_addr += num_4K)
	{
		ker_umalloc_4k(lin_addr,p_proc_current->task.pid, PG_P | PG_USU | PG_RWR);
	}
	for (ph_num = 0, phdr = Echo_Phdr; ph_num < Echo_Ehdr->e_phnum; ph_num++, phdr++) {
		if (phdr->p_type == ELF_LOAD) {
			exec_elfcpy(fd, phdr);
		}
	}
	p_proc_current->task.memmap.data_lin_base = data_lin_base;
	p_proc_current->task.memmap.data_lin_limit = data_lin_limit;
	p_proc_current->task.memmap.text_lin_base = text_lin_base;
	p_proc_current->task.memmap.text_lin_limit = text_lin_limit;
	return 0;
}

/*======================================================================*
*                          exec_init		add by visual 2016.5.23
* 重新初始化寄存器和特权级、线性地址布局
*======================================================================*/
PRIVATE int exec_pcb_init(char *path)
{
	char *p_regs; //point to registers in the new kernel stack, added by xw, 17/12/11

	//名称 状态 特权级 寄存器
	strcpy(p_proc_current->task.p_name, path);						   //名称
	p_proc_current->task.stat = READY;								   //状态
	p_proc_current->task.ldts[0].attr1 = DA_C | PRIVILEGE_USER << 5;   //特权级修改为用户级
	p_proc_current->task.ldts[1].attr1 = DA_DRW | PRIVILEGE_USER << 5; //特权级修改为用户级

	/***************copy registers data****************************/
	//copy registers data to the bottom of the new kernel stack
	//added by xw, 17/12/11
	p_regs = (char *)(p_proc_current + 1);
	p_regs -= P_STACKTOP;
	p_proc_current->task.esp_save_int = (STACK_FRAME*)p_regs;
	p_proc_current->task.esp_save_int->cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
	p_proc_current->task.esp_save_int->ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
	p_proc_current->task.esp_save_int->es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
	p_proc_current->task.esp_save_int->fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
	p_proc_current->task.esp_save_int->ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
	p_proc_current->task.esp_save_int->gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_USER;
	p_proc_current->task.esp_save_int->eflags = 0x202; /* IF=1,bit2 永远是1 */
	//memcpy(p_regs, (char *)p_proc_current, 18 * 4);

	//进程表线性地址布局部分，text、data已经在前面初始化了
	p_proc_current->task.memmap.vpage_lin_base = VpageLinBase;				   //保留内存基址
	p_proc_current->task.memmap.vpage_lin_limit = VpageLinBase;				   //保留内存界限
	p_proc_current->task.memmap.heap_lin_base = HeapLinBase;				   //堆基址
	p_proc_current->task.memmap.heap_lin_limit = HeapLinBase;				   //堆界限
	p_proc_current->task.memmap.stack_child_limit = StackLinLimitMAX;		   //add by visual 2016.5.27
	p_proc_current->task.memmap.stack_lin_base = StackLinBase;				   //栈基址
	p_proc_current->task.memmap.stack_lin_limit = StackLinBase - 0x4000;	   //栈界限（使用时注意栈的生长方向）
	p_proc_current->task.memmap.arg_lin_base = ArgLinBase;					   //参数内存基址
	p_proc_current->task.memmap.arg_lin_limit =  ArgLinLimitMAX;//added byxyx&&wjh  2021-12-31		
	p_proc_current->task.memmap.kernel_lin_base = KernelLinBase;			   //内核基址
	p_proc_current->task.memmap.kernel_lin_limit = KernelLinBase + kernel_size; //内核大小初始化为8M

	//进程树属性,只要改两项，其余不用改
	//p_proc_current->task.info.type = TYPE_PROCESS;			//当前是进程还是线程
	//p_proc_current->task.info.real_ppid = -1;  	//亲父进程，创建它的那个进程
	//p_proc_current->task.info.ppid = -1;			//当前父进程
	//p_proc_current->task.info.child_p_num = 0;	//子进程数量
	//p_proc_current->task.info.child_process[NR_CHILD_MAX];//子进程列表
	//p_proc_current->task.info.child_t_num = 0;		//子线程数量
	//p_proc_current->task.info.child_thread[NR_CHILD_MAX];//子线程列表
	p_proc_current->task.info.text_hold = 1; //是否拥有代码
	p_proc_current->task.info.data_hold = 1; //是否拥有数据

	return 0;
}
