CC = gcc
CFLAGS = -Wall -Wextra -Werror 
LDFLAGS = -lSDL3
OBJS = run.o chip8.o

chip8: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c chip8.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o chip8
