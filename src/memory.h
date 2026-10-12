#ifndef MEMORY 
#define MEMORY 

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define MEM_SIZE 4096
#define RAM_SIZE 512 

//0x000-0xFFF, 0x000-0x1FF - inpterpreter, 0x200-0xFFF - RAM
extern unsigned char memory[MEM_SIZE];

void ClearMemory();
void WriteFontsToMemory();
void WriteRomsToMemory();

#endif
