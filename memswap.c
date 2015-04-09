#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "list.h"
#include "process.h"
#include "process_size_file.h"
#include "memory.h"

#define USAGE "./memswap -a [algorithm name] -f [filename] -m [memsize]"

#define SWAP_LIMIT 3

process_t *swap_process(list_t *memory, list_t *free_list);
void add_free(list_t *free_list, memory_t *rem);
int process_cmp(void *cmp1, void *cmp2);
void print_free(list_t *free_list);
void print_mem(list_t *memory);
void print_que(list_t *queue);
int get_mem_usage(list_t *memory);
int remove_free(list_t *list, void *a, void *b);
int add_free_part(list_t *list, void *a, void *b);

void print_free_data(void *data);
void print_memory_data(void *data);
void print_process_data(void *data);

int get_arguments(int argc, char **argv, char **algorithm_name, char **filename, int *memsize);

int get_addr(list_t *, process_t *, select_func);
int match_addr(void *, void *);
int match_size(void *, void *);
void *first_get_addr(void *, void *);
void *best_get_addr(void *, void *);
void *worst_get_addr(void *, void *);
void *next_get_addr(void *, void *);

void *select_process(void *, void *);

int reduce_memory(void *a, void *b);

// Lets not go adding arguments to all our functions...
int last_address;

int main(int argc, char **argv)
{
	int memsize;
	char *filename, *algorithm_name;

	filename = algorithm_name = NULL;
	memsize = -1;

	if (!get_arguments(argc, argv, &algorithm_name, &filename, &memsize))
		return 1;

	// Last address we've looked at is the start.
	last_address = 0;

	// Parse the process file to obtain the initial queue of processes waiting to be swapped into memory.
	list_t *process_list = load_processes_from(filename);

	// Assume memory is initially empty.
	list_t *memory = list_new(sizeof(process_t));

	// Free list holds all the cards!
	list_t *free_list = list_new(sizeof(memory_t));

	// Initialise free memory to have a whole lot of nothing.
	memory_t *init_memory = malloc(sizeof(memory_t));
	init_memory->process = NULL;
	init_memory->addr = 0;
	init_memory->size = memsize;	
	list_push(free_list, init_memory);

	// Function pointer pointing to an address function
	// corresponding to the algorithm the user specified.
	select_func alg_get_addr  = NULL;
	if (strcmp(algorithm_name, "first") == 0)
		alg_get_addr = first_get_addr;
	else if (strcmp(algorithm_name, "best") == 0)
		alg_get_addr = best_get_addr;
	else if (strcmp(algorithm_name, "worst") == 0)
		alg_get_addr = worst_get_addr;
	else if (strcmp(algorithm_name, "next") == 0)
		alg_get_addr = next_get_addr;
	else
	{
		printf("ERROR: Invalid algorithm specified.\n");
		return 1;
	}

	int time = 0;
	// Load the processes from the queue into memory, one by one, according to one of the four algorithms.
	process_t *cur = NULL;
	while ((cur = list_pop(process_list)) != NULL)
	{
		// Allocate a new block of memory to add to the current
		// active memory.
		memory_t *new_mem = malloc(sizeof(memory_t));
		new_mem->process = cur;
		new_mem->process->last_loaded = time;
		new_mem->size = new_mem->process->size;
		
		// Swap out processes until we have enough space to allocate to
		// the current process
		while ((new_mem->addr = get_addr(free_list, new_mem->process, alg_get_addr)) == -1)
		{
			// Swap out a process to make room to allocate memory
			process_t *to_swap = swap_process(memory, free_list);
			// Processes will not be requeued for execution if they've
			// been swapped SWAP_LIMIT times.
			if (to_swap->swap_count < SWAP_LIMIT)
				list_push(process_list, to_swap);
		}
		list_push(memory, new_mem);
		//print_free(free_list);
		list_modify(free_list, new_mem, remove_free);
	
		// Print information string...
		printf("%d loaded, numprocesses=%d, numholes=%d, memusage=%d%%\n", new_mem->process->pid,memory->node_count,free_list->node_count,(int)(ceil(100*(((double)get_mem_usage(memory))/memsize))));
		
		//print_free(free_list);
		//print_mem(memory);
		//print_que(process_list);

		// Time advances at a constant rate! In this universe anyway..
		time += 1;
	}

	// Clean up
	list_destroy(memory);
	list_destroy(free_list);
	list_destroy(process_list);

	return 0;
}

