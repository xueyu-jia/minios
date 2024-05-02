
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            string.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include "const.h"
PUBLIC char *itoa(int num, char *str, int radix);
PUBLIC void *memcpy(void *p_dst, const void *p_src, int size);
PUBLIC void  memset(void *p_dst, char ch, int size);
PUBLIC char *strcpy(char *p_dst, const char *p_src);
PUBLIC int   strcmp(const char *s1, const char *s2);
// added by zcr
PUBLIC int strlen(const char *p_str);

// added by ran
char *strncpy(char *dest, const char *src, int n);
int   strncmp(const char *s1, const char *s2, int n);
int   stricmp(const char *s1, const char *s2); // 不区分大小写