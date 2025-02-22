#pragma once

#include <stdbool.h>
#include <multiboot.h>

//! whether the kernel is being initialized
extern bool kstate_on_init;

//! nest level of interruption
extern int kstate_reenter_cntr;

//! multiboot info
extern multiboot_boot_info_t* kstate_mbi;
