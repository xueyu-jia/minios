#ifndef PTHREAD_H
#define PTHREAD_H

#include "type.h"

/* detachstate-线程分离状态 */
#define PTHREAD_CREATE_DETACHED 0
#define PTHREAD_CREATE_JOINABLE 1

/* schedpolicy-线程调度策略 */
#define SCHED_FIFO      0
#define SCHED_RR        1
#define SCHED_OTHER     2

/* schedparam-线程调度参数 */

/* 
schedparam需要涉及到对timespec定义，
但是个人能力有限，没能读完全部内容
所以就仅仅建立一个结构体代表schedparam
*/

typedef struct p_sched_param {
        /* DATA */
        int sched_priority; //added by dongzhangqi 2023.5.8
}SCHED_PARAM;

/* inheritsched-线程的继承性 */
#define PTHREAD_INHERIT_SCHED   0
#define PTHREAD_EXPLICIT_SCHED  1

/* scope-线程作用域 */
#define PTHREAD_SCOPE_PROCESS   0
#define PTHREAD_SCOPE_SYSTEM    1

typedef struct p_pthread_attr_t{
        int             detachstate;
        int             schedpolicy;
        SCHED_PARAM     schedparam;
        int             inheritsched;
        int             scope;
        u32             guardsize;
        int             stackaddr_set;
        void*           stackaddr;
        u32             stacksize;
}pthread_attr_t;

#endif