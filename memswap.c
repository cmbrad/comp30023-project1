#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "list.h"
#include "process.h"
#include "process_size_file.h"
#include "memory.h"

#define USAGE "./memswap -a [algorithm name] -f [filename] -m [memsize]"

#define SWAP_LIMIT 3

int get_addr(list_t *list, process_t *process);
process_t *swap_process(list_t *memory, list_t *free_list);
void remove_process(list_t *list, process_t *process);
void add_free(list_t *free_list, memory_t *rem);
int cmp(void *cmp1, void *cmp2);
void print_free(list_t *free_list);
void rem_free(list_t *free_list, memory_t *rem);
void print_mem(list_t *memory);
void print_que(list_t *queue);

int main(int argc, char **argv)
{
	int c, memsize;
	char *filename, *algorithm_name;

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

	
	printf("filename: %s algorithm_name: %s memsize: %d\n", filename, algorithm_name, memsize);

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

	// void load_process(list_t *free_list, process_t *process, void (*fit_function)(list_t *free_list, process_t *process));

	// Load the processes from the queue into memory, one by one, according to one of the four algorithms.
	node_t *cur = process_list->head;
	do
	{
		memory_t *new_mem = malloc(sizeof(memory_t));
		new_mem->process = (process_t *)cur->data;
		// new_mem->addr = get_addr(free_list, new_mem->process);
		new_mem->size = new_mem->process->size;

		int addr = get_addr(free_list, new_mem->process);
		while (addr == -1)
		{
			process_t *to_swap = swap_process(memory, free_list);
			addr = get_addr(free_list, new_mem->process);
			if (to_swap->swap_count <= SWAP_LIMIT) {
				printf("Queued process %d.\n", to_swap->pid);
				list_push(process_list, to_swap);
			} else
				printf("Process %d finished. (%d)\n", to_swap->pid, to_swap->swap_count);
		}
		new_mem->addr = addr;

		//printf("%d\n", new_mem->addr);
		//(*load_process)(cur->process);


		//printf("GUARD S\n");
		//print_free(free_list);
		//printf("GUARD F\n");
		list_push(memory, new_mem);

		//memory_t *old_free = list_pop(free_list);
		//memory_t *new_free = malloc(sizeof(memory_t));
		//new_free->process = NULL;
		//new_free->addr = new_mem->addr + new_mem->size;
		//new_free->size = old_free->size - new_mem->size;
		
		rem_free(free_list, new_mem);
		//add_free(free_list, new_free);
		
	//	//list_push_o(free_list, new_free, cmp);
		
		printf("%d loaded, numprocesses=%d, numholes=%d, memusage=%d%%\n", new_mem->process->pid,memory->node_count,free_list->node_count,0);
		print_free(free_list);
		print_mem(memory);
		print_que(process_list);
	} while ((cur = cur->next));

	// If a process needs to be loaded, but there is no hole large enough
	// to fit it, then processes should be swapped out, one by one, until
	// there is a hole large enough to hold the process needing to be loaded.
	
	// If a process needs to be swapped out, choose the one which has the largest size. If two processes
	// have equal largest size, choose the one which has been in memory the longest (measured from the
	// time it was most recently placed in memory).

	// After a process has been swapped out, it is placed at the end of the queue of processes waiting to
	// be swapped in.

	// Once a process has been swapped out for the third time, we assume the process has finished and
	// it is not re-queued. Note that not all processes will be swapped out for three times.

	// The simulation should terminate once no more processes are waiting to be swapped into memory

	return 0;
}

int get_addr(list_t *free_list, process_t *process)
{
	int addr = -1;

	node_t *cur = free_list->head;
	do {
		memory_t *cur_mem = (memory_t *)cur->data;
		if (cur_mem->size >= process->size)
			return cur_mem->addr;
	} while ((cur = cur->next));

	return addr;
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
		{
			//if( to_swap != NULL)
			//	printf("better: (%d,%d) > (%d,%d)\n", cur_proc->size, cur_proc->last_loaded, to_swap->process->size, to_swap->process->last_loaded);
			//else
			//	printf("Null..ehe\n");
			to_swap = cur_mem;
		}
		//else
		//	printf("worse: (%d,%d) > (%d,%d)\n", cur_proc->size, cur_proc->last_loaded, to_swap->process->size, to_swap->process->last_loaded);
	} while ((cur_node = cur_node->next));

	//printf("Chose pid=%d\n", to_swap->process->pid);
	remove_process(memory, to_swap->process);
	add_free(free_list, to_swap);

	//printf("Swapping out %d\n", to_swap->process->pid);
	
	to_swap->process->swap_count++;
	return to_swap->process;
}

void add_free(list_t *free_list, memory_t *rem)
{
	memory_t *cur_mem = NULL;
	node_t *cur_node = free_list->head;
	int soln_found = 0;

	//printf("Freeing pid=%d, addr=%d, size=%d\n", rem->process->pid, rem->addr, rem->process->size);
	do {
		cur_mem = (memory_t *)cur_node->data;
		if(cur_mem->addr == rem->addr + rem->size)
		{
			// Block which was freed is directly before a free block.
			// Add it to that free block.
			cur_mem->addr = rem->addr;
			cur_mem->size += rem->size;
			soln_found = 1;
			printf("extend into prev block.\n");

			break;
		}
		else if (cur_mem->addr + cur_mem->size == rem->addr)
		{
			// Block which was freed occurs directly after a free block.
			// Add it to that free block.
			cur_mem->size += rem->size;
			soln_found = 1;
			printf("extend into next block\n");

			if(cur_node->next != NULL && cur_mem->addr + cur_mem->size == ((memory_t *)cur_node->next->data)->addr)
			{
				printf("woo hoo consolidate\n");
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
		printf("make a new hole.\n");
	}
	//print_free(free_list);
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
			printf("Shrinking free space...\n");
			break;
		}
		pre_node = cur_node;
	} while ((cur_node = cur_node->next));
}

void print_free(list_t *free_list)
{
	memory_t *cur_mem = NULL;
	node_t *cur_node = free_list->head;

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
	printf("Real memory: ");
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

	//printf("Comparing m1: addr=%d size=%d with m2: addr=%d size=%d\n",m1->addr,m1->size,m2->addr,m2->size);
	if (m1->addr + m1->size > m2->addr)
		return -1;
	else if (m1->addr + m1->size == m2->addr)
		return 0;
	else
		return 1;
}

void remove_process(list_t *list, process_t *process)
{
	process_t *cur_proc = NULL;
	node_t *pre_node = NULL;
	node_t *cur_node = list->head;
	
	do {
		cur_proc = ((memory_t *)cur_node->data)->process;
		if(cur_proc->pid == process->pid)
		{
			if(pre_node != NULL)
				pre_node->next = cur_node->next;
			else
				list->head = cur_node->next;
			// free(cur_node)
			list->node_count--;

			//printf("Removed node for pid=%d.\n", process->pid);
			return;
		} //else
			//printf("%d does not match %d\n", cur_proc->pid, process->pid);
		//printf("%d %d \n", process->pid, process->size);
		pre_node = cur_node;
	} while ((cur_node = cur_node->next));
	
}
