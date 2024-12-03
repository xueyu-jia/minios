#include <kernel/ahci.h>
#include <kernel/blame.h>
#include <kernel/buddy.h>
#include <kernel/clock.h>
#include <kernel/console.h>
#include <kernel/const.h>
#include <kernel/dev.h>
#include <kernel/devfs.h>
#include <kernel/hd.h>
#include <kernel/keyboard.h>
#include <kernel/mmap.h>
#include <kernel/pagetable.h>
#include <kernel/proc.h>
#include <kernel/proto.h>
#include <kernel/shm.h>
#include <kernel/type.h>
#ifndef OPT_DISP_SERIAL
// #define GDBSTUB
#endif
#include "../gdbstub/gdbstub.h"

PRIVATE int initialize_processes();  // added by xw, 18/5/26
PRIVATE int initialize_cpus();       // added by xw, 18/6/2

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main() {
#ifdef GDBSTUB
  gdb_sys_init();
#endif
  int error;

  // kernel_initial = 1; //kernel is in initial state. added by xw, 18/5/31
  // moved to cstart begin
  // zcr added(清屏)
  disp_pos = 0;
  for (int i = 0; i < 25; i++) {
    for (int j = 0; j < 80; j++) {
      disp_str(" ");
    }
  }
  disp_pos = 0;

  disp_str("-----Kernel Initialization Begins-----\n");

  sched_init();
  // initialize PCBs, added by xw, 18/5/26
  error = initialize_processes();
  if (error == 0) return error;

  // initialize CPUs, added by xw, 18/6/2
  error = initialize_cpus();
  if (error != 0) return error;

  k_reenter = 0;  // record nest level of only interruption! it's different from
                  // Orange's.

  /************************************************************************
  *device initialization
  added by xw, 18/6/4
  *************************************************************************/
  init_kb();  // added by mingxuan 2019-5-19

  AHCI_init();

  /* initialize hd-irq and hd rdwt queue */
  init_hd();

  /* initialize message queue */
  init_msgq();  // added by yingchi 2021.12.24

  /* init shm*/
  init_shm();
  init_devices();
  register_fs_types();
  /* initialize 8253 PIT */
  init_clock();  // read rtc init, 放在硬盘、键盘后面初始化减少误差
  /* enable interrupt, we should read information of some devices by interrupt.
   * Note that you must have initialized all devices ready before you enable
   * interrupt. added by xw
   */
  enable_int();
  init_open_hd();

  init_buffer(64);
  init_fs(SATA_BASE);

  disable_int();

  disp_str("-----Processes Begin-----\n");

  /* linear address 0~8M will no longer be mapped to physical address 0~8M.
   * note that disp_xx can't work after this function is invoked until processes
   * runs. add by visual 2016.5.13; moved by xw, 18/5/30 清掉低端页表后
   * disp_xx不运行这个问题已经修复  modified by sundong 2023.3.8
   */
  // clear_kernel_pagepte_low(); //delete by sundong 2023.3.8
  // 因为kernel重新初始化页表的时候就删掉了低端页表

  // p_proc_current = proc_table;
  p_proc_current = &proc_table[PID_INIT];
  p_proc_current->task.cwd = vfs_root;
  cr3_ready = p_proc_current->task.cr3;

  disp_str("main:total_mem_size=");
  disp_int(kern_total_mem_size());
  disp_str("\n");
  init_ttys();
  initlock(&video_mem_lock, "vmem");
  kernel_initial = 0;

#ifdef BLAME_STAT
  stat_init();  // stat performance
#endif
  restart_initial();  // modified by xw, 18/4/19
  while (1) {
  };
}

/*************************************************************************
return 0 if there is no error, or return -1.
added by xw, 18/6/2
***************************************************************************/
PRIVATE int initialize_cpus() {
  // just use the fields of struct PCB in cpu_table, we needn't initialize
  // something at present.

  return 0;
}

/*
 * @brief 初始化所有起始进程
 * @return 失败返回-1，成功返回initial进程的pid
 * @details
 * 会初始化所有PCB，同时根据task_table创建task进程，task进程即服务进程；
 * 			创建initial进程，将p_proc_current和cr3_ready指向initial进程
 * 			创建idle进程
 */
PRIVATE int initialize_processes() {
  // init_all_PCB();
  for (int i = 0; i < NR_TASKS; i++) {
    int pid = kthread_create(task_table[i].name,
                             (void*)task_table[i].initial_eip, task_table[i].rt,
                             task_table[i].rpl, task_table[i].priority_nice);
    rq_insert(&proc_table[pid]);
  }

  int initial_pid =
      kthread_create("initial", initial, false, PRIVILEGE_TASK, 0);
  rq_insert(&proc_table[initial_pid]);

  /*************************进程树信息初始化***************************************/
  PROCESS* p_proc = &proc_table[PID_INIT];
  p_proc->task.tree_info.type = TYPE_PROCESS;  //当前是进程还是线程
  p_proc->task.tree_info.real_ppid = -1;  //亲父进程，创建它的那个进程
  p_proc->task.tree_info.ppid = -1;       //当前父进程
  p_proc->task.tree_info.child_p_num = 0;  //子进程数量
  // p_proc->task.tree_info.child_process[NR_CHILD_MAX];//子进程列表
  p_proc->task.tree_info.child_t_num = 0;  //子线程数量

  p_proc->task.memmap.vpage_lin_base = VpageLinBase;   //保留内存基址
  p_proc->task.memmap.vpage_lin_limit = VpageLinBase;  //保留内存界限

  return initial_pid;
}
