/**********************************************
*			exec.c 		add by visual 2016.5.23
*************************************************/

#include "type.h"
#include "const.h"
#include "string.h"
#include "elf.h"
#include "vfs.h"//added by xyx && wjh 2021-12-31
#include "buddy.h"// added by wang 2021.8.27

PRIVATE u32 exec_elfcpy(u32 fd, Elf32_Phdr *Echo_Phdr, u32 attribute);
PRIVATE u32 exec_load(u32 fd, const Elf32_Ehdr *Echo_Ehdr, const Elf32_Phdr *Echo_Phdr);
PRIVATE int exec_pcb_init(char *path);

//PUBLIC u32 do_execve(char *path);  deleted by xyx&&wjh 2021-12-31
//PUBLIC u32 kern_execve(char *path); deleted by xyx&&wjh 2021-12-31

// PRIVATE char *exec_path(char* path);//added by xyx&&wjh 2021.12.31
PUBLIC u32 do_execve(char *path, char *argv[], char *envp[ ]);//added by xyx&&wjh 2021.12.31
PUBLIC u32 kern_execve(char *path, char *argv[], char *envp[ ]);//added by xyx&&wjh 2021.12.31

/*  deleted by xyx&&wjh 2021-12-31   */
//added by mingxuan 2021-8-11
//PUBLIC u32 sys_execve(void *uesp)
//{
//	return do_execve(get_arg(uesp, 1));
//}

//PUBLIC u32 do_execve(char *path)
//{
	//return kern_execve(path);
