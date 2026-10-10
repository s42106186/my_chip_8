#include "chip_8.h"

int main(void)
{
	Init();
	WriteRomsToMemory();
	for(int i = 0; i < 39; ++i) {
		update = 0;
		EmulateCycle();
		//printf("0x%X ", opcode);
		if(update)
		  UpdateDisplay();
	}
	return 0;
}
