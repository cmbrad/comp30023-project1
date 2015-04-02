#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "list.h"

list_t *list_new(size_t data_size)
{
	list_t *list = malloc(sizeof(*list));
	list->head = list->foot = NULL;
	list->data_size = data_size;
	list->node_count = 0;

	return list;
}

void list_destroy(list_t *list)
{
	
}

void list_push(list_t *list, void *data)
{
	node_t *node = malloc(sizeof(*node));
	node->data = data;
	node->next = NULL;

	if (list->head == NULL)
		list->head = list->foot = node;
	else
	{
		list->foot->next = node;
		list->foot = node;
	}
	list->node_count++;
}

void *list_pop(list_t *list)
{
	void *res;

	assert(!list_is_empty(list));

	node_t *next = list->head->next;
	res = list->head->data;
        //free(list->head);
        list->head = next;
	list->node_count--;
	
	return res;
}

int list_is_empty(list_t *list)
{
	return list->node_count == 0;
}

unsigned int list_count(list_t *list)
{
	return list->node_count;
}