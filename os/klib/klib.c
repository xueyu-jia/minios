
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            klib.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

//modified by mingxuan 2021-4-3
#include "type.h"
#include "const.h"
#include "proto.h"


/*======================================================================*
                               itoa
 *======================================================================*/
PUBLIC char *itoa(int num, char *str, int radix)
{
	char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // 索引表
	unsigned unum;										   // 存放要转换的整数的绝对值,转换的整数可能是负数
	int i = 0, j, k = 0;									   // i用来指示设置字符串相应位，转换之后i其实就是字符串的长度；转换后顺序是逆序的，有正负的情况，k用来指示调整顺序的开始位置;j用来指示调整顺序时的交换。

	// 获取要转换的整数的绝对值
	if (radix == 10 && num < 0) // 要转换成十进制数并且是负数
	{
		unum = (unsigned)-num; // 将num的绝对值赋给unum
		str[i++] = '-';		   // 在字符串最前面设置为'-'号，并且索引加1
		k = 1;
	}
	else
		unum = (unsigned)num; // 若是num为正，直接赋值给unum

	// 转换部分，注意转换后是逆序的
	do
	{
		str[i++] = index[unum % (unsigned)radix]; // 取unum的最后一位，并设置为str对应位，指示索引加1
		unum /= radix;							  // unum去掉最后一位

	} while (unum); // 直至unum为0退出循环

	str[i] = '\0'; // 在字符串最后添加'\0'字符，c语言字符串以'\0'结束。

	// 将顺序调整过来
	// if (str[0] == '-')
	// 	k = 1; // 如果是负数，符号不用调整，从符号后面开始调整
	// else
	// 	k = 0; // 不是负数，全部都要调整

	char temp;						   // 临时变量，交换两个值时用到
	for (j = k; j <= (i - 1) / 2; j++) // 头尾一一对称交换，i其实就是字符串的长度，索引最大值比长度少1
	{
		temp = str[j];				 // 头部赋值给临时变量
		str[j] = str[i - 1 + k - j]; // 尾部赋值给头部
		str[i - 1 + k - j] = temp;	 // 将临时变量的值(其实就是之前的头部值)赋给尾部
	}

	return str; // 返回转换后的字符串
}
// PUBLIC char * itoa(char * str, int num)/* 数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800 */
// {
// 	char *	p = str;
// 	char	ch;
// 	int	i;
// 	int	flag = FALSE;

// 	*p++ = '0';
// 	*p++ = 'x';

// 	if(num == 0){
// 		*p++ = '0';
// 	}
// 	else{	
// 		for(i=28;i>=0;i-=4){
// 			ch = (num >> i) & 0xF;
// 			if(flag || (ch > 0)){
// 				flag = TRUE;
// 				ch += '0';
// 				if(ch > '9'){
// 					ch += 7;
// 				}
// 				*p++ = ch;
// 			}
// 		}
// 	}

// 	*p = 0;

// 	return str;
// }


/*======================================================================*
                               disp_int
 *======================================================================*/
PUBLIC void disp_int(int input)
{
	char output[16];
	itoa(input, output, 16);
	disp_str(output);
}

/*======================================================================*
                               delay
 *======================================================================*/
PUBLIC void delay(int time)
{
	int i, j, k;
	for(k=0;k<time;k++){
		/*for(i=0;i<10000;i++){	for Virtual PC	*/
		for(i=0;i<10;i++){/*	for Bochs	*/
			for(j=0;j<10000;j++){}
		}
	}
}

/**
 * @brief 获取当前进程的特权级
 * added by zhenhao 2023.5.19
 * @return void
 */
PUBLIC u32 get_ring_level() {
	int ringLevel;
	asm volatile ("mov %%cs, %0 \n\t and $0x3, %0" : "=a" (ringLevel));
	return ringLevel;
}