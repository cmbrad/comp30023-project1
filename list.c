/* Author: Chris Bradley (635 847)
 * Contact: chris.bradley@cy.id.au */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "list.h"

// Private function! Here instead of header.
node_t *get_node_for(list_t *list, void *data);

/* Creates a new list of size zero and returns a pointer
 * to it.
 *
 * Returns a pointer to list_t, a new list. */
list_t *list_new()
{
	list_t *list = malloc(sizeof(*list));
	list->head = list->foot = NULL;
	list->node_count = 0;

	return list;
}

/* Destroys a list! The list is only responsible for it's own
 * memory. If there are any complex structs inside data then
 * they won't be freed...luckily I'm not using any of those :D 
 *
 * list: List we're destroying. */
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


/* Adds an item onto the list at the end. Useful if
 * FIFO behavior is required.
 *
 * list: List to append to
 * data: Data to append to the list. Can be anything! */
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

/* Inserts into the given list using a specific ordering
 * as specified in a given comparison function.
 *
 * list: List to insert into
 * data: Item to insert into list
 * cmp: Function to used to keep ordering in list */
void list_insert(list_t *list, void *data, cmp_func cmp)
{
	node_t *new, *cur, *pre;

	new = malloc(sizeof(*new));
	assert(new != NULL);
	new->data = data;
	new->next = NULL;

	// We've inserted an item! Acknowledge that.
	list->node_count++;
	
	// If the list is empty then just insert at the top
	if (list->head == NULL) {
		list->head = list->foot = new;
		return;
	}

	// If the list is not empty then search until the cmp function is -1,
	// in other words search until the item we have is larger than the
	// items already in the list.
	pre = NULL;
	cur = list->head;

	while (cur != NULL && cmp(data, cur->data) == 1) {
		pre = cur;
		cur = cur->next;
	}

	if (pre == NULL) {
		// Item is at start of list
		list->head = new;
		new->next = cur;
	} else if(cur == NULL) {
		// Item is at end of list
		pre->next = new;
		list->foot = new;
	} else {
		// Item is between two other items.
		pre->next = new;
		new->next = cur;
	}
}

/* Remove the first item from the head of the list and return it.
 * If the list is empty then return NULL.
 *
 * list: List to pop from */
void *list_pop(list_t *list)
{
	void *res;

	// Could cause a crash here, but let caller handle it.
	if (list_is_empty(list))
		return NULL;

	// Return the current item and set head to be
	// the next item
	node_t *next = list->head->next;
	res = list->head->data;
	
	// Free the node but don't free the data, we need the data!
	// We just fetched it! No seriously. What sort of genius
	// would even code that free in the first place...whoops
	//free(list->head->data);
        free(list->head);
        list->head = next;
	list->node_count--;
	
	return res;
}

/* Executes a function using the data stored at each element in the
 * list. Useful for print functions!
 * list: List to interate over
 * list_with: Function to iterate with. Executes for each data in list. */
void list_for_each(list_t *list, iter_func list_with)
{
	node_t *cur = list->head;
	assert(cur != NULL);
	do {
		list_with(cur->data);
	} while((cur = cur->next));
}

/* Seek to modify items in the the list by comparing them to a specified
 * item (data) using a specified function (modify). Can modify more than
 * one item, if modification of any item succeeds we tell the caller. 
 *
 * list: List to modify
 * data: Item to compare against
 * modify: Function that modifies given list items
 *
 * Return 1 if any item is modified, else 0. */
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

/* Iterates over a list and compares all values against an (optional) static
 * value using the specified match function, and also compares values against
 * the current best value using the specified select function.
 *
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

/* Similar to list_select however it starts iterating from any item that is
 * a match to the given start item. Iteration will then cover all items in the
 * list and wrap around again. All items will be covered, it's just the start
 * point is modified.
 *
 * list: Item to select from
 * start: Data value where iteration should start from
 * data: Static data to compare against. (Optional)
 * match: Function to use to compare to static data. (Optional)
 * sel: Function to use when comparing against best selected item so far.
 *
 * Returns a pointer to the selected item. */
void *list_select_from(list_t *list, void *start, void *data, match_func match, select_func sel)
{
	void *res = NULL;
	node_t *cur = NULL;
	int first_iter = 1;

	if (list_is_empty(list))
		return NULL;

	// If no start point was specified start from the beginning.
	if (start == NULL)
		cur = list->head;
	else
	{
		// Start from given start point
		cur = get_node_for(list,start);
		// Want to cover the whole list even if don't start at start.
		// Set foot->next to head to make a loop
		list->foot->next = list->head;
	}
	assert(cur != NULL);

	do {
		// If we reach the start point again then break
		if (start != NULL && !first_iter && cur->data == start)
			break;
		first_iter = 0;
		// Might not want to always compare to a static value,
		// This data and match may be NULL. If so ignore them.
		if (match == NULL || match(cur->data, data))
			res = sel(res, cur->data);
	} while ((cur = cur->next));

	// Reset foot->next to NULL to break infinite loop we made to
	// cover all items earlier.
	list->foot->next = NULL;

	return res;
}

/* Get the list node associated with a particular piece of data.
 * INTERNAL USE ONLY.
 * list: List to fetch node from
 * data: Data point to be present in list
 *
 * Returns a pointer to the node where the data is present at.
 * If not present then return NULL. */
node_t *get_node_for(list_t *list, void *data)
{
	node_t *cur = list->head;
	assert(cur != NULL);

	do {
		if (cur->data == data)
			return cur;
	} while ((cur = cur->next));

	return NULL;
}

/* Remove an item from the list that corresponds to the given data value
 *
 * list: List to remove from
 * data: item to remove. */
void list_remove(list_t *list, void *data)
{
	node_t *pre = NULL;
	node_t *cur = list->head;

	assert(cur != NULL);
	do {
		if (cur->data == data)
		{
			// Keep the count accurate.
			list->node_count--;

			// If item is not at head then update previous item,
			// else update head of list.
			if (pre != NULL)
				pre->next = cur->next;
			else
				list->head = cur->next;

			// If item is at the end of the list then update the foot
			if (cur->next == NULL)
			{
				list->foot = pre;
				// This should usually run. list->foot will
				// only be NULL if list is empty.
				if (list->foot != NULL)
					list->foot->next = NULL;
			}

			return;
		}
		pre = cur;
	} while((cur = cur->next));	
}

/* Function to 'reduce' a list down to one value. Inspired by using
 * reduce when learning ruby! Total is directly written into accum
 * rather than returned.
 *
 * list: List to reduce
 * accum: Variable to accumulate list items into
 * reduce: Function to use while accumulating. */
void list_reduce(list_t *list, void *accum, reduce_func reduce)
{
	node_t *cur = list->head;
	assert(cur != NULL);
	do {
		reduce(accum, cur->data);
	} while ((cur = cur->next));
}

/* Get the item in the list that occurs after a given item.
 * Note: Returns data and not node_t itself, gotta keep that
 * encapsulation.
 *
 * list: List to iterate over
 * data: Item we're searching for...to get the next item
 *
 * Returns a pointer to the next item, or NULL if there is no next. */
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

/* Check if a given list is empty (no nodes) or not.
 *
 * list: List to check
 *
 * Returns 1 if list is empty or 0 if it contains at least 1 item. */
int list_is_empty(list_t *list)
{
	return list->node_count == 0;
}
