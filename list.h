#include <stdlib.h>

#ifndef INCLUDE_LIST_H
#define INCLUDE_LIST_H

typedef struct node {
	void *data;
	struct node *next;
} node_t;

typedef struct list {
	node_t *head;
	node_t *foot;
	unsigned int node_count;
} list_t;

typedef int (*match_func)(void *, void*);
typedef void *(*select_func)(void *, void*);
typedef void (*reduce_func)(void *, void *);
typedef int (*cmp_func)(void *, void *);
typedef int (*modify_func)(list_t *, void *, void *);
typedef void (*iter_func)(void *);

list_t *list_new();
void list_destroy(list_t *);

void list_push(list_t *, void *);
void *list_pop(list_t *);
void list_remove(list_t *, void *);
void *list_get_next(list_t *, void *);

void list_for_each(list_t *, iter_func);
int list_modify(list_t *, void *, modify_func);

void *list_select(list_t *, void *, match_func, select_func);
void *list_select_from(list_t *, void *, void *, match_func, select_func);

void list_reduce(list_t *list, void *accum, reduce_func);
void list_insert(list_t *list, void *data, cmp_func cmp);

int list_is_empty(list_t *list);
#endif
