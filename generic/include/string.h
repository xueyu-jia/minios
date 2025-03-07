#pragma once

#include <stddef.h>

char *itoa(int num, char *str, int radix);

void *memcpy(void *p_dst, const void *p_src, size_t size);
void memset(void *p_dst, char ch, size_t size);
int memcmp(const void *p1, const void *p2, int size);
void *memmove(void *dest, const void *src, size_t count);
char *strcpy(char *p_dst, const char *p_src);
char *strncpy(char *dest, const char *src, int n);
char *strcat(char *dst, const char *src);
int strlen(const char *p_str);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
int stricmp(const char *s1, const char *s2);

char *strchr(const char *str, char c);
char *strrchr(const char *s, char c);
char *strstr(const char *s1, const char *s2);
