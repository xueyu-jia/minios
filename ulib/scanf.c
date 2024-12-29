
#include <stdio.h>
#include <string.h> // IWYU pragma: keep
#include <stdarg.h>

int scanf(char *str, ...) {
    va_list va;
    int i = 0, j = 0, ret = 0;
    char buff[100] = {};
    char *out_loc;
    gets(buff);
    va_start(va, str);
    i = 0;
    while (str && str[i]) {
        if (str[i] == '%') {
            i++;
            switch (str[i]) {
                case 'c': {
                    *(char *)va_arg(va, char *) = buff[j];
                    j++;
                    ret++;
                    break;
                }
                case 'd': {
                    *(int *)va_arg(va, int *) = strtol(&buff[j], &out_loc, 10);
                    j += out_loc - &buff[j];
                    ret++;
                    break;
                }
                case 'x': {
                    *(int *)va_arg(va, int *) = strtol(&buff[j], &out_loc, 16);
                    j += out_loc - &buff[j];
                    ret++;
                    break;
                }
            }
        } else {
            buff[j] = str[i];
            j++;
        }
        i++;
    }
    va_end(va);
    return ret;
}
