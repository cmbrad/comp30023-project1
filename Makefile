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
memswap.o: memswap.c list.h process.h process_size_file.h
list.o: list.c list.h
process_size_file.o: process_size_file.c list.h process.h
first_fit.o: process.h
#process.o: process.h
