/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-14 13:24:41
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-19 10:49:52
 * @FilePath: /minios/os/include/klib/assert.h
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#ifndef ASSERT_H
#define ASSERT_H

#include <kernel/console.h>
#include <klib/string.h>

#define _STR(x) _VAL(x)
#define _VAL(x) #x

#define assert(expr)                                    \
  if (!(expr)) do {                                     \
      disp_color_str("assert failed:" #expr " ", 0x74); \
      disp_color_str(__builtin_FILE(), 0x74);           \
      disp_color_str(":"_STR(__LINE__), 0x74);          \
      while (1)                                         \
        ;                                               \
    } while (0);

//! TODO: impl gsl
#define unreachable(...)     \
  do {                       \
    while (1)                \
      ;                      \
    __builtin_unreachable(); \
  } while (0)
#define todo(...) unreachable()
#define unimplemented()

#endif
