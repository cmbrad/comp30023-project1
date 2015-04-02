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

list_t *list_new(size_t data_size);
void list_destroy(list_t *list);

void list_push(list_t *list, void *data);
void *list_pop(list_t *list);

int list_is_empty(list_t *list);
unsigned int list_count(list_t *list);
#endif