/* Parses the arguments passed in via command line using getopt.
 *
 * argc: Number of arguments passed into the program
 * argv: Pointers to the arguments passed in
 * algorithm_name: Pointer to a char pointer to store algorithm name
 * filename: Pointer to a char pointer to store filename
 * memsize: Pointer to an integer to store size of virtual memory
 *
 * Returns 1 if fetching arguments succeeds, else returns 0. */
int get_arguments(int argc, char **argv, char **algorithm_name, char **filename, int *memsize)
{
	int c;

	while ((c=getopt(argc, argv, "a:f:m:")) != -1)
	{
		switch (c)
		{
			case 'a':
				*algorithm_name = optarg;
				break;
			case 'f':
				*filename = optarg;
				break;
			case 'm':
				*memsize = atoi(optarg);
				break;
			default:
				printf(USAGE);
				return 0;
		}
	}

	int all_given = 1;
	if (*filename == NULL) {
		printf("ERROR: No filename specified.\n");
		all_given = 0;
	} if (*algorithm_name == NULL) {
		printf("ERROR: No algorithm specified.\n");
		all_given = 0;
	} if (*memsize <= 0) {
		printf("ERROR: Memory size must be greater than 0.\n");
		all_given = 0;
	}

	if (!all_given) {
		return 0;
	}
	return 1;
}


/* Returns an address in memory which a new process should be inserted at. If
 * the addr function corresponding to the 'next fit' algorithm is specified
 * the global variable last_address is considered (otherwise it will be zero)
 *
 * free_list: List of free memory to try and insert into
 * process: Process to insert into memory
 * sel_get_addr: Pointer to a function which decides what address to choose.
 *               Different for diff algorithms ie first/worst/best/next
 * 
 * Returns an int as a location to insert to in memory, or -1 if no space.*/
int get_addr(list_t *free_list, process_t *process, select_func sel_get_addr)
{
	memory_t *chosen = NULL;
	memory_t *last = NULL;
	
	if (list_is_empty(free_list))
		return -1;

	last = list_select(free_list, &last_address, match_addr, first_get_addr);
	chosen = list_select_from(free_list, last, &process->size, match_size, sel_get_addr);
	
	return chosen != NULL ? chosen->addr : -1;
}

/* Match function which is passed into a list object which decides whether
 * the memory address (our virtual integer one)  of an item in the list
 * occurs after a specified address. Used with the free list.
 *
 *  a: Item from the list to test.
 *  b: Condition to satisfy - A valid int address in free memory list.
 *
 *  Returns 1 if the condition is met (b occurs after a), otherwise 0.*/
int match_addr(void *a, void *b)
{
	int cand = ((memory_t *)a)->addr;
	int want = *(int *)b;

	return cand >= want;
}

/* Match function which is passed into a list object which decides whether
 * the size of the virtual memory object (size being part of the memory_t
 * struct) is greater than than of a diff, static size. Used with free list.
 *
 * a: memory_t var to test against b.
 * b: Integer referring to a size of a memory location (Should be >=0)
 *
 * Returns 1 if the size of a is greater, or 0 otherwise.*/
int match_size(void *a, void *b)
{
	int cand = ((memory_t *)a)->size;
	int want = *(int *)b;

	assert(cand >= 0 && want >= 0);

	return cand >= want;
}

/* Select function which is passed into a list which tests a given item
 * from the list against the item which currently best satisifies selection
 * criteria for first fit algorithm. Criteria are if nothing is selected then
 * select the new item, else return the already selected item. Assume all
 * input is valid as has been validated to earlier call to a corresponding
 * match function.
 *
 * a: Current best item from the list (first valid)
 * b: memory_t item from the list to test against a.
 *
 * Returns a pointer to b if it's a better match, else return a. */
void *first_get_addr(void *a, void *b)
{
	memory_t *best = (memory_t *)a;
	memory_t *cand = (memory_t *)b;

	// Only want the first item. (Assume earlier call to a match function
	// means that all items are valid) Therefore if nothing is selected,
	// select the first item given! If something is selected then ignore
	// all new items.
	if(best == NULL)
		return cand;
	return best;
}

