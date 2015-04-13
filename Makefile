# Author: Chris Bradley (635847)

CC = gcc
CFLAGS = -Wall

SRC = memswap.c list.c process_size_file.c #process.c
OBJ = memswap.o list.o process_size_file.o #process.o
EXE = memswap

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ) -lm

clean:
	rm $(OBJ)

clobber: clean
	rm $(EXE)

## Dependencies
memswap.o: Makefile memswap.c list.h process.h process_size_file.h memory.h
list.o: Makefile list.c list.h
process_size_file.o: Makefile process_size_file.c list.h process.h
first_fit.o: Makefile process.h
