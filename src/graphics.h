#ifndef GRAPHICS 
#define GRAPHICS 

#include <stdio.h>

extern unsigned char display[2048];
extern unsigned int display2[2048];
extern unsigned char update;

void ClearDisplay();
void UpdateDisplay();

#endif