/* Select function to be passed into a list to test a against b with
 * criteria satisfying the best fit algorithm. Criteria are that if
 * no items have been selected, or if the size of the 'new' list item
 * b is equal to or smaller than that of the best item. Assume earlier
 * match call has validated input so it is valid.
 *
 * a: Current best item from the list (Smallest size)
 * b: memory_t item from the list to test against a
 *
 * Returns a pointer to b if it's a better match to criteria, else a.*/
void *best_get_addr(void *a, void *b)
{
	memory_t *best = (memory_t *)a;
	memory_t *cand = (memory_t *)b;

	// Fancier than first fit! Not only do we want the first item
	// we want the smallest item given as judged by the size propery.
	if(best == NULL || cand->size <= best->size)
		return cand;
	return best;
}

/* Select function to be a supplied to a list to test a against b. Here a
 * is the current 'best' item to fit search criteria, test b to see if it
 * is better than a. Better is defined as best being empty or being of
 * smaller size than the new candidate, b.
 *
 * a: Current best item from the list (Largest size)
 * b: Item from the list to test against a.
 *
 * Returns a pointer to b if it's a better match, else pointer to a.*/
void *worst_get_addr(void *a, void *b)
{
	memory_t *best = (memory_t *)a;
	memory_t *cand = (memory_t *)b;

	if(best == NULL || best->size <= cand->size)
		return cand;
	return best;
}

/* Select function to be a supplied to a list to test a against b. Here a
 * is the current 'best' item to fit search criteria, test b to see if it
 * is better than a. Better is defined as being the first valid item
 * supplied to the function. If the item is considered the best then
 * set the global variable last_address to be its address as the search
 * should resume here next time.
 *
 * a: Current best item from the list. (First valid from last_address)
 * b: Item from the list to test against a
 *
 * Returns a pointer to b if it's the first valid match, else return a.*/
void *next_get_addr(void *a, void *b)
{
	memory_t *best = (memory_t *)a;
	memory_t *cand = (memory_t *)b;

	if(best == NULL)
	{
		last_address = cand->addr;
		return cand;
	}
	return best;
}

/* Scans through the supplied memory list to find the largest
 * process, it is then swapped out of memory and placed onto
 * the free list to be allocated to a different process. If
 * two processes are of equal largest size then swap out the
 * one that has been in memory the longest.
 *
 * memory: List of memory_t => Processes which are currently in memory
 * free_list: List of memory_t representing unallocated memory.
 *
 * Returns a pointer to the process which was swapped out of memory. */
process_t *swap_process(list_t *memory, list_t *free_list)
{
	memory_t *to_swap = NULL;

	// list_select can take a value and a function by which
	// each value in the list is compared to the specified static
	// value to see if it should be selected. In this case
	// we have nothing to compare against so specify NULL
	to_swap = list_select(memory, NULL, NULL, select_process);
	assert(to_swap != NULL);
	list_remove(memory, to_swap);

	// Add the memory just swapped out to the list of free memory.
	add_free(free_list, to_swap);
	
	to_swap->process->swap_count++;
	return to_swap->process;
}

/* Select function which is passed into a list in order to find
 * an item in the list which best satisfies criteria in this func.
 * Criteria are that this is the first item passed in, that b
 * is bigger than a, or that b has been in memory longer.
 *
 * a: Current best item from the list
 * b: Item to test against a.
 *
 * Return a pointer to b if it is a better match, else a. */
void *select_process(void *a, void *b)
{
	memory_t *best = (memory_t *)a;
	memory_t *curr = (memory_t *)b;

	if (best == NULL || curr->size > best->size || (curr->size == best->size && curr->process->last_loaded < best->process->last_loaded))
		return curr;
	return best;
}

void add_free(list_t *free_list, memory_t *rem)
{
	// Try to combine newly freed memory with a current free block
	if (list_is_empty(free_list) || !list_modify(free_list, rem, add_free_part))
	{
		// Free memory is not contiguous and cannot be combined,
		// or there is no free memory. Create a new free block.
		memory_t *new_free = malloc(sizeof(memory_t));
		new_free->process = NULL;
		new_free->addr = rem->addr;
		new_free->size = rem->size;

		list_insert(free_list, new_free, process_cmp);
	}
}

