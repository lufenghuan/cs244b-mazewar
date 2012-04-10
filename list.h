/*
 * list.h - A generic linked list implementation taken from the Linux Kernel
 */

#ifndef _LIST_H
#define _LIST_H

#include <stdlib.h>
#include <stddef.h>

struct list_head {
	struct list_head *next, *prev;
};

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *_new,
                              struct list_head *prev,
                              struct list_head *next)
{
	next->prev = _new;
	_new->next  = next;
	_new->prev  = prev;
	prev->next = _new;
}

static inline void list_add_head(struct list_head *_new, struct list_head *head)
{
	__list_add(_new, head, head->next);
}

static inline void list_add_tail(struct list_head *_new, struct list_head *head)
{
	__list_add(_new, head->prev, head);\
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

#endif /* _LIST_H */

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
