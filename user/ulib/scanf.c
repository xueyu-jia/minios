
#include "const.h"
#include "stdio.h"
#include "type.h"

// #include "../include/type.h"
// #include "../include/stdio.h"
// #include "../include/const.h"

PUBLIC int scanf(char *str, ...) {
  va_list vl;
  int i = 0, j = 0, ret = 0;
  char buff[100] = {0}, tmp[20], c;
  char *out_loc;
  gets(buff);
  va_start(vl, str);
  i = 0;
  while (str && str[i]) {
    if (str[i] == '%') {
      i++;
      switch (str[i]) {
        case 'c': {
          *(char *)va_arg(vl, char *) = buff[j];
          j++;
          ret++;
          break;
        }
        case 'd': {
          *(int *)va_arg(vl, int *) = strtol(&buff[j], &out_loc, 10);
          j += out_loc - &buff[j];
          ret++;
          break;
        }
        case 'x': {
          *(int *)va_arg(vl, int *) = strtol(&buff[j], &out_loc, 16);
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
  va_end(vl);
  return ret;
}