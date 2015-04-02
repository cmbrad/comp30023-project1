#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "list.h"
#include "process.h"
#include "process_size_file.h"

#define USAGE "./memswap -a [algorithm name] -f [filename] -m [memsize]"

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
	list_t *process_list = list_new(sizeof(process_t));
	process_list = load_processes_from(filename, process_list);

	// Assume memory is initially empty.
	//memory_t memory = new_memory(memsize);
	//list_t *free_list = new_list(sizeof());
	
	// Function pointers we use to load into memory - they correspond to what algorithm we chose earlier.

	// Load the processes from the queue into memory, one by one, according to one of the four algorithms.
	node_t *cur = process_list->head;
	do
	{
		//load_process(cur->process);
		printf("%d loaded, numprocesses=%d, numholes=%d, memusage=%d%%\n", ((process_t *)cur->data)->pid,0,0,0);
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
