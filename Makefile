CC = gcc
CFLAGS = -Wall

SRC = memswap.c process_list.c process_size_file.c #process.c
OBJ = memswap.o process_list.o process_size_file.o #process.o
EXE = memswap

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ) -lm

clean:
	rm $(OBJ)

clobber: clean
	rm $(EXE)

## Dependencies
memswap.o: memswap.c process_list.h process.h process_size_file.h
process_list.o: process_list.c process_list.h process.h
process_size_file.o: process_size_file.c process_list.c
#process.o: process.c
