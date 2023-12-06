#include "../include/stdio.h"
#include "../include/ushm.h"

#define key 123456
void main(int arg, char *argv[])
{

	//udisp_int(shmid);
	int q = fork();

	if (q == 0)
	{
		int shmidA = -1;
		udisp_str("\nchirldA: ");
		shmidA = shmget(key, 80, SHM_IPC_CREAT);
		int *p = NULL;

		p = shmat(shmidA, NULL, 0);

		int i = 0;
		int q[15] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7};
		memcpy(p, q, sizeof(q));
		for (i = 0; i < 14; i++)
		{
			udisp_int(*(p + i));
		}

		shmdt(p);
	}
	else
	{
		int t;

		t = fork();
		if (t == 0)
		{
			int shmidB = -1;
			sleep(500);
			udisp_str("\nchirldB: ");
			shmidB = shmget(key, 80, SHM_IPC_CREAT);
			int *pp = NULL;
			pp = shmat(shmidB, NULL, 0);

			/********int***********/
			/*
		int a = *pp;
		udisp_int(a);
	*/
			/**********buf*******/
			int i = 0;
			for (i = 0; i < 14; i++)
			{
				udisp_int(*(pp + i));
			}

			*pp = 5;
			//int reverse[15]={7,9,8,5,3,9,6,2,9,5,1,4,1,3};
			//memcpy(pp,reverse,sizeof(reverse));

			shmdt(pp);

			shmctl(shmidB, DELETE, NULL);
			/************TEST_INFO*******/
			/*
	     struct ipc_shm *buf;
            buf = shmctl(shmid, INFO, buf);
            int c = buf->in_use;
            
	    udisp_str("\nINFO_in_Pro: ");
	    udisp_str("ids.in_use: ");
            udisp_int(c);
            //udisp_int(buf->seq);
*/
		}
	}

	while (1)
	{
	}
	return;
}

// void main(int arg, char *argv[])
// {

// // 	//udisp_int(shmid);
// // 	int q = fork();

// // 	if (q == 0)
// // 	{
// // 		int shmidA = -1;
// // 		udisp_str("\nchirldA: ");
// // 		shmidA = shmget(key, 4, SHM_IPC_CREAT);
// // 		int *p = NULL;

// // 		p = shmat(shmidA, NULL, 0);

// // 		/********int***********/
// // 		/*
// //         *p = 1024;
// //         udisp_int(*p);
// // */

// // 		/***********buf***********/

// // 		int i = 0;
// // 		int q[15] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7};
// // 		memcpy(p, q, sizeof(q));
// // 		for (i = 0; i < 14; i++)
// // 		{
// // 			udisp_int(*(p + i));
// // 		}

// // 		shmdt(p, 5);
// // 	}
// // 	else
// 	// {
// 		int t;

// 		// t = fork();
// 		// if (t == 0)
// 		// {
// 			int shmidB = -1;
// 			sleep(500);
// 			udisp_str("\nchirldB: ");
// 			shmidB = shmget(key, 4, SHM_IPC_CREAT);
// 			int *pp = NULL;
// 			pp = shmat(shmidB, NULL, 0);

// 			/********int***********/
// 			/*
// 		int a = *pp;
// 		udisp_int(a);
// 	*/
// 			/**********buf*******/
// 			int i = 0;
// 			for (i = 0; i < 14; i++)
// 			{
// 				udisp_int(*(pp + i));
// 			}

// 			*pp = 5;
// 			/*********打印内核信息************/

// 			shmdt(pp);
// 			struct ipc_shm *buf;
// 			buf = shmctl(shmidB, INFO, buf);
// 			int c = buf->in_use;

// 			udisp_str("\nINFO_in_Pro: ");
// 			udisp_str("ids.in_use: ");
// 			udisp_int(c);

// 			shmdt(pp);

// 			shmctl(shmidB, DELETE, NULL);
// 		// }
// 	// }

// 	// while (1)
// 	// {
// 	// }
// 	exit(0);
// 	return;
// }