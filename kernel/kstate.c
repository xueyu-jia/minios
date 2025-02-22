#include <minios/kstate.h>

bool kstate_on_init;
int kstate_reenter_cntr;
multiboot_boot_info_t* kstate_mbi;
