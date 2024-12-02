#include <malloc.h>
#include <opt.h>
#include <stdio.h>
#include <string.h>

// borrowed from glibc
/* For communication from 'getopt' to the caller.
   When 'getopt' finds an option that takes an argument,
   the argument value is returned here.
   Also, when 'ordering' is RETURN_IN_ORDER,
   each non-option ARGV-element is returned here.  */

// 上一次调用getopt解析得到参数的字符串
char *optarg;

/* Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to 'getopt'.

   On entry to 'getopt', zero means this is the first call; initialize.

   When 'getopt' returns -1, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, 'optind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */

/* 1003.2 says this must be 1 before any call.  */
int optind = 1;

/* Callers store zero here to inhibit the error message
   for unrecognized options.  */
int opterr = 1;

/* Set to an option character which was unrecognized. */
int optopt = '?';

// self simple implement for getopt
// optstring: containing the legitimate option characters, : for argument
// option, see `man 3 getopt`

/// @brief 解析选项参数
/// @param argc 参数个数，argv的长度
/// @param argv 参数列表
/// @param optstring 选项字符
/// @return
int getopt(int argc, char *argv[], const char *optstring) {
  char *p_arg, *p_opt;
  p_arg = argv[optind];
  int opt_checkind = optind;
  while (opt_checkind < argc && ((p_arg[0] != '-') || (p_arg[1] == '\0'))) {
    p_arg = argv[++opt_checkind];  // 找到第一个选项字符串"-x", x是选项字符
  }
  if (opt_checkind == argc) {  // 没有选项
    return -1;
  }
  // exchange non option backwards
  int non_opt = opt_checkind - optind;
  if (non_opt > 0) {
    char **tmp = malloc(sizeof(char *) * non_opt);
    for (int i = 0; i < non_opt; i++) {
      tmp[i] = argv[i + optind];
    }
    int ind = optind;
    for (int i = opt_checkind; i < argc; i++) {
      argv[ind++] = argv[i];
    }
    for (int i = 0; i < non_opt; i++) {
      argv[ind++] = tmp[i];
    }
    free((u32)tmp);
  }
  // exchange finished

  // if((p_arg[0] == '-') && (p_arg[1] != '\0')) {
  char optchar = *(++p_arg);
  p_opt = strchr(optstring, optchar);
  if (p_opt == NULL || optchar == ':' || optchar == ';') {
    fprintf(STD_ERR, "invalid option!\n");
    optopt = optchar;
    return '?';
  }
  if (*(++p_arg) == 0) {
    optind++;
  }
  if (p_opt[1] == ':') {
    if (p_opt[2] == ':') {
      if (*p_arg != NULL) {
        optarg = p_arg;
        optind++;
      } else {
        optarg = NULL;
      }
    } else {
      if (*p_arg != NULL) {
        optarg = p_arg;
        optind++;
      } else if (optind == argc) {
        fprintf(STD_ERR, "option require arg -%c\n", optchar);
        optopt = optchar;
        return '?';
      } else {
        optarg = argv[optind++];
      }
    }
  }
  return optchar;
  // }
  // return -1;
}
