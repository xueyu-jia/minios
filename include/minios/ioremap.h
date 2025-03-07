#pragma once

#include <klib/sys/types.h>

void *ioremap(phyaddr_t phy_addr, size_t size);
void iounmap(void *addr);
