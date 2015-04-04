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

typedef memory_t *(*addr_func)(memory_t *, memory_t *, process_t *);

process_t *swap_process(list_t *memory, list_t *free_list);
void add_free(list_t *free_list, memory_t *rem);
int cmp(void *cmp1, void *cmp2);
void print_free(list_t *free_list);
void rem_free(list_t *free_list, memory_t *rem);
void print_mem(list_t *memory);
void print_que(list_t *queue);
int get_mem_usage(list_t *memory);
int match_process(void *d1, void *d2);
//int get_addr(list_t *memory, list_t *free_list, list_t *process_list, process_t *process, int (*addr_func)(list_t *list, process_t *process));

int get_addr(list_t *, process_t *, addr_func);
memory_t *best_get_addr(memory_t *best, memory_t *cand, process_t *process);
memory_t *first_get_addr(memory_t *best, memory_t *cand, process_t *process);
memory_t *worst_get_addr(memory_t *best, memory_t *cand, process_t *process);
memory_t *next_get_addr(memory_t *best, memory_t *cand, process_t *process);

int main(int argc, char **argv)
{
	int c, memsize;
	char *filename, *algorithm_name;

	memsize = -1;
	filename = algorithm_name = NULL;	
	while ((c=getopt(argc, argv, "a:f:m:")) != -1)
	{
		switch (c)
		{
			case 'a':
				algorithm_name = optarg;
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				memsize = atoi(optarg);
				break;
			default:
				printf(USAGE);
				return 1;
		}
	}

	int all_given = 1;
	if (filename == NULL) {
		printf("ERROR: No filename specified.\n");
		all_given = 0;
	} if (algorithm_name == NULL) {
		printf("ERROR: No algorithm specified.\n");
		all_given = 0;
	} if (memsize <= 0) {
		printf("ERROR: Memory size must be greater than 0.\n");
		all_given = 0;
	}

	if (!all_given) {
		return 1;
	}
	//printf("filename: %s algorithm_name: %s memsize: %d\n", filename, algorithm_name, memsize);

	// Parse the process file to obtain the initial queue of processes waiting to be swapped into memory.
	list_t *process_list = load_processes_from(filename);

	// Assume memory is initially empty.
	list_t *memory = list_new(sizeof(process_t));

	// Free list holds all the cards!
	list_t *free_list = list_new(sizeof(memory_t));
	
	memory_t *init_memory = malloc(sizeof(memory_t));
	init_memory->process = NULL;
	init_memory->addr = 0;
	init_memory->size = memsize;
	
	list_push(free_list, init_memory);

	// Function pointers we use to load into memory - they correspond to what algorithm we chose earlier.
	//void (*load_process)(process_t);

	addr_func alg_get_addr  = NULL;
	if (strcmp(algorithm_name, "first") == 0)
		alg_get_addr = &first_get_addr;
	else if (strcmp(algorithm_name, "best") == 0)
		alg_get_addr = &best_get_addr;
	else if (strcmp(algorithm_name, "worst") == 0)
		alg_get_addr = &worst_get_addr;
	//else if (strcmp(algorithm_name, "next") == 0)
	//	get_addr = &next_get_addr;
	else
	{
		printf("ERROR: Invalid algorithm specified.\n");
		return 1;
	}
	int time = 0;
	// Load the processes from the queue into memory, one by one, according to one of the four algorithms.
	node_t *cur = process_list->head;
	do
	{
		memory_t *new_mem = malloc(sizeof(memory_t));
		new_mem->process = (process_t *)cur->data;
		new_mem->process->last_loaded = time;
		new_mem->size = new_mem->process->size;
		
		// Swap out processes until we have enough space to allocate to
		// the current process
		while ((new_mem->addr = get_addr(free_list, new_mem->process, alg_get_addr)) == -1)
		{
			// Swap out a process to make room to allocate memory
			process_t *to_swap = swap_process(memory, free_list);
			// Processes will not be requeued for execution if they've
			// ben swapped SWAP_LIMIT times. Otherwise,
			// add to the back of the queue.
			if (to_swap->swap_count < SWAP_LIMIT)
				list_push(process_list, to_swap);
		}
		list_push(memory, new_mem);
		rem_free(free_list, new_mem);
		
		printf("%d loaded, numprocesses=%d, numholes=%d, memusage=%d%%\n", new_mem->process->pid,memory->node_count,free_list->node_count,(int)(ceil(100*((float)get_mem_usage(memory)/memsize))));
		///print_free(free_list);
		///print_mem(memory);
		///print_que(process_list);

		// Time advances at a constant rate! In this universe anyway..
		time += 1;
	} while ((cur = cur->next));

	// If a process needs to be swapped out, choose the one which has the largest size. If two processes
	// have equal largest size, choose the one which has been in memory the longest (measured from the
	// time it was most recently placed in memory).

	// After a process has been swapped out, it is placed at the end of the queue of processes waiting to
	// be swapped in.

	// Once a process has been swapped out for the third time, we assume the process has finished and
	// it is not re-queued. Note that not all processes will be swapped out for three times.

	// The simulation should terminate once no more processes are waiting to be swapped into memory

	// Clean up
	list_destroy(memory);
	list_destroy(free_list);
	list_destroy(process_list);

	return 0;
}

