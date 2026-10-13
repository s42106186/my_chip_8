CC = gcc 
CFLAGS = -I/usr/include/pipewire-0.3 -I/usr/include/spa-0.2 \
	 -D_REENTRANT -I/usr/include/libdrm \
	 -I/usr/include/libdecor-0  
LDFLAGS = -lSDL3

all: run.o chip_8.o 
	$(CC) -o chip_8 run.o chip_8.o $(LDFLAGS)
	./chip_8

run.o: run.c
	$(CC) $(CFLAGS) -c run.c

chip_8.o: chip_8.c chip_8.h
	$(CC) $(CFLAGS) -c chip_8.c chip_8.h

clean:
	rm -f *.o *.gch chip_8 

