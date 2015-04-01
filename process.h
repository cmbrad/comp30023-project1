#ifndef INCLUDE_PROCESS_H
#define INCLUDE_PROCESS_H
typedef struct process {
	int pid;		// Process ID
	int size;		// Size in MB
	int last_loaded;	// Time stamp last loaded
} process_t;
#endif
