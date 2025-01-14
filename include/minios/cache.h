#pragma once

#include <minios/spinlock.h>
#include <list.h>

struct inode;

typedef struct address_space {
    struct inode *host;
    int type;
    list_head page_list;
    spinlock_t lock;
} address_space_t;
