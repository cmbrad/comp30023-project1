#include <stdlib.h>
#include <assert.h>
#include "process_list.h"


list_t *new_list()
{
	list_t *list;
	list = malloc(sizeof(*list));
	assert(list != NULL);
	list->head = list->foot = NULL;
	return list;
}

list_t *push(list_t *list, process_t *process)
{
	node_t *new_node = malloc(sizeof(node_t));
	new_node->process = process;
	new_node->next = NULL;

	if(list->foot == NULL)
		list->head = list->foot = new_node;
	else
	{
		list->foot->next = new_node;
		list->foot = new_node;
	}

	return list;
}

process_t *pop(list_t *list)
{
	process_t *res = NULL;

	if(list_is_empty(list))
		return NULL;

	node_t *next = list->head->next;
	res = list->head->process;
	free(list->head);
	list->head = next;

	return res;
}

int list_is_empty(list_t *list)
{
	assert(list != NULL);
	return list->head == NULL;
}
