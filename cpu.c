#include "cpu.h"

unsigned char v[16];
unsigned char dt;
unsigned char st;
unsigned short i;
unsigned short pc;
unsigned short opcode;

void ClearRegisters(unsigned char v[16])
{
  for(unsigned char i = 0; i < 16; ++i)
	  v[i] = 0x000;
}
