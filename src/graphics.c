#include "graphics.h"

unsigned char display[2048];
unsigned int display2[2048];
unsigned char update = 0;

void ClearDisplay()
{
  for(size_t i = 0; i < 2048; ++i)
	  display[i] = 0;
}

void UpdateDisplay()
{
  for(size_t i = 0; i < 2048; ++i) {
    if(display[i] == 1)
      display2[i] = 0xFFFFFFFF;
    else
      display2[i] = 0xFF000000;
  }
  return;
}
