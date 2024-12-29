#pragma once

#include <stdbool.h>

//! whether the kernel is being initialized
extern bool kstate_on_init;

//! nest level of interruption
extern int kstate_reenter_cntr;
