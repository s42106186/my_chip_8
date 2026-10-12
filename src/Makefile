CC = gcc 
CFLAGS = -I/usr/include/pipewire-0.3 -I/usr/include/spa-0.2 \
	 -D_REENTRANT -I/usr/include/libdrm \
	 -I/usr/include/libdecor-0  
LDFLAGS = -lSDL3

all: run.o chip_8.o graphics.o keyboard.o memory.o stack.o cpu.o 
	$(CC) -o chip_8 run.o chip_8.o graphics.o \
	keyboard.o memory.o stack.o cpu.o $(LDFLAGS)
	./chip_8

run.o: run.c
	$(CC) $(CFLAGS) -c run.c

chip_8.o: chip_8.c chip_8.h
	$(CC) $(CFLAGS) -c chip_8.c chip_8.h

graphics.o: graphics.c graphics.h
	$(CC) $(CFLAGS) -c graphics.c graphics.h

keyboard.o: keyboard.c keyboard.h
	$(CC) $(CFLAGS) -c keyboard.c keyboard.h

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c memory.c memory.h

stack.o: stack.c stack.h
	$(CC) $(CFLAGS) -c stack.c stack.h

cpu.o: cpu.c cpu.h
	$(CC) $(CFLAGS) -c cpu.c cpu.h

clean:
	rm -f *.o *.gch chip_8 

