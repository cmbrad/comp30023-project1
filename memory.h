/* Author: Chris Bradley (635 847)
 * Contact: chris.bradley@cy.id.au */

#include "process.h"

typedef struct memory {
	process_t *process;
	int addr;	// Address
	int size;
} memory_t;
