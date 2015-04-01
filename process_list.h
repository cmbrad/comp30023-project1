#include "process.h"

#ifndef INCLUDE_PROCESS_LIST_H
#define INCLUDE_PROCESS_LIST_H
typedef struct node node_t;

struct node
{
	process_t *process;
	node_t *next;
};

typedef struct
{
	node_t *head;
	node_t *foot;
} list_t;

list_t *new_list();
list_t *push(list_t *list, process_t *process);
process_t *pop(list_t *list);
int list_is_empty(list_t *list);
#endif