//}
/*   end deleted  */


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
*                          sys_exec		add by visual 2016.5.23
*exec系统调用功能实现部分
*======================================================================*/
//PUBLIC u32 sys_execve(char *path)
//PUBLIC u32 do_execve(char *path)	//modified by mingxuan 2021-8-11
PUBLIC u32 kern_execve(char *path, char *argv[], char *envp[ ]) //modified by mingxuan 2021-8-11
{
	//disable_int();	//使用关中断的方法解决对sys_exec的互斥 //added by mingxuan 2021-1-31

	Elf32_Ehdr *Echo_Ehdr = kern_kmalloc(sizeof(Elf32_Ehdr));
	Elf32_Phdr *Echo_Phdr = kern_kmalloc(10 * sizeof(Elf32_Phdr));
	Elf32_Shdr *Echo_Shdr = kern_kmalloc(10 * sizeof(Elf32_Shdr));
	u32 addr_lin;
	u32 err_temp;
	u32 pde_addr_phy, addr_phy_temp;

	char *p_reg; //point to a register in the new kernel stack, added by xw, 17/12/11

	if (0 == path)
	{
		disp_color_str("exec: path ERROR!", 0x74);

		//如果该进程是通过fork得到的，exec加载失败后，该进程的state还是READY，调度器还会选中它。
		//因此需要把state设置为IDLE，否则会发生缺页。mingxuan 2021-1-30
		//p_proc_current->task.stat = IDLE; //added by mingxuan 2021-1-30
		//提醒父进程中的wait可以回收该进程，否则父进程会一直wait, mingxuan 2021-1-30
		//p_proc_current->task.we_flag = ZOMBY; //added by mingxuan 2021-1-30
		// p_proc_current->task.stat = ZOMBY;//modified by dongzhangqi 2023-6-2
		//因proc_stat和we_flag的改动而改动

		//enable_int();	//使用关中断的方法解决对sys_exec的互斥 //added by mingxuan 2021-1-31

		return -1;
	}


    //path = exec_path(path);//added by xyx&&wjh 2021.12.31

	/*******************打开文件************************/
	u32 fd = kern_vfs_open(path, O_RDONLY);

	if (fd == -1)
	{
		disp_str("sys_exec open error!\n"); //added by mingxuan 2020-12-22
		//printf("sys_exec open error!\n");	//deleted by mingxuan 2019-5-23

		//如果该进程是通过fork得到的，exec加载失败后，该进程的state还是READY，调度器还会选中它
		//因此需要把state设置为IDLE，否则会发生缺页, mingxuan 2021-1-30
		//p_proc_current->task.stat = IDLE; //added by mingxuan 2021-1-30

		//提醒父进程中的wait可以回收该进程，否则父进程会一直wait, mingxuan 2021-1-30
		//p_proc_current->task.we_flag = ZOMBY; //added by mingxuan 2021-1-30
		// p_proc_current->task.stat = ZOMBY;//modified by dongzhangqi 2023-6-2
		//因proc_stat和we_flag的改动而改动

		//enable_int();	//使用关中断的方法解决对sys_exec的互斥 //added by mingxuan 2021-1-31

		return -1;
	}
	// u32 fd = fake_open(path,"r");	//modified by xw, 18/5/30

	/*************获取elf信息**************/
	read_elf(fd, Echo_Ehdr, Echo_Phdr, Echo_Shdr); //注意第一个取了地址，后两个是数组，所以没取地址，直接用了数组名

    /*    added by xyx&&wjh   2021-12-31  */ //目前这个execve传参算是完全搞不懂了，自己写一个吧
    // char **p = argv;
	
	// char arg_stack[num_4B];
	// int stack_len = 0;

	// if (p != NULL)
	// {
	// 	while (*p++)
	// 	{
	// 		stack_len += sizeof(char *);
	// 	}

	// 	//disp_int(stack_len,0x71);

	// 	*((int*)(&arg_stack[stack_len])) = 0;
	// 	stack_len += sizeof(char*);
	// 	//disp_int(stack_len,0x74);
		
	// 	int tmp;
	// 	char **q = (char**)arg_stack;
	// 	for (p = argv; *p != 0; p++) {
	// 		*q++ = &arg_stack[stack_len];
	// 		strcpy(&arg_stack[stack_len], *p);
	// 		tmp = strlen(*p);
	// 		//disp_int(tmp,0x74);
	// 		stack_len += tmp;

	// 		arg_stack[stack_len] = 0;
	// 		stack_len++;
	// 		//disp_int(stack_len,0x74);
	// 	}
	// 	u8* orig_stack = (u8*)(ArgLinBase);

	// 	//disp_int(orig_stack);

	// 	int delta = (int)(void*)arg_stack - (int)orig_stack;
	// 	//disp_int(delta, 0x71);
	// 	int argc = 0;
	// 	if (stack_len) {	// has args
	// 		char **q = (char**)arg_stack;
	// 		for (; *q != 0; q++,argc++)
	// 			*q -= delta;
	// 	}
	// }
	// 处理参数 argc 放在 ebp + 8 (ArgLinBase - 4); argv 放在 ebp + 12
	// err_temp = ker_umalloc_4k(ArgLinBase, p_proc_current->task.pid, PG_P | PG_USU | PG_RWW);
	// low<--               ---stack---           -->high(base)
	//  | user main ebp | _start rt |        argc           |  argv    | argv[0]... argv[argc-1]|... argv raw data
	//  |               |           | StackLinBase(pcb esp) |ArgLinBase|
	int argc = 0, len = 0;
	for(char** p = argv; p != 0 && *p != 0; p++){
		argc++;
	}
	char** args_base = (char**)(ArgLinBase + 4);
	char* args_raw_base = ((char*)args_base) + num_4B * (argc + 1);
	*(int*)(ArgLinBase-4) = argc;
	*(char***)(ArgLinBase) = args_base;
	for(char** p = argv; p != 0 && *p != 0; p++){
		len = strlen(*p);
		strcpy(args_raw_base, *p);
		*(args_base++) = args_raw_base;
		args_raw_base += len + 1;
	}
	*args_base = 0;
	
     /*   end added   */

	/*************释放进程内存****************/
	//目前还没有实现 思路是：数据、代码根据text_info和data_info属性决定释放深度，其余内存段可以完全释放

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

		return -1; //使用了const指针传递
	}
	//disp_free();	//for test, added by mingxuan 2021-1-7

	/*****************重新初始化该进程的进程表信息（包括LDT）、线性地址布局、进程树属性********************/
	exec_pcb_init(path);
	// addr_lin = p_proc_current->task.memmap.arg_lin_base ;
	// memcpy(addr_lin, arg_stack, num_4K);

	// /*    added by xyx&&wjh 2021-12-31  */
	// for( addr_lin = p_proc_current->task.memmap.arg_lin_base ; addr_lin < p_proc_current->task.memmap.arg_lin_limit ; addr_lin+=num_4K )
	// {
	// 	err_temp = ker_umalloc_4k(addr_lin, p_proc_current->task.pid, PG_P | PG_USU | PG_RWW);
		
	// 	if( err_temp!=0 )
	// 	{
	// 		disp_color_str("kernel_main Error:lin_mapping_phy",0x74);
	// 		//如果该进程是通过fork得到的，exec加载失败后，该进程的state还是READY，调度器还会选中它。
	// 		//因此需要把state设置为IDLE，否则会发生缺页。mingxuan 2021-1-30
	// 		p_proc_current->task.stat = IDLE; //added by mingxuan 2021-1-30

	// 		//提醒父进程中的wait可以回收该进程，否则父进程会一直wait, mingxuan 2021-1-30
	// 		p_proc_current->task.we_flag = ZOMBY; //added by mingxuan 2021-1-30

	// 		//enable_int();	//使用关中断的方法解决对sys_exec的互斥 //added by mingxuan 2021-1-31
	// 		return -1;
	// 	}
	// 	memcpy((void*)addr_lin, (void*)arg_stack, num_4K);
	// }
   /*   end added   */

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

