#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "process_size_file.h"

list_t *load_processes_from(char *filename, list_t *list)
{
	FILE *file;
	int pid, size;
	file = fopen(filename, "r");
	assert(file != NULL);
	while (fscanf(file, "%d %d", &pid, &size) != EOF) {
		process_t *process = malloc(sizeof(process_t));
		process->pid = pid;
		process->size = size;
		process->last_loaded = 0;

		list = push(list,process);
	}

	return list;
}