int get_mem_usage(list_t *memory)
{
	int usage = 0;
	list_reduce(memory, &usage, reduce_memory);
	return usage;
}

int reduce_memory(void *a, void *b)
{
	int *total = (int *)a;
	memory_t *memory = (memory_t *)b;

	*total += memory->size;

	return -1;
}

// If a process needs to be swapped out, choose the one which has
// the largest size. If two processes are of equal size then
// choose the one which has been in memory longest.
int process_cmp(void *cmp1, void *cmp2)
{
	memory_t *m1 = (memory_t *)cmp1;
	memory_t *m2 = (memory_t *)cmp2;

	if (m1->addr < m2->addr)
		return -1;
	//else if (m1->addr == m2->addr)
	//	return 0;
	else
		return 1;
}

int remove_free(list_t *list, void *a, void *b)
{
	memory_t *m1 = (memory_t *)a;
	memory_t *m2 = (memory_t *)b;

	if (m1->addr == m2->addr)
	{
		//printf("Removal: %d",m1->addr);
		m1->addr += m2->size;
		m1->size -= m2->size;
		//printf("->%d\n",m1->addr);

		assert(m1->size >=0);
		// If a portion of free memory is reduced to
		// size zero then remove it.
		if (m1->size == 0)
		{
			list_remove(list, m1);
			//list_remove(list, m2);
			//printf("REMOVE\n");
		}

		return 1;
	}

	return 0;
}

int add_free_part(list_t *list, void *a, void *b)
{
	memory_t *m1 = (memory_t *)a;
	memory_t *m2 = (memory_t *)b;
	memory_t *next = NULL;

	//printf("B:\n");
	//print_free(list);
	if (m1->addr == m2->addr + m2->size)
	{
		//printf("lol\n");
		// Block which was freed is directly before a free block.
		// Extend the current block into the previous block.
		m1->addr = m2->addr;
		m1->size += m2->size;
	}
	else if (m1->addr + m1->size == m2->addr)
	{
		// Block which was freed occurs directly after a free block.
		// Extend the current free block into the next
		//printf("%d->",m1->size);
		m1->size += m2->size;
		//printf("%d\n", m1->size);
		//printf("jks\n");
		//print_free(list);
		
		if ((next = list_get_next(list, m1)) != NULL && m1->addr + m1->size == next->addr)
		{
			m1->size += next->size;
			//printf("man\n");
			//print_free(list);
			//printf("Remove item at addr=%d\n", next->addr);
			list_remove(list,next);
			//print_free(list);
			//list_modify(list, next, remove_free);
		}
	}
	else
	{
		//printf("add_free_part fail\n");

		return 0;
	}
	//printf("A:\n");
	//print_free(list);
	//printf("add_free_part success\n");
	return 1;
}

void print_free(list_t *free_list)
{
	printf("Free memory (addr,size) (%d): ", free_list->node_count);
	if (!list_is_empty(free_list))
		list_for_each(free_list, print_free_data);
	//if (free_list->foot != NULL)
	//	printf("\nfoot->next=%p.\n",free_list->foot->next);
	//else
	printf(".\n");
}

void print_free_data(void *data)
{
	memory_t *m = (memory_t *)data;
	printf(" (%d, %d)", m->addr, m->size);
}

void print_mem(list_t *memory)
{
	printf("Real memory (pid,addr,size) (%d): ", memory->node_count);
	if (!list_is_empty(memory))
		list_for_each(memory, print_memory_data);
	printf(".\n");
}

void print_memory_data(void *data)
{
	memory_t *m = (memory_t *)data;
	printf(" (%d, %d, %d)", m->process->pid, m->addr, m->size);
}

void print_que(list_t *queue)
{
	printf("Process Queue (pid,size,last_loaded,swap_count) (%d): ", queue->node_count);
	if (!list_is_empty(queue))
		list_for_each(queue, print_process_data);
	printf(".\n");
}

void print_process_data(void *data)
{
	process_t *p = (process_t *)data;
	printf(" (%d, %d, %d, %d)", p->pid, p->size, p->last_loaded, p->swap_count);
}
