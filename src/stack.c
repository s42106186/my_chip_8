#include "stack.h"

unsigned short stack[16];
unsigned short sp;

void ClearStack()
{
  for(unsigned char i = 0; i < 16; ++i)
	  stack[i] = 0x000;
}
