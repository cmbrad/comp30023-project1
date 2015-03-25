#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>



int main(int argc, char **argv)
{
	int c, memsize;
	char *filename, *algorithm_name;
	
	while ((c=getopt(argc, argv, "afm:")) != -1)
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
				printf("");
				return 1;
		}
	}
	printf("filename: %s algorithm_name: %s memsize: %d\n", filename, algorithm_name, memsize);
	return 0;
}
