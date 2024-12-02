/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-15 09:42:23
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-15 09:49:06
 * @FilePath: /minios/os/kernel/scheduler.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include <kernel/buffer.h>
#include <kernel/clock.h>
#include <kernel/console.h>
#include <kernel/const.h>
#include <kernel/hd.h>
#include <kernel/proc.h>
#include <kernel/proto.h>
#include <kernel/type.h>
#include <klib/string.h>

typedef struct sched_process_entity {
  u32 pid;
  PROCESS* p_process;
  struct sched_process_entity* next;
  struct sched_process_entity* prev;
  // int pri;
} sched_entity;

int nice_to_weight[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */ 9548,  7620,  6100,  4904,  3906,
    /*  -5 */ 3121,  2501,  1991,  1586,  1277,
    /*   0 */ 1024,  820,   655,   526,   423,
    /*   5 */ 335,   272,   215,   172,   137,
    /*  10 */ 110,   87,    70,    56,    45,
    /*  15 */ 36,    29,    23,    18,    15};

sched_entity rt_rq_head_empty = {-1, NULL, NULL, NULL};
sched_entity* rt_rq = &rt_rq_head_empty;
sched_entity* rt_rq_tail = &rt_rq_head_empty;
sched_entity rt_rq_array[PROC_READY_MAX];

sched_entity rq_head_empty = {-1, NULL, NULL, NULL};
sched_entity* rq = &rq_head_empty;
sched_entity* rq_tail = &rq_head_empty;
sched_entity rq_array[PROC_READY_MAX];

int sysctl_sched_rt_period = 10;
int sysctl_sched_rt_runtime = 9;
int rt_runtime;
int rt_period;

PRIVATE u32 get_min_vruntime();
PRIVATE sched_entity* get_curr_entity(PROCESS* current_proc);
PRIVATE void rq_resort(sched_entity* changed_entity);

PRIVATE int test_cfs() {
  if (p_proc_current->task.stat == READY ||
      rt_rq->next->p_process->task.stat == READY)
    return true;
  else
    return false;
}

/*======================================================================*
                              cfs_schedule
 *======================================================================*/

void cfs_sched() {
  if (rt_runtime < sysctl_sched_rt_runtime &&
      rt_rq->next !=
          NULL)  // rt_runtime<950000 rt_rq is not empty(静态优先级+FIFO)
  {
    if (p_proc_current->task.is_rt == true) {
      if (p_proc_current->task.stat == READY &&
          p_proc_current->task.rt_priority >
              rt_rq->next->p_process->task.rt_priority) {
        p_proc_next = p_proc_current;
      } else {
        p_proc_current->task.sum_cpu_use += p_proc_current->task.cpu_use;
        p_proc_current->task.cpu_use = 0;

        p_proc_next = rt_rq->next->p_process;
      }
    } else {
      p_proc_current->task.vruntime +=
          p_proc_current->task.cpu_use * 1024 / p_proc_current->task.weight;
      p_proc_current->task.sum_cpu_use += p_proc_current->task.cpu_use;
      p_proc_current->task.cpu_use = 0;

      sched_entity* curr_entity = get_curr_entity(p_proc_current);

      if (curr_entity != NULL && curr_entity->next != NULL &&
          p_proc_current->task.vruntime >
              curr_entity->next->p_process->task.vruntime) {
        // resort
        rq_resort(curr_entity);
      }

      p_proc_next = rt_rq->next->p_process;
    }
  } else {
    // update vruntime/sum_cpu_use
    if (p_proc_current->task.is_rt == false) {
      p_proc_current->task.vruntime += p_proc_current->task.cpu_use *
                                       (double)1024 /
                                       p_proc_current->task.weight;
    }
    p_proc_current->task.sum_cpu_use += p_proc_current->task.cpu_use;
    p_proc_current->task.cpu_use = 0;

    if (rq->next == NULL) {
      // disp_str("-cfs-");	//mark debug
      int idle_pid = kern_get_pid_byname("task_idle");
      proc_table[idle_pid].task.stat = READY;  // IDLE
      in_rq(&proc_table[idle_pid]);
      p_proc_next = rq->next->p_process;
      return;
    }
    // disp_int(rq->next->p_process->task.pid);

    if (p_proc_current->task.stat != READY ||
        p_proc_current->task.is_rt == true) {
      p_proc_next = rq->next->p_process;
    } else {
      sched_entity* curr_entity = get_curr_entity(p_proc_current);
      // if(curr_entity == NULL){
      // 	disp_str("cfs_sched error1: cannot find the curr_entity");
      // 	while(1){};
      // }
      if (curr_entity != NULL && curr_entity->next != NULL &&
          p_proc_current->task.vruntime >
              curr_entity->next->p_process->task.vruntime) {
        // resort
        rq_resort(curr_entity);
      }
      p_proc_next = rq->next->p_process;

      if (rq_tail != rq &&
          rq_tail->p_process->task.vruntime  // must check rq not empty
              > 0xFFFFF)  // vruntime-min_vruntime,avoid overflow and time
                          // waste
      {
        sched_entity* pos = rq->next;
        double min_vruntime = get_min_vruntime();

        while (pos != NULL) {
          pos->p_process->task.vruntime -= min_vruntime;
          pos = pos->next;
        }
      }
    }
  }
}

