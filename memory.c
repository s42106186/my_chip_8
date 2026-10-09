#include "memory.h"

unsigned char memory[4096];

void ClearMemory(unsigned char memory[4096])
{
  for(unsigned short i = 0; i < 4096; ++i)
	  memory[i] = 0x000;
}
