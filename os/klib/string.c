#include <klib/string.h>

void* memcpy(void* p_dst, const void* p_src, size_t size){
	char* d = p_dst;
	const char *s = p_src;
	while(size > 0) {
		*(d++) = *(s++);
		size--;
	}
	return p_dst;
}

void memset(void* p_dst, char ch, size_t size){
	char* p = p_dst;
	while(size > 0) {
		*(p++) = ch;
		size--;
	}
}

char* strcat(char *dst, const char *src)
{
    if(dst==0 || src==0) return 0;
    char *temp = dst;
    while (*temp != '\0')
        temp++;
    while ((*temp++ = *src++) != '\0');

    return dst;
}

char* strcpy(char* p_dst, const char* p_src){
	char c = 0, *p = p_dst;
	do{
		c = *(p_src++);
		*(p++) = c;
	}while(c);
	return p_dst;
}

int strlen(const char* p_str){
	int cnt = 0;
	while(*(p_str++)){
		cnt++;
	}
	return cnt;
}

//added by ran
char* strncpy(char * dest, const char *src, int n)
{
	if ((dest == 0) || (src == 0)) { /* for robustness */
		return dest;
	}
    int i;
    for (i = 0; i < n; i++)
    {
        dest[i] = src[i];
        if (!src[i])
        {
            break;
        }
    }
    return dest;
}
/*****************************************************************************
 *                                strcmp
 *****************************************************************************/
/**
 * Compare two strings.
 *
 * @param s1  The 1st string.
 * @param s2  The 2nd string.
 *
 * @return  an integer less than, equal to, or greater than zero if s1 (or the
 *          first n bytes thereof) is  found,  respectively,  to  be less than,
 *          to match, or be greater than s2.
 *****************************************************************************/
int strcmp(const char *s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0))
	{ /* for robustness */
		return (s1 - s2);
	}

	const char *p1 = s1;
	const char *p2 = s2;

	for (; *p1 && *p2; p1++, p2++)
	{
		if (*p1 != *p2)
		{
			break;
		}
	}

	return (*p1 - *p2);
}

//added by ran
int strncmp(const char *s1, const char *s2, int n)
{
    if (!s1 || !s2)
    {
        return s1 - s2;
    }
    int i;
    for (i = 0; i < n; i++)
    {
        if (s1[i] != s2[i])
        {
            return s1[i] - s2[i];
        }
    }
    return 0;
}

char tolower(char c){
	if(c >= 'A' && c <= 'Z'){
		c = c + 'a' - 'A';
	}
	return c;
}

int stricmp(const char *s1, const char *s2)
{
	unsigned char c1, c2;

	c1 = 0;	c2 = 0;
	if (!s1 || !s2)
    {
        return s1 - s2;
    }
	for(; (c1 = *s1, c2 = *s2, c1 && c2); s1++, s2++){
		if (c1 == c2)
			continue;
		c1 = tolower(c1);
		c2 = tolower(c2);
		if (c1 != c2)
			break;
	}
	return (int)c1 - (int)c2;
}

// char *strchr(const char *s, char c) {
// 	if (!s)
//     {
//         return 0;
//     }
// 	const char *p;
// 	for(p = s; *p; p++) {
// 		if(*p == c) {
// 			break;
// 		}
// 	}
// 	return (*p == c)? p: 0;
// }
char *strchr(const char *s, char c) {
    char ch = (char)c;
    while (*s != '\0') {
        if (*s == ch) {
            return (char *)s;
        }
        s++;
    }
    return NULL;
}

// char* strrchr(const char *s, char c)
// {
//     if (!s)
//     {
//         return 0;
//     }
// 	char *r = 0;
// 	while (*s)
// 	{
// 		if (*s == c)
// 		{
// 			r = s;
// 		}
// 		++s;
// 	}
// 	return r;
// }
char *strrchr(const char *s, char c) {
    char ch = (char)c;
    char *last_occurrence = NULL;

    for (const char *p = s + strlen(s) - 1; p >= s; p--) {
        if (*p == ch) {
            last_occurrence = (char *)p;
        }
    }

    return last_occurrence;
}

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
