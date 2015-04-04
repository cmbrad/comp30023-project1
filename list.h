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
	size_t data_size;
	unsigned int node_count;
} list_t;

typedef int (*cmp_func)(void *, void *);
typedef int (*modify_func)(list_t *, void *, void *);

list_t *list_new(size_t data_size);
void list_destroy(list_t *list);

void list_push(list_t *list, void *data);
void list_push_o(list_t *, void *, cmp_func);
void *list_pop(list_t *list);
void *list_get_next(list_t *list, void *data);

void list_for_each(list_t *list, void (*list_with)(void *));
void list_remove(list_t *list, void *data);
int list_modify(list_t *list, void *data, modify_func modify);

int list_is_empty(list_t *list);
unsigned int list_count(list_t *list);
#endif
