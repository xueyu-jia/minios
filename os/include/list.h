#ifndef LIST_H
#define LIST_H
#include "const.h"
struct list_node{
	struct list_node *prev;
	struct list_node *next;
};
typedef struct list_node list_head;

#define list_entry(node, type, member) (container_of(node, type, member))

#define list_init(node)	\
	(node)->prev = (node)->next = (node)

#define list_empty(head) \
	((head)->next == head)

PRIVATE inline void _list_insert(struct list_node* new, struct list_node* prev, struct list_node* next) {
	prev->next = new;
	next->prev = new;
	new->prev = prev;
	new->next = next;
}

PRIVATE inline void _list_remove(struct list_node* self, struct list_node* prev, struct list_node* next) {
	prev->next = next;
	next->prev = prev;
	self->next = self->prev = self;
}

#define list_add_first(node, list) _list_insert(node, (list), (list)->next)

#define list_add_last(node, list) _list_insert(node, (list)->prev, (list))

#define list_move(node, list) do{\
	_list_remove(node, (node)->prev, (node)->next);\
	list_add_first(node, list);\
}while(0)

#define list_remove(node) _list_remove(node, (node)->prev, (node)->next)

#define list_for_each(head, entry, member) \
for(entry = list_entry((head)->next, typeof(*entry), member);	\
	&entry->member != (head);\
	entry = list_entry(entry->member.next, typeof(*entry), member)\
)

#define list_front(head, type, member) ((list_empty((head)))?NULL:list_entry((head)->next, type, member))
#endif