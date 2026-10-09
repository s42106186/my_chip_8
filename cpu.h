#ifndef CPU 
#define CPU 

//registers 0-E - general, F - flag
extern unsigned char v[16];
//register - delay timer
extern unsigned char dt;
//register - sound timer
extern unsigned char st;
//register for memmory address
extern unsigned short i;
//register - programm counter
extern unsigned short pc;

//code operation
extern unsigned short opcode;

void ClearRegisters(unsigned char v[16]);

#endif
