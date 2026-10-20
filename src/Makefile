CC = gcc
CFLAGS = -Wall -Wextra -Werror
LDFLAGS = -lSDL3

all: run.o chip_8.o 
	$(CC) -o chip8 run.o chip_8.o $(LDFLAGS)

run.o: run.c
	$(CC) $(CFLAGS) -c run.c

chip_8.o: chip_8.c chip_8.h
	$(CC) $(CFLAGS) -c chip_8.c chip_8.h

clean:
	rm -f *.o *.gch chip8 

