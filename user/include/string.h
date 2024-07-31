#ifndef USR_STRING_H
#define USR_STRING_H
void* memcpy(void* p_dst, void*  p_src, int size);//void* memcpy(void* es:p_dst, void* ds:p_src, int size);
void memset(void* p_dst, char ch, int size);
int memcmp(const void* p1, const void*  p2, int size);
char* strcat(char *dst, const char *src);
char* strcpy(char* p_dst, const char* p_src);
char* strncpy(char * dest, const char *src, int n);
int strlen(const char* p_str);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
char *strchr(const char *str, char c);
char* strrchr(const char *s, char c);
#endif