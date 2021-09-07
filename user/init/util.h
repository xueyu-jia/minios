#pragma once
//#include "stdio.h"
#include "../include/stdio.h"   //modified by mingxuan 2021-4-3

int listdir(const char* dirname);
int findfile(const char* filename);
int fat_open(const char *filename, int mode);
int fat_opendir(char * dirname);
int fat_createdir(char *dirname);
int fat_delete(char *filename);
int fat_deletedir(char *dirname);
int chdir(const char *path);
char *getcwd(char *buf, int size);

int fprintf(int fd, const char *fmt, ...);
int strcmp(const char *s1, const char *s2);
char* strrchr(char *s, int c);
extern char workdir[256];
extern char workdev[16];
