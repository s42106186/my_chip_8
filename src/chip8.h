#ifndef CHIP8
#define CHIP8

#include <stdbool.h>
#include <stdint.h>

enum {
  kDisplayWidth = 64,
  kDisplayHeight = 32,
  kDisplaySize = kDisplayWidth * kDisplayHeight,
  kSizeV = 16,
  kMemorySize = 4096,
  kStackSize = 16,
  kKeyboardSize = 16,
};

typedef struct {
  uint8_t v[kSizeV];  // Список регистров V0-VF
  int delay_timer;
  int sound_timer;
  uint16_t i;                // Используется для адресации по памяти
  uint16_t program_counter;  // Хранит текущий адрес выполнения программы
  uint16_t opcode;           // Текущая инструкция
  uint8_t memory[kMemorySize];
  uint16_t stack[kStackSize];
  int stack_pointer;  // Указатель уровеня стека
  uint32_t display[kDisplaySize];
  bool update;  // Указывает, что экран изменился
  bool vblank;  // Блокирует выполнение процессора
  uint8_t keyboard[kKeyboardSize];
  int pressed_key;  // Запоминает текущую нажатую клавишу
} Chip8;

// Присваиваение значений полям структуры
void InitChip8(Chip8* chip8);
// Запись шрифтов в память
void WriteFontsToMemory(uint8_t* memory);
// Запись программы в память
bool WriteRomsToMemory(uint8_t* memory, int argc, char* argv[]);
// Эмуляция одного цикла процессора
void EmulateCycle(Chip8* chip8);

#endif  // CHIP8