void sched_init() {
  // added by zq
  for (int i = 0; i < PROC_READY_MAX; i++) {
    rt_rq_array[i].pid = PID_NO_PROC;
    rq_array[i].pid = PID_NO_PROC;
  }
}

void proc_update() {
  rt_period++;
  if (p_proc_current->task.is_rt == true) {
    rt_runtime++;
  }

  if (rt_period > sysctl_sched_rt_period)  // initialize
  {
    rt_period = rt_runtime = 0;
  }

  if (p_proc_current->task.is_rt == false) {
    p_proc_current->task.cpu_use++;
  }
  // p_proc_current->task.ticks--;
}

void schedule() {
  cfs_sched();
  cr3_ready = p_proc_next->task.cr3;
}

u32 get_min_vruntime() {
  sched_entity* pos = rq->next;
  u32 min_vruntime = 0xFFFFFF;

  while (pos != NULL) {
    if (min_vruntime > pos->p_process->task.vruntime) {
      min_vruntime = pos->p_process->task.vruntime;
    }
    pos = pos->next;
  }
  return min_vruntime;
}

// 返回指定PCB的调度实体sched_entity
sched_entity* get_curr_entity(PROCESS* current_proc) {
  sched_entity* pos = rq->next;

  while (pos != NULL) {
    if (pos->p_process == current_proc) {
      return pos;
    }
    pos = pos->next;
  }
  return NULL;
}

// 删除指定节点（但没释放该节点的空间，只是用pid=PID_NO_PROC标空了）
// 并将该节点的PCB重新插入链表
void rq_resort(sched_entity* changed_entity)  // only for rq
{
  changed_entity->pid = PID_NO_PROC;  // mark empty
  changed_entity->prev->next = changed_entity->next;
  if (changed_entity->next != NULL) {
    changed_entity->next->prev = changed_entity->prev;
  } else {
    rq_tail = changed_entity->prev;
  }

  in_rq(changed_entity->p_process);  // reinsert
}

PRIVATE sched_entity* find_new_sched_entity(sched_entity* array) {
  int i = 0;
  for (; i < PROC_READY_MAX && array[i].pid < NR_PCBS; i++);
  if (i < PROC_READY_MAX) {
    return &array[i];
  }
  return NULL;
}

// todo: rename
void in_rq(PROCESS* p_in) {
  int is_rt = p_in->task.is_rt;
  sched_entity* _rq_head = (is_rt) ? rt_rq : rq;
  sched_entity* _rq_tail = (is_rt) ? rt_rq_tail : rq_tail;
  sched_entity* _rq_arr = (is_rt) ? rt_rq_array : rq_array;
  sched_entity* new_entity = find_new_sched_entity(_rq_arr);
  if (new_entity == NULL) {
    disp_str("in_rq error1: cannot find a rq_array\n");
    return;  // mark 这里没做错误处理和提示，之后要加上
  }
  new_entity->pid = p_in->task.pid;
  new_entity->p_process = p_in;
  new_entity->next = NULL;
  new_entity->prev = NULL;

  // new_entity.pri=p_in->task.rt_priority;
  if (_rq_head == _rq_tail)  // rq is empty
  {
    _rq_head->next = new_entity;
    new_entity->prev = _rq_head;
    _rq_tail = new_entity;
  } else  // not empty,find propery position
  {
    sched_entity* pos = _rq_head;
    // avoid re_in_rq
    if (is_rt) {
      while (pos->next != NULL && pos->next->p_process->task.rt_priority >
                                      new_entity->p_process->task.rt_priority) {
        pos = pos->next;
      }
    } else {
      while (pos->next != NULL && pos->next->p_process->task.vruntime <=
                                      new_entity->p_process->task.vruntime) {
        pos = pos->next;
      }
    }
    if (pos->next == NULL) {
      pos->next = new_entity;
      new_entity->prev = pos;
      _rq_tail = new_entity;
    } else {
      new_entity->prev = pos;
      new_entity->next = pos->next;
      pos->next->prev = new_entity;
      pos->next = new_entity;
    }
  }
  if (is_rt) {
    rt_rq_tail = _rq_tail;
  } else {
    rq_tail = _rq_tail;
  }
}

