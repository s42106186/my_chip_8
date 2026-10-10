#include "cpu.h"

unsigned char v[16];
unsigned char dt;
unsigned char st;
unsigned short i;
unsigned short pc;
unsigned short opcode;

void ClearRegisters()
{
  for(unsigned char i = 0x0; i < 0xF; ++i)
	  v[i] = 0;
  return;
}

void EmulateCycle()
{
  opcode = memory[pc] << 8 | memory[pc+1];
  switch(opcode & 0xF000) {
    case 0x0000: //00E0 - clear dislplay
      ClearDisplay();
      break;
    case 0x1000: //1NNN - jump nnn
      pc = opcode & 0x0FFF;
      break;
    case 0x6000: //LD Vx
      v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
      break;
    case 0x7000: //ADD Vx
      v[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
      break;
    case 0xA000: //LD I
      i = opcode & 0x0FFF;
      break;
    case 0xD000:
      unsigned short x = v[(opcode & 0x0F00) >> 8];
      unsigned short y = v[(opcode & 0x00F0) >> 4];
      unsigned short h = opcode & 0x000F;
      unsigned short pixel;
      v[0xF] = 0;

      for(unsigned short yline = 0; yline < h; ++yline) {
        pixel = memory[i+yline];
	for(unsigned short xline = 0; xline < 8; ++xline) {
	  if((pixel & (0x80 >> xline)) != 0) {
	    if(display[x + xline + ((y + yline) * 64)] == 1)
	      v[0xF] = 1;
	    display[x + xline + ((y + yline) * 64)] ^= 1;
	  }
	}
      }
      update = 1;
      break;
    default:
        printf ("Unknown opcode [0x0000]: 0x%X\n", opcode);
  }
  pc += 2;
  return;
}
