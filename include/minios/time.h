#pragma once

#include <uapi/minios/time.h>
#include <fs/fs.h>
#include <klib/stdint.h>

extern u32 current_timestamp;

u32 mktime(struct tm* time);
struct tm* gmtime(u32 timestamp, struct tm* tm_time);
struct tm* localtime(u32 timestamp, struct tm* tm_time);
u32 get_init_rtc_timestamp();

int kern_get_time(struct tm* time);

extern struct file_operations rtc_file_ops;
