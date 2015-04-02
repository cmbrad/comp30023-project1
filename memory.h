#include "process.h"

typedef struct memory {
	process_t *process;
	int addr;	// Address
	int size;
} memory_t;