/*    added by xyx&&wjh  2021-12-31  */
	// p_proc_current->task.regs.ecx = argc; /* argc */
	// *((u32*)(p_reg + ECXREG - P_STACKTOP)) = p_proc_current->task.regs.ecx;
	// p_proc_current->task.regs.edx = (u32)orig_stack; /* argv */
	// *((u32*)(p_reg + EDXREG - P_STACKTOP)) = p_proc_current->task.regs.edx;
/*   end added   */


	//u32 stack_lin_base = addr_lin=p_proc_current->task.memmap.stack_lin_base - num_4K;	//added vy mingxuan 2021-8-24
	for (addr_lin = p_proc_current->task.memmap.stack_lin_base; addr_lin > p_proc_current->task.memmap.stack_lin_limit; addr_lin -= num_4K)
	{
		/*
		err_temp = lin_mapping_phy(	addr_lin,//线性地址						//add by visual 2016.5.9
									MAX_UNSIGNED_INT,//物理地址						//edit by visual 2016.5.19
									p_proc_current->task.pid,//进程pid			//edit by visual 2016.5.19
									PG_P  | PG_USU | PG_RWW,//页目录的属性位
									PG_P  | PG_USU | PG_RWW);//页表的属性位
*/
		//进程栈上分配物理页，所以调用ker_umalloc_4k,  wang 2021.8.27
		err_temp = ker_umalloc_4k(addr_lin, p_proc_current->task.pid, PG_P | PG_USU | PG_RWW); //edited by wang 2021.8.27
		if (err_temp != 0)
		{
			disp_color_str("kernel_main Error:lin_mapping_phy", 0x74);

			//如果该进程是通过fork得到的，exec加载失败后，该进程的state还是READY，调度器还会选中它。
			//因此需要把state设置为IDLE，否则会发生缺页。mingxuan 2021-1-30
			//p_proc_current->task.stat = IDLE; //added by mingxuan 2021-1-30

			//提醒父进程中的wait可以回收该进程，否则父进程会一直wait, mingxuan 2021-1-30
			//p_proc_current->task.we_flag = ZOMBY; //added by mingxuan 2021-1-30
			// p_proc_current->task.stat = ZOMBY;//modified by dongzhangqi 2023-6-2
			//因proc_stat和we_flag的改动而改动

			//enable_int();	//使用关中断的方法解决对sys_exec的互斥 //added by mingxuan 2021-1-31

			return -1;
		}
	}

	//added by mingxuan 2021-8-24, for test
	//u32 page_phy_addr = get_page_phy_addr(p_proc_current->task.pid, p_proc_current->task.memmap.stack_lin_limit);
	//page_phy_addr = get_page_phy_addr(p_proc_current->task.pid, p_proc_current->task.memmap.stack_lin_limit + num_4B);
	//page_phy_addr = get_page_phy_addr(p_proc_current->task.pid, p_proc_current->task.memmap.stack_lin_limit + num_4B + num_4B);

	//堆    用户还没有申请，所以没有分配，只在PCB表里标示了线性起始位置

	kern_vfs_close(fd);
	//disp_color_str("\n[exec success:",0x72);//灰底绿字
	//disp_color_str(path,0x72);//灰底绿字
	//disp_color_str("]",0x72);//灰底绿字
	//disp_free();	//for test, added by mingxuan 2021-1-7

	//enable_int();	//使用关中断的方法解决对sys_exec的互斥 //added by mingxuan 2021-1-31

	return 0;
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
PRIVATE u32 exec_elfcpy(u32 fd, Elf32_Phdr *Echo_Phdr, u32 attribute) // 这部分代码将来要移动到exec.c文件中，包括下面execve()中的一部分
{
	u32 lin_addr = Echo_Phdr->p_vaddr;
	u32 lin_limit = Echo_Phdr->p_vaddr + Echo_Phdr->p_memsz;

	u32 file_offset = Echo_Phdr->p_offset;
	u32 file_limit = Echo_Phdr->p_offset + Echo_Phdr->p_filesz;

	u32 lin_file_limit = Echo_Phdr->p_vaddr + Echo_Phdr->p_filesz; //added by mingxuan 2021-3-16

	/*	//deleted by mingxuan 2021-1-29
	u8 ch;
	//u32 pde_addr_phy = get_pde_phy_addr(p_proc_current->task.pid); //页目录物理地址			//delete by visual 2016.5.19
	//u32 addr_phy = do_malloc(Echo_Phdr->p_memsz);//申请物理内存					//delete by visual 2016.5.19

	for(  ; lin_addr<lin_limit ; lin_addr++,file_offset++ )
	{
		lin_mapping_phy(lin_addr,MAX_UNSIGNED_INT,p_proc_current->task.pid,PG_P  | PG_USU | PG_RWW,attribute);//说明：PDE属性尽量为读写，因为它要映射1024个物理页，可能既有数据，又有代码	//edit by visual 2016.5.19
		if( file_offset<file_limit )
		{//文件中还有数据，正常拷贝
			//modified by xw, 18/5/30
			// seek(file_offset);
			// read(fd,&ch,1);

			//fake_seek(file_offset); //deleted by mingxuan 2019-5-22
			//real_lseek(fd, file_offset, SEEK_SET); //modified by mingxuan 2019-5-22
			do_vlseek(fd, file_offset, SEEK_SET);	//modified by mingxuan 2019-5-24

			//fake_read(fd,&ch,1); //deleted by mingxuan 2019-5-22
			//real_read(fd, &ch, 1); //modified by mingxuan 2019-5-22
			do_vread(fd, &ch, 1);	//modified by mingxuan 2019-5-24
			//~xw	file_offset += num_4K;	//added by mingxuan 2021-3-9

			*((u8*)lin_addr) = ch;//memcpy((void*)lin_addr,&ch,1);
		}
		else
		{
			//已初始化数据段拷贝完毕，剩下的是未初始化的数据段，在内存中填0
			*((u8*)lin_addr) = 0;//memset((void*)lin_addr,0,1);
		}
	}
	*/

	// modified by mingxuan 2021-1-29, start
	char *buf = kern_kmalloc_4k();; // added by mingxuan 2020-12-14

	// added by mingxuan 2020-12-14
	// 给lin_addr建立页映射, mingxuan
//	for (; lin_addr < lin_limit; lin_addr++, file_offset++)
	for (; lin_addr < lin_limit; lin_addr+=num_4K, file_offset+=num_4K)
	{
		//lin_mapping_phy(lin_addr, MAX_UNSIGNED_INT, p_proc_current->task.pid, PG_P | PG_USU | PG_RWW, attribute);
		ker_umalloc_4k(lin_addr,p_proc_current->task.pid,attribute);           //edited by wang 2021.8.27
	}

	lin_addr = Echo_Phdr->p_vaddr;	  // added by mingxuan 2020-12-14
	file_offset = Echo_Phdr->p_offset; // added by mingxuan 2020-12-14

	u32 for_flag; //added by mingxuan 2021-3-16
	for_flag = 0; //added by mingxuan 2021-8-8

	//for(  ; lin_addr<lin_limit ; lin_addr+=num_4K,file_offset+=num_4K )	// modified by mingxuan 2020-12-14
	for (; lin_addr < lin_limit, file_offset < file_limit; lin_addr += num_4K, file_offset += num_4K) // modified by mingxuan 2021-3-16
	{
		for_flag = 1; //表示进入了for循环，离开for循环时需要做变量调整 //added by mingxuan 2021-3-16

		//以4K个字节为一个单位进行拷贝. 剩余的字节数小于4K字节，则一次全拷完剩余的字节, mingxuan
		//if( lin_limit-lin_addr >= num_4K )	// modified by mingxuan 2020-12-14
		if (lin_file_limit - lin_addr >= num_4K) // modified by mingxuan 2021-3-16
		{					
			kern_vfs_lseek(fd, file_offset, SEEK_SET); //modified by mingxuan 2021-8-19
			kern_vfs_read(fd, buf, num_4K);
			memcpy(lin_addr, buf, num_4K); //modified by mingxuan 2020-12-14
		}
		else
		{ //剩余的字节数小于4K字节，则一次全拷完剩余的字节, mingxuan

			//do_vlseek(fd, file_offset, SEEK_SET); // added by mingxuan 2020-12-14

			//memcpy(buf, 0, num_4K);	//给buf清除脏数据,清0	// added by mingxuan 2020-12-14 //deleted by mingxuan 2020-12-18
			u32 left_size = file_limit - file_offset;
			kern_vfs_lseek(fd, file_offset, SEEK_SET); //modified by mingxuan 2021-8-19
			kern_vfs_read(fd, buf, left_size);
			memcpy(lin_addr, buf, left_size); //modified by mingxuan 2021-3-16

			file_offset = file_offset + left_size; //added by mingxuan 2021-3-16
		}
	}
	// modified by mingxuan 2021-1-29, end

	//added by mingxuan 2021-3-16
	if (for_flag == 1) //表示进入了for循环，离开for循环时需要做变量调整
	{
		//减4K的原因是，最后一次for循环在判断时会自动加上4K
		lin_addr -= num_4K;	   //added by mingxuan 2021-3-16
		file_offset -= num_4K; //added by mingxuan 2021-3-16
	}

	//added by mingxuan 2021-3-16
	if (lin_limit - lin_file_limit > 0)
	{
		memset(lin_file_limit, 0, lin_limit - lin_file_limit);
	}

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

	//added by mingxuan 2021-1-31
	u32 min_text_lin_base = 0x8048000; //用来处理text_lin_*存在多个段的情况
	u32 max_text_lin_base = 0x8048000; //用来处理text_lin_*存在多个段的情况

	//我们还不能确定elf中一共能有几个program，但就目前我们查看过的elf文件中，只出现过两中program，一种.text（R-E）和一种.data（RW-）
	for (ph_num = 0; ph_num < Echo_Ehdr->e_phnum; ph_num++)
	{
		if (0 == Echo_Phdr[ph_num].p_memsz)
		{ //最后一个program
			break;
		}

		//if( Echo_Phdr[ph_num].p_flags == 0x5 ) //0x101,可读可执行
		if (Echo_Phdr[ph_num].p_flags == 0x4 || Echo_Phdr[ph_num].p_flags == 0x5) //100b:只读; 101b:可读可执行	//modified by mingxuan 2021-1-30
		//这样修改的原因：把.text, .rodata, .frame合并，一起赋给text_lin_*, mingxuan 2021-1-31
		{ //.text .rodata .eh_frame

			//added by mingxuan 2020-12-25
			if (p_proc_current->task.pid >= NR_K_PCBS) //前4个是系统进程,pid大于4的是用户进程 此处应使用宏，硬编码之后进程数都变了也找不到位置改
			{
				//清空子进程对代码段的映射的所有页表项
				//modified by mingxuan 2021-1-4
				u32 addr_lin = 0;
				for (addr_lin = Echo_Phdr[ph_num].p_vaddr; addr_lin < Echo_Phdr[ph_num].p_vaddr + Echo_Phdr[ph_num].p_memsz; addr_lin += num_4K)
				{
					clear_pte(p_proc_current->task.pid, addr_lin);
				}
			}

			exec_elfcpy(fd, &Echo_Phdr[ph_num], PG_P | PG_USU | PG_RWR); //进程代码段

			//modified by mingxuan 2021-1-30
			//如果存在多个段(.text .rodata .eh_frame),text_lin_base一定是最低端地址
			if (Echo_Phdr[ph_num].p_vaddr <= min_text_lin_base)
			{
				min_text_lin_base = Echo_Phdr[ph_num].p_vaddr;
				p_proc_current->task.memmap.text_lin_base = min_text_lin_base;
			}
			//如果存在多个段(.text .rodata .eh_frame),text_lin_limit一定是最高地址的lin_base+最高地址的段的size
			if (Echo_Phdr[ph_num].p_vaddr >= max_text_lin_base)
			{
				max_text_lin_base = Echo_Phdr[ph_num].p_vaddr;
				p_proc_current->task.memmap.text_lin_limit = max_text_lin_base + Echo_Phdr[ph_num].p_memsz;
			}
		}
		else if (Echo_Phdr[ph_num].p_flags == 0x6)						//110，读写
		{																//.data .bss
			exec_elfcpy(fd, &Echo_Phdr[ph_num], PG_P | PG_USU | PG_RWW); //进程数据段
			p_proc_current->task.memmap.data_lin_base = Echo_Phdr[ph_num].p_vaddr;
			p_proc_current->task.memmap.data_lin_limit = Echo_Phdr[ph_num].p_vaddr + Echo_Phdr[ph_num].p_memsz;
		}

		else
		{
			disp_color_str("exec_load: unKnown elf'program!", 0x74);
			return -1;
		}
	}
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
	//p_proc_current->task.memmap.arg_lin_limit = ArgLinBase;	deleted byxyx&&wjh  2021-12-31				   //参数内存界限
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
