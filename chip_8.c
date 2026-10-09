#include "chip_8.h"

void Init()
{
	ClearStack(stack);
	ClearDisplay(display);
	ClearMemory(memory);
	ClearRegisters(v);
	pc = 0x200;
	opcode = 0;
	i = 0;
	sp = 0;
	//timers
	//load fonts	
}
