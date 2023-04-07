#ifndef MINIOS_STRING_H
#define MINIOS_STRING_H
//ported by sundong 2023.3.26

#include "type.h"

int	strlen(const char *s);
//int	strnlen(const char *s, size_t size);
//char *	strcpy(char *dst, const char *src);
//char *	strncpy(char *dst, const char *src, size_t size);
//char *	strcat(char *dst, const char *src);
int	strncmp(const char *s1, const char *s2, size_t size);
int strcmp(const char *p, const char *q);
void *	memset(void *v, int c, size_t n);
void *	memcpy(void *dst, const void *src, size_t n);

#endif /* MINIOS_STRING_H */