int get_addr(list_t *free_list, process_t *process, addr_func get_new_addr)
{
	memory_t *chosen = NULL;
	node_t *cur = free_list->head;

	if (list_is_empty(free_list))
		return -1;

	do {
		memory_t *cur_mem = (memory_t *)cur->data;
		if (cur_mem->size >= process->size)
			chosen = get_new_addr(chosen, cur_mem, process);
	} while ((cur = cur->next));

	return chosen != NULL ? chosen->addr : -1;
}

memory_t *first_get_addr(memory_t *best, memory_t *cand, process_t *process)
{
	if(best == NULL)
		return cand;
	return best;
}

memory_t *best_get_addr(memory_t *best, memory_t *cand, process_t *process)
{
	if(best == NULL || best->size >= cand->size)
		return cand;
	return best;
}

memory_t *worst_get_addr(memory_t *best, memory_t *cand, process_t *process)
{
	if(best == NULL || best->size <= cand->size)
		return cand;
	return best;
}

process_t *swap_process(list_t *memory, list_t *free_list)
{
	process_t *cur_proc = NULL;
	memory_t *to_swap = NULL;
	memory_t *cur_mem = NULL;
	node_t *cur_node = memory->head;

	assert(cur_node != NULL);
	do {
		cur_mem = ((memory_t *)cur_node->data);
		cur_proc = cur_mem->process;
		if (to_swap == NULL ||
			cur_proc->size > to_swap->process->size ||
			(cur_proc->size == to_swap->process->size && cur_proc->last_loaded < to_swap->process->last_loaded))
			to_swap = cur_mem;
	} while ((cur_node = cur_node->next));

	list_remove(memory, to_swap, &match_process);
	add_free(free_list, to_swap);
	
	to_swap->process->swap_count++;

	assert(to_swap != NULL);
	return to_swap->process;
}

void add_free(list_t *free_list, memory_t *rem)
{
	memory_t *cur_mem = NULL;
	node_t *cur_node = free_list->head;
	int soln_found = 0;

	do {
		// cur_node may be NULL if memory is full.
		if (cur_node == NULL)
			break;

		cur_mem = (memory_t *)cur_node->data;
		if(cur_mem->addr == rem->addr + rem->size)
		{
			// Block which was freed is directly before a free block.
			// Extend the current block into the previous block.
			cur_mem->addr = rem->addr;
			cur_mem->size += rem->size;
			soln_found = 1;
			break;
		}
		else if (cur_mem->addr + cur_mem->size == rem->addr)
		{
			// Block which was freed occurs directly after a free block.
			// Extend the current free block into the next
			cur_mem->size += rem->size;
			soln_found = 1;

			if(cur_node->next != NULL && cur_mem->addr + cur_mem->size == ((memory_t *)cur_node->next->data)->addr)
			{
				///printf("woo hoo consolidate\n");
				cur_mem->size += ((memory_t *)cur_node->next->data)->size;
				rem_free(free_list, (memory_t *)cur_node->next->data);
			}
			break;
		}
	} while ((cur_node = cur_node->next));

	if (!soln_found)
	{
		// Block is not adjacent to any curent free blocks.
		// Make a new free block.
		memory_t *new_free = malloc(sizeof(memory_t));
		new_free->process = NULL;
		new_free->addr = rem->addr;
		new_free->size = rem->size;

		list_push_o(free_list, new_free, cmp);
	}
}

