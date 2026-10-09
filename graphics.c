#include "graphics.h"

unsigned char display[64*32];

void ClearDisplay(unsigned char display[64*32])
{
  for(unsigned short i = 0; i < 64*32; ++i)
	  display[i] = 0x000;
}
