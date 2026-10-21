#ifndef CHIP8
#define CHIP8

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define V_SIZE 16

#define MEM_SIZE 4096
#define RAM_SIZE 512

#define STACK_SIZE 16

#define DISPLAY_SIZE 2048
#define PIXEL_ON 0xFFFFFFFFu
#define PIXEL_OFF 0xFF000000u

#define KEYBOARD_SIZE 16

typedef struct {
  // cpu
  uint8_t *v;
  uint8_t dt;
  uint8_t st;
  uint16_t i;
  uint16_t pc;
  uint16_t opcode;
  // memory
  uint8_t *memory;
  // stack
  uint16_t *stack;
  uint8_t sp;
  // graphics
  uint32_t *display;
  bool update;
  bool vblank;
  // keyboard
  uint8_t *keyboard;
  int8_t pressed_key;
} Chip8;

bool InitChip8(Chip8 *chip8);
void FreeChip8(Chip8 *chip8);
bool WriteFontsToMemory(uint8_t *memory);
bool WriteRomsToMemory(uint8_t *memory, int argc, char *argv[]);
void EmulateCycle(Chip8 *chip8);

#endif
