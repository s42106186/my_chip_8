#include "graphics.h"

unsigned char display[2048];
unsigned char update = 0;

void ClearDisplay()
{
  for(size_t i = 0; i < 2048; ++i)
	  display[i] = 0;
}

void UpdateDisplay()
{
  for(size_t i = 0; i < 2048; ++i) {
    if(i % 64 == 0)
	    printf("|");
    if(display[i] == 1)
      printf("#");
    else
      printf(" ");
  }
  putchar('\n');

  for(unsigned char i = 0; i < 64; ++i) {
    putchar('-');
  }
  return;
}

