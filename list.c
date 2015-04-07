#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "list.h"
node_t *get_node_for(list_t *list, void *data);

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
	node_t *cur = list->head;

	while (list->head != NULL) {
		cur = list->head;
		list->head = cur->next;

		free(cur->data);
		free(cur);
	}

	free(list);
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

void list_push_o(list_t *list, void *data, cmp_func cmp)
{
	node_t *node = malloc(sizeof(*node));
	node->data = data;
	node->next = NULL;
	int res;

	node_t *pre = NULL;
	node_t *cur = list->head;

	// If the list is empty then just insert at the top.
	if (cur == NULL)
	{
		list_push(list, data);
		return;
	}

	do {
		assert(cur != NULL);
		res = (*cmp)(cur->data, data);
		if (res == -1)
		{
			if (pre != NULL)
				pre->next = node;
			else
				list->head = node;
			node->next = cur;

			break;
		}
		else if (res == 0 || res == 1)
		{
			node->next = cur->next;
			cur->next = node;

			if (node->next == NULL)
				list->foot = node;
			break;
		}
		pre = cur;
	} while ((cur = cur->next));
	list->node_count++;
}

void *list_pop(list_t *list)
{
	void *res;

	if (list_is_empty(list))
		return NULL;

	node_t *next = list->head->next;
	res = list->head->data;
        free(list->head);
        list->head = next;
	list->node_count--;
	
	return res;
}

void list_for_each(list_t *list, void (*list_with)(void *))
{
	node_t *cur = list->head;
	assert(cur != NULL);
	do {
		list_with(cur->data);
	} while((cur = cur->next));
}

int list_modify(list_t *list, void *data, modify_func modify)
{
	int success = 0;
	node_t *cur = list->head;
	assert(cur != NULL);
	do {
		if(modify(list, cur->data, data))
			success = 1;
	} while((cur = cur->next));
	return success;
}

//int list_for_each_at(list_t *list, void* start, void(*

/* Iterates over a list and compares all values against an (optional) static
 * value using the specified match function, and also compares values against
 * the current best value using the specified select function.
 * list: List to iterate over
 * data: Static data to compare against (Optional)
 * match: Function to use when comparing to data (Optional)
 * select: Function to use when comparing to best selected value so far
 *
 * Returns pointer to the data of value which best matches given criteria. */
void *list_select(list_t *list, void *data, match_func match, select_func sel)
{
	void *res = NULL;
	node_t *cur = list->head;
	assert(cur != NULL);

	do {
		// Might not want to always compare to a static value,
		// This data and match may be NULL. If so ignore them.
		if (match == NULL || match(cur->data, data))
			res = sel(res, cur->data);
	} while ((cur = cur->next));

	return res;
}

void *list_select_from(list_t *list, void *start, void *data, match_func match, select_func sel)
{
	void *res = NULL;
	node_t *cur = NULL;
	int first_iter = 1;

	// If a start point was specified 
	if (start == NULL)
		cur = list->head;
	else
	{
		cur = get_node_for(list,start);
		assert(cur != NULL);
		//printf("list->foot->next=%p\n",list->foot->next);
		//list->foot->next = list->head;
	}

	assert(cur != NULL);

	do {
		if (start != NULL && !first_iter && cur->data == start)
			break;
		first_iter = 0;

		// Might not want to always compare to a static value,
		// This data and match may be NULL. If so ignore them.
		if (match == NULL || match(cur->data, data))
			res = sel(res, cur->data);
	} while ((cur = cur->next));

	list->foot->next = NULL;

	return res;
}

node_t *get_node_for(list_t *list, void *data)
{
	node_t *cur = list->head;

	do {
		if (cur->data == data)
			return cur;
	} while ((cur = cur->next));

	return NULL;
}

void list_remove(list_t *list, void *data)
{
	node_t *pre = NULL;
	node_t *cur = list->head;

	//printf("list_remove\n");
	assert(cur != NULL);
	do {
		if (cur->data == data)
		{
			list->node_count--;
			if (pre != NULL)
				pre->next = cur->next;
			else
				list->head = cur->next;

			if (cur->next == NULL)
			{
				list->foot = pre;
				if (list->foot != NULL)
					list->foot->next = NULL;
			}

			//printf("head=%p, foot=%p\n",list->head, list->foot);
			return;
		}
		pre = cur;
	} while((cur = cur->next));	
}

void *list_get_next(list_t *list, void *data)
{
	node_t *cur = list->head;
	assert(cur != NULL);
	do {
		if(cur->data == data && cur->next)
			return cur->next->data;
	} while((cur = cur->next));
	return NULL;
}

int list_is_empty(list_t *list)
{
	return list->node_count == 0;
}

unsigned int list_count(list_t *list)
{
	return list->node_count;
}
