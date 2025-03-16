#include <minios/kstate.h>

bool kstate_on_init = true;

int kstate_reenter_cntr = 0;

multiboot_boot_info_t* kstate_mbi = NULL;

struct tm kstate_os_startup_time = {};

uint64_t kstate_spurious_irq_cntr = 0;
