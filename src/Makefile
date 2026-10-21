CC = gcc
CFLAGS = -Wall -Wextra -Werror
LDFLAGS = -lSDL3

all: run.o chip8.o 
	$(CC) -o chip8 run.o chip8.o $(LDFLAGS)

run.o: run.c
	$(CC) $(CFLAGS) -c run.c

chip8.o: chip8.c chip8.h
	$(CC) $(CFLAGS) -c chip8.c chip8.h

clean:
	rm -f *.o *.gch chip8 

