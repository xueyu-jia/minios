#ifndef LIST_H
#define LIST_H
#include "const.h"

/// @brief  仿照linux实现的通用双向循环链表
/// @author jiangfeng 2024.3
/// @details 可以理解为单独将常规链表的指针域抽象为单独的节点结构体
/// 	为了能够访问数据，需要在使用时传入节点所在的数据结构类型或者对应类型的指针
///		以及节点在所在的数据结构的成员名。
///		大致的数据结构示意：其中head, type, entry, member
///   head       type___________  <--entry  
///    |             |          |
///    |      (first)|          |
///    v   next      |          |
///  list_head <---->|  member  | <----> ... node 
///(struct list_node)|__________|
///    ^
///prev|      (last)
///    +---->     ...
///
struct list_node{
	struct list_node *prev;
	struct list_node *next;
};
typedef struct list_node list_head;

#define list_entry(node, type, member) (container_of(node, type, member))

// 将节点初始化为空
#define list_init(node)	\
	(node)->prev = (node)->next = (node)

// 节点是否为空
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

#define list_add_after(node, list) _list_insert(node, (list), (list)->next)
#define list_add_first(node, list) _list_insert(node, (list), (list)->next)
#define list_add_before(node, list) _list_insert(node, (list)->prev, (list))
#define list_add_last(node, list) _list_insert(node, (list)->prev, (list))

// 将node从当前所在的链表移出
#define list_remove(node) _list_remove(node, (node)->prev, (node)->next)

// 将node从当前所在的链表移出,并插入到list的开头
#define list_move(node, list) do{\
	list_remove(node);\
	list_add_first(node, list);\
}while(0)

/// @brief 返回entry在双向链表中的后一项，类型与entry相同，如果entry是最后一项(entry对应的node的后一项是head),返回NULL
/// @param head struct list_node* 链表头指针
/// @param entry Type* 数据项指针
/// @param member 链表节点在数据项Type中的成员名
#define list_next(head, entry, member) 	\
(((entry)->member.next != (head)) ? list_entry((entry)->member.next, typeof(*entry), member): NULL)

/// @brief 返回entry在双向链表中的前一项，类型与entry相同，如果entry是第一项(entry对应的node的前一项是head),返回NULL
/// @param head struct list_node* 链表头指针
/// @param entry Type* 数据项指针
/// @param member 链表节点在数据项Type中的成员名
#define list_prev(head, entry, member) 	\
(((entry)->member.prev != (head)) ? list_entry((entry)->member.prev, typeof(*entry), member): NULL)

/// @brief 返回entry在双向链表中的第一项，类型为type*，如果链表为空,返回NULL
/// @param head struct list_node* 链表头指针
/// @param type 数据项类型
/// @param member 链表节点在数据项Type中的成员名
#define list_front(head, type, member) ((list_empty((head)))?NULL:list_entry((head)->next, type, member))

/// @brief 返回entry在双向链表中的最后一项，类型为type*，如果链表为空,返回NULL
/// @param head struct list_node* 链表头指针
/// @param type 数据项类型
/// @param member 链表节点在数据项Type中的成员名
#define list_back(head, type, member) ((list_empty((head)))?NULL:list_entry((head)->prev, type, member))

/// @brief 此宏用来便于正向遍历双向链表
/// @param head struct list_node* 链表头指针
/// @param type 数据项类型
/// @param member 链表节点在数据项Type中的成员名
/// @details 注意：使用此宏必须保证不改变链表结构 
/// example:
/// struct Type {
/// 	...
///		struct list_node t_list;
/// };
/// list_head demo_list;
/// Type * elem = NULL;
/// list_for_each(&demo_list, elem, t_list)
/// {
///		// any action to do with elem
/// }
#define list_for_each(head, entry, member) \
for(entry = list_front(head, typeof(*entry), member);	\
	entry; entry = list_next(head, entry, member)\
)

/// @brief 此宏用来便于反向遍历双向链表
/// @param head struct list_node* 链表头指针
/// @param type 数据项类型
/// @param member 链表节点在数据项Type中的成员名
/// @details 注意：使用此宏必须保证不改变链表结构 
#define list_for_each_reverse(head, entry, member) \
for(entry = list_back(head, typeof(*entry), member);	\
	entry; entry = list_prev(head, entry, member)\
)

/// @brief 返回链表中第一个节点数据中key的值为value的有效数据指针，如果找不到，则返回NULL
/// @param head struct list_node* 链表头指针
/// @param type 数据项类型
/// @param member 链表节点在数据项类型type中的成员名
/// @param key  要查找的数据在type中的成员名
/// @param value 要匹配的值
#define list_find_key(head, type, member, key, value) ({	\
	type *_entry = NULL;	\
	list_for_each(head, _entry, member){	\
		if((_entry)->key == value)break;	\
	}	\
	_entry;	\
})

#endif