void rem_free(list_t *free_list, memory_t *rem)
{
	memory_t *cur_mem = NULL;
	node_t *pre_node = NULL;
	node_t *cur_node = free_list->head;

	assert(cur_node != NULL);
	do {
		cur_mem = (memory_t *)cur_node->data;
		if(cur_mem->addr == rem->addr)
		{
			// Block which was freed is directly before a free block.
			// Add it to that free block.
			cur_mem->addr += rem->size;
			cur_mem->size -= rem->size;

			if (cur_mem->size == 0)
			{
				// remove
				if (pre_node == NULL)
					free_list->head = cur_node->next;
				else
					pre_node->next = cur_node->next;
				//free(cur_node);
				free_list->node_count--;
			}
			///printf("Shrinking free space...\n");
			break;
		}
		pre_node = cur_node;
	} while ((cur_node = cur_node->next));
	///print_free(free_list);
}

int get_mem_usage(list_t *memory)
{
	int usage = 0;
	memory_t *cur_mem = NULL;
	node_t *cur_node = memory->head;

	assert(cur_node != NULL);
	do {
		cur_mem = (memory_t *)cur_node->data;
		usage += cur_mem->size;
	} while ((cur_node = cur_node->next));
	return usage;
}

void print_free(list_t *free_list)
{
	memory_t *cur_mem = NULL;
	node_t *cur_node = free_list->head;

	assert(cur_node != NULL);
	printf("Free memory: ");
	do {
		cur_mem = (memory_t *)cur_node->data;
		printf(" (%d, %d)", cur_mem->addr, cur_mem->size);
	} while ((cur_node = cur_node->next));
	printf(".\n");
}

void print_mem(list_t *memory)
{
	memory_t *cur_mem = NULL;
	node_t *cur_node = memory->head;

	assert(cur_node != NULL);
	printf("Real memory (%d): ", memory->node_count);
	do {
		cur_mem = (memory_t *)cur_node->data;
		printf(" (addr=%d, size=%d, pid=%d)", cur_mem->addr, cur_mem->size, cur_mem->process->pid);
	} while ((cur_node = cur_node->next));
	printf(".\n");
}

void print_que(list_t *queue)
{
	process_t *cur_proc = NULL;
	node_t *cur_node = queue->head;

	assert(cur_node != NULL);
	printf("Process Queue: ");
	do {
		cur_proc = (process_t *)cur_node->data;
		printf(" (pid=%d, size=%d, last_loaded=%d, swap_count=%d)", cur_proc->pid, cur_proc->size, cur_proc->last_loaded, cur_proc->swap_count);
	} while ((cur_node = cur_node->next));
	printf(".\n");
}

int cmp(void *cmp1, void *cmp2)
{
	memory_t *m1 = (memory_t *)cmp1;
	memory_t *m2 = (memory_t *)cmp2;

	if (m1->addr + m1->size > m2->addr)
		return -1;
	else if (m1->addr + m1->size == m2->addr)
		return 0;
	else
		return 1;
}

int match_process(void *d1, void *d2)
{
	process_t *p1 = ((memory_t *)d1)->process;
	process_t *p2 = ((memory_t *)d2)->process;

	return p1->pid == p2->pid;
}
