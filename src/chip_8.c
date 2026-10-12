#include "chip_8.h"

void Init()
{
	ClearStack();
	ClearDisplay();
	ClearMemory();
	ClearRegisters();
	opcode = 0;
	pc = 0x200;
	sp = 0;
        dt = 0;
	st = 0;
	i = 0;
	WriteFontsToMemory();	
	WriteRomsToMemory();	
}

