#pragma once

#include <compiler.h>

struct list_node {
    struct list_node* prev;
    struct list_node* next;
};
typedef struct list_node list_head;

#define list_entry(node, type, member) (container_of(node, type, member))
#define list_init(node) (node)->prev = (node)->next = (node)
#define list_empty(head) ((head)->next == head)

static inline void _list_insert(struct list_node* new, struct list_node* prev,
                                struct list_node* next) {
    prev->next = new;
    next->prev = new;
    new->prev = prev;
    new->next = next;
}

static inline void _list_remove(struct list_node* self, struct list_node* prev,
                                struct list_node* next) {
    prev->next = next;
    next->prev = prev;
    self->next = self->prev = self;
}

#define list_add_after(node, list) _list_insert(node, (list), (list)->next)
#define list_add_first(node, list) _list_insert(node, (list), (list)->next)
#define list_add_before(node, list) _list_insert(node, (list)->prev, (list))
#define list_add_last(node, list) _list_insert(node, (list)->prev, (list))
#define list_remove(node) _list_remove(node, (node)->prev, (node)->next)

#define list_move(node, list)       \
    do {                            \
        list_remove(node);          \
        list_add_first(node, list); \
    } while (0)

#define list_next(head, entry, member)                                                           \
    (((entry)->member.next != (head)) ? list_entry((entry)->member.next, typeof(*entry), member) \
                                      : NULL)

#define list_prev(head, entry, member)                                                           \
    (((entry)->member.prev != (head)) ? list_entry((entry)->member.prev, typeof(*entry), member) \
                                      : NULL)

#define list_front(head, type, member) \
    ((list_empty((head))) ? NULL : list_entry((head)->next, type, member))

#define list_back(head, type, member) \
    ((list_empty((head))) ? NULL : list_entry((head)->prev, type, member))

#define list_for_each(head, entry, member)                        \
    for (entry = list_front(head, typeof(*entry), member); entry; \
         entry = list_next(head, entry, member))

#define list_for_each_reverse(head, entry, member)               \
    for (entry = list_back(head, typeof(*entry), member); entry; \
         entry = list_prev(head, entry, member))

#define list_find_key(head, type, member, key, value) \
    ({                                                \
        type* _entry = NULL;                          \
        list_for_each(head, _entry, member) {         \
            if ((_entry)->key == value) break;        \
        }                                             \
        _entry;                                       \
    })
