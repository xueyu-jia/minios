/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            type.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//ported by sundong 2023.3.26

#ifndef	_ORANGES_TYPE_H_
#define	_ORANGES_TYPE_H_
#include <stdbool.h>

#ifndef NULL
#define NULL ((void*) 0)
#endif
typedef unsigned int		u32;
typedef int			i32;
typedef u32 			uintptr_t;
typedef u32			phyaddr_t;
typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned long long	u64;

// 通常描述一个对象的大小，会根据机器的型号变化类型
typedef u32			size_t;
// signed size_t 通常描述系统调用返回值，会根据机器的型号变化类型
typedef i32			ssize_t;



typedef unsigned char   BYTE;//字节
typedef unsigned short  WORD;//双字节
typedef unsigned long   DWORD;//四字节
typedef unsigned int    UINT;//无符号整型
typedef char		    CHAR;//字符类型

/* 函数类型 */
#define	PUBLIC		/* PUBLIC is the opposite of PRIVATE */
#define	PRIVATE	static	/* PRIVATE x limits the scope of x */

/*分页机制常量的定义,必须与load.inc中一致*/				//add by visual 2016.4.5
#define	PG_P		1	// 页存在属性位
#define	PG_RWR		0	// R/W 属性位值, 读/执行
#define	PG_RWW		2	// R/W 属性位值, 读/写/执行
#define	PG_USS		0	// U/S 属性位值, 系统级
#define	PG_USU		4	// U/S 属性位值, 用户级
#define PG_PS		64	// PS属性位值，4K页

#define TRUE 1//是
#define FALSE 0//否

#endif /* _ORANGES_TYPE_H_ */