void out_rq(PROCESS* p_out) {
  sched_entity* pos;
  if (p_out->task.is_rt == true)  // real time process
  {
    pos = rt_rq->next;
  } else  // not real time process
  {
    pos = rq->next;
  }
  while (pos != NULL && pos->pid != p_out->task.pid) pos = pos->next;
  if (pos != NULL)  // found
  {
    pos->prev->next = pos->next;
    if (pos->next != NULL) {
      pos->next->prev = pos->prev;
    } else {
      if (p_out->task.is_rt == true)  // real time process
      {
        rt_rq_tail = pos->prev;
      } else  // not real time process
      {
        rq_tail = pos->prev;
      }
    }
    pos->next = NULL;
    pos->prev = NULL;
    pos->pid = PID_NO_PROC;  // mark empty
  }
}

/*======================================================================*
                           yield and sleep
 *======================================================================*/
// used for processes to give up the CPU
PUBLIC void kern_yield() {
  // p_proc_current->task.ticks = 0;	/* modified by xw, 18/4/27 */
  p_proc_current->task.vruntime += 5;
  sched();  // Modified by xw, 18/4/19
}

PUBLIC void sched_yield() {
  u32 ringlevel = get_ring_level();
  if (ringlevel == 0) {
    kern_yield();
  } else {
    yield();
  }
}

PUBLIC void do_yield() { kern_yield(); }

PUBLIC void sys_yield() { do_yield(); }

PUBLIC void kern_sleep(int n) {
  int ticks0;

  ticks0 = ticks;
  //
  while (ticks - ticks0 < n) {
    wait_event(&ticks);
  }
}

PUBLIC void do_sleep(int n) { return kern_sleep(n); }

PUBLIC void sys_sleep() { return do_sleep(get_arg(1)); }

/*invoked by clock-interrupt handler to wakeup
 *processes sleeping on ticks.
 */
PUBLIC void wakeup(void* channel) {
  PROCESS *p, *target_p;
  target_p = NULL;

  for (p = proc_table; p < proc_table + NR_PCBS; p++) {
    if (p->task.stat == SLEEPING && p->task.channel == channel) {
      p->task.stat = READY;
      target_p = p;
      break;
    }
  }
  if (target_p != NULL) {
    if (target_p->task.is_rt == false) {
      target_p->task.vruntime = get_min_vruntime();  // min_vruntime
    }
    target_p->task.channel = 0;
    in_rq(target_p);
  }
}

// added by zq
PUBLIC void kern_nice(int inc) {
  if (p_proc_current->task.is_rt == true) return;

  if (inc < -20)
    inc = -20;
  else if (inc > 19)
    inc = 19;

  if (inc + p_proc_current->task.nice < -20) {
    p_proc_current->task.nice = -20;
  } else if (inc + p_proc_current->task.nice > 19) {
    p_proc_current->task.nice = 19;
  } else {
    p_proc_current->task.nice += inc;
  }
  p_proc_current->task.weight = nice_to_weight[p_proc_current->task.nice + 20];
}

PUBLIC void do_nice(int inc) { return kern_nice(inc); }

PUBLIC void sys_nice() { do_nice(get_arg(1)); }

PUBLIC void kern_set_rt(int turn_rt) {
  if (turn_rt == true &&
      p_proc_current->task.is_rt == false)  // not rt change to rt process
  {
    out_rq(p_proc_current);
    p_proc_current->task.is_rt = true;
  } else if (turn_rt == false &&
             p_proc_current->task.is_rt == true)  // rt change to not rt process
  {
    out_rq(p_proc_current);
    p_proc_current->task.is_rt = false;
  } else {
    return;
  }
  in_rq(p_proc_current);
  sched();
}

PUBLIC void do_set_rt(int turn_rt) { kern_set_rt(turn_rt); }

PUBLIC void sys_set_rt() { do_set_rt(get_arg(1)); }

PUBLIC void kern_rt_prio(int prio) {
  if (prio < 0 || p_proc_current->task.is_rt == false)
    return;
  else {
    p_proc_current->task.rt_priority = prio;
  }
  sched();
}

PUBLIC void do_rt_prio(int prio) { kern_rt_prio(prio); }
PUBLIC void sys_rt_prio() { do_rt_prio(get_arg(1)); }
