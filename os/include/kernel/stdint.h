/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-14 13:09:35
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-18 14:31:59
 * @FilePath: /minios/os/include/kernel/stdint.h
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
// #pragma once
#ifndef MY_CUSTOM_TYPE_H
#define MY_CUSTOM_TYPE_H

typedef signed char      int8_t;
typedef short     int16_t;
typedef int       int32_t;
typedef long long int64_t;

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

#define i8  int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

typedef uint32_t size_t;

#endif // MY_CUSTOM_TYPE_H
