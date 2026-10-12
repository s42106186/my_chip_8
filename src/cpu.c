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
  unsigned char x = 0;
  unsigned char y = 0;
  switch(opcode & 0xF000) {
    case 0x0000: 
      switch(opcode & 0x000F) {
        case 0x0000: //00E0 - clear dislplay
          ClearDisplay();
          pc += 2;
          break;
	case 0x000E: //000E - RET
	  --sp;
	  pc = stack[sp];
	  break;
      }
      break;
    case 0x1000: //1NNN - jump nnn
      pc = opcode & 0x0FFF;
      break;
    
    case 0x2000: //2NNN - call addr
        stack[sp] = pc+2;
	++sp;
        pc = opcode & 0x0FFF;
      break;
    case 0x3000: //3xkk - SE Vx, byte
      if(v[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
        pc += 4;
      else
	pc += 2;
      break;
    case 0x4000: //4xkk - SNE Vx, byte
      if(v[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
        pc += 4;
      else
	pc += 2;
      break;
    case 0x5000: //5xy0 - SE Vx, Vy 
      if(v[(opcode & 0x0F00) >> 8] == v[(opcode & 0x00F0) >> 4])
        pc += 4;
      else
	pc += 2;
      break;
    case 0x6000: //LD Vx
      v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
      pc += 2;
      break;
    
    case 0x7000: //ADD Vx
      v[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
      pc += 2;
      break;
    
    case 0x8000:
      switch(opcode & 0x000F) {
        case 0x0000: //8xy2 LD Vx, Vy
	  v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x00F0) >> 4];
	  pc += 2;
	  break;
        case 0x0001: //8xy2 OR Vx, Vy
	  v[(opcode & 0x0F00) >> 8] |= v[(opcode & 0x00F0) >> 4];
	  pc += 2;
	  break;
        case 0x0002: //8xy2 AND Vx, Vy
	  v[(opcode & 0x0F00) >> 8] &= v[(opcode & 0x00F0) >> 4];
	  pc += 2;
	  break;
        case 0x0003: //8xy3 XOR Vx, Vy
	  v[(opcode & 0x0F00) >> 8] ^= v[(opcode & 0x00F0) >> 4];
	  pc += 2;
	  break;
        case 0x0004: //8xy4 ADD Vx, Vy
	  unsigned char rem = 0xFF - v[(opcode & 0x0F00) >> 8];
	  v[(opcode & 0x0F00) >> 8] += v[(opcode & 0x00F0) >> 4];
	  v[0xF] = (v[(opcode & 0x00F0) >> 4] > rem) ? 1 : 0; 
	  pc += 2;
	  break;
        case 0x0005: //8xy5 SUB Vx, Vy
	  x = v[(opcode & 0x0F00) >> 8];
	  y = v[(opcode & 0x00F0) >> 4];
	  v[(opcode & 0x0F00) >> 8] -= y;
	  v[0xF] = (x >= y) ? 1 : 0;
	  pc += 2;
	  break;
        case 0x0006: //8xy6 SHR Vx {, Vy} 
	  x = v[(opcode & 0x0F00) >> 8];
	  v[(opcode & 0x0F00) >> 8] /= 2;
	  v[0xF] = ((x & 0x01) == 0x01) ? 1 : 0;
	  pc += 2;
	  break;
	case 0x0007: //8xy7 SUBN Vx, Vy
	  x = v[(opcode & 0x0F00) >> 8];
	  y = v[(opcode & 0x00F0) >> 4];
	  v[(opcode & 0x0F00) >> 8] = y - x;
	  v[0xF] = (y >= x) ? 1 : 0;
	  pc += 2;
	  break;
        case 0x000E: //8xyE SHL Vx {, Vy} 
	  x = v[(opcode & 0x0F00) >> 8];
	  v[(opcode & 0x0F00) >> 8] *= 2;
	  v[0xF] = ((x & 0x80) == 0x80) ? 1 : 0;
	  pc += 2;
	  break;

      }
      break;
    case 0x9000: //9xy0 SNE Vx, Vy
      if(v[(opcode & 0x0F00) >> 8] != v[(opcode & 0x00F0) >> 4])
        pc += 4;
      else
	pc += 2;
        break;
    case 0xA000: //Annn LD I
      i = opcode & 0x0FFF;
      pc += 2;
      break;
    case 0xB000: //Bnnn JP V0, addr
      pc = (opcode & 0xFFF) + v[0x0];
      break;
    case 0xC000: //Cxkk RND Vx, byte
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
      pc += 2;
      update = 1;
      break;
    /*
    keyboard
    case 0xE000: Ex91 ExA1
      switch(
    */
    case 0xF000:
      
      switch(opcode & 0x00FF) {
	case 0x0007: //Fx07 LD Vx, DT
	  v[(opcode & 0x0F00) >> 8] = dt;
          pc += 2;
	  break;
	/*
	keyboard
	case 0x000A: Fx0A
	  break;
	*/
	case 0x0015: //Fx15 LD DT, Vx
	  dt = v[(opcode & 0x0F00) >> 8];
          pc += 2;
	  break;
	case 0x0018: //Fx18 LD ST, Vx
	  st = v[(opcode & 0x0F00) >> 8];
          pc += 2;
	  break;
	case 0x001E: //Fx1E ADD I, Vx
	  i += v[(opcode & 0x0F00) >> 8];
          pc += 2;
	  break;
	case 0x0029: //Fx29
	  i = 0x050+5*(unsigned short)v[(opcode & 0x0F00) >> 8];
	  pc += 2;
	  break;
	case 0x0033: //Fx33 LD BSD, Vx
	  memory[i] = v[(opcode & 0x0F00) >> 8] / 100;
	  memory[i+1] = (v[(opcode & 0x0F00) >> 8] % 100) / 10;
	  memory[i+2] = v[(opcode & 0x0F00) >> 8] % 10;
	  pc += 2;
	  break;
	case 0x0055: //Fx55 LD [I], Vx
	  for(unsigned char k = 0x0; k < ((opcode & 0x0F00) >> 8)+1; ++k, ++i)
		  memory[i] = v[k];
	  pc += 2;
	  break;
	case 0x0065: //Fx65 LD Vx, [I] 
	  for(unsigned char k = 0x0; k < ((opcode & 0x0F00) >> 8)+1; ++k, ++i)
		  v[k] = memory[i];
	  pc += 2;
	  break;
      }
      break;
    default:
        printf ("Unknown opcode [0x0000]: 0x%X\n", opcode);
  }
  return;
}
