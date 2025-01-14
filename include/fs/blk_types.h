#pragma once

#include <klib/stddef.h>

#ifndef SECTOR_SIZE_SHIFT
#define SECTOR_SIZE_SHIFT 9
#endif

#define SECTOR_SIZE (1 << (SECTOR_SIZE_SHIFT))
#define SECTOR_BITS ((SECTOR_SIZE) * 8)

#ifndef BLOCK_SIZE_SHIFT
#define BLOCK_SIZE_SHIFT 12
#endif

#define BLOCK_SIZE (1 << (BLOCK_SIZE_SHIFT))
#define SECTOR_PER_BLOCK (ROUNDUP(BLOCK_SIZE, SECTOR_SIZE) / (SECTOR_SIZE))
