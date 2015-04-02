#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "process.h"
#include "process_size_file.h"

list_t *load_processes_from(char *filename)
{
	FILE *file;
	int pid, size;
	list_t *list;

	list = list_new(sizeof(process_t));

	file = fopen(filename, "r");
	assert(file != NULL);
	while (fscanf(file, "%d %d", &pid, &size) != EOF) {
		process_t *process = malloc(sizeof(process_t));
		process->pid = pid;
		process->size = size;
		process->last_loaded = 0;
		process->swap_count = 0;

		list_push(list,process);
	}

	return list;
}
