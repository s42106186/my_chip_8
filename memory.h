#ifndef MEMORY 
#define MEMORY 

//0x000-0xFFF, 0x000-0x1FF - inpterpreter, 0x200-0xFFF - RAM
extern unsigned char memory[4096];

void ClearMemory(unsigned char display[4096]);

#endif
