#include "chip_8.h"

int main(void)
{
	Init();
	WriteRomsToMemory();
	for(; ;) {
		update = 0;
		EmulateCycle();
	//	printf("0x%X ", opcode);
		if(update)
 		  UpdateDisplay();
	}
	return 0;
}
