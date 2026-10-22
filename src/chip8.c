#include "chip8.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
  kProgramStart = 0x200,
  kStartFonts = 0x050,
};

/*
Массив экрана будет состоять из черных и белых пикселей.
Включенный пикслель означает белые цвет, а выключенный - черный.
Такие числа выбраны специально для SDL. На основе массивы таких чисел
обновляется текстура.
*/
static const uint32_t kPixelOn = 0xFFFFFFFFu;
static const uint32_t kPixelOff = 0xFF000000u;

static void ClearDisplay(uint32_t* display) {
  // Заполняем экран черным цветом
  for (int i = 0; i < kDisplaySize; ++i) display[i] = kPixelOff;
  return;
}

void InitChip8(Chip8* chip8) {
  chip8->delay_timer = 0;
  chip8->sound_timer = 0;

  chip8->i = 0x000;
  chip8->opcode = 0x000;
  chip8->program_counter = kProgramStart;

  chip8->stack_pointer = 0;

  ClearDisplay(chip8->display);
  chip8->update = true;
  chip8->vblank = true;

  chip8->pressed_key = -1;
}

void WriteFontsToMemory(uint8_t* memory) {
  const unsigned char kFonts[80] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
      0x20, 0x60, 0x20, 0x20, 0x70,  // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
      0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
      0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
      0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
      0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
      0xF0, 0x80, 0xF0, 0x80, 0x80   // F
  };

  for (int i = 0; i < 80; ++i) memory[kStartFonts + i] = kFonts[i];
}

bool WriteRomsToMemory(uint8_t* memory, int argc, char* argv[]) {
  if (argc != 2) {
    printf("Enter the name of one game!\n");
    return false;
  }

  bool res = true;

  char game_path[256];
  snprintf(game_path, sizeof(game_path), "../roms/%s.ch8", argv[1]);
  int fd = open(game_path, O_RDONLY);
  if (fd == -1) {
    perror(game_path);
    res = false;
  }

  if (res) {
    struct stat st;
    if (fstat(fd, &st) == -1) {
      perror("Stat error!\n");
      res = false;
    } else {
      if (st.st_size <= (kMemorySize - kProgramStart)) {
        if (read(fd, memory + kProgramStart, st.st_size) == -1) {
          perror("Read error!\n");
          res = false;
        }
      } else {
        printf("Big size rom!\n");
        res = false;
      }
    }
    close(fd);
  }
  return res;
}

static void Opcode00E0(Chip8* chip8) {
  ClearDisplay(chip8->display);
  chip8->update = true;
  chip8->program_counter += 2;
}

// Возвращает из подпрограммы
static void Opcode00EE(Chip8* chip8) {
  if (chip8->stack_pointer > 0) --(chip8->stack_pointer);
  chip8->program_counter = chip8->stack[chip8->stack_pointer];
}

// Перемещает счётчик программ на адресс nnn
static void Opcode1nnn(Chip8* chip8) {
  chip8->program_counter = chip8->opcode & 0x0FFF;
}

// Вызывает подпрограмму
static void Opcode2nnn(Chip8* chip8) {
  if (chip8->stack_pointer <= 15) {
    chip8->stack[chip8->stack_pointer] = chip8->program_counter + 2;
    chip8->program_counter = chip8->opcode & 0x0FFF;
  }
  if (chip8->stack_pointer < 15) chip8->stack_pointer++;
}

// Пропускает следующую инструкцию, если Vx == kk
static void Opcode3xkk(Chip8* chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t kk = (chip8->opcode & 0x00FF);
  chip8->program_counter += (x == kk) ? 4 : 2;
}

// Пропускает следующую инструкцию, если Vx != kk
static void Opcode4xkk(Chip8* chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t kk = (chip8->opcode & 0x00FF);
  chip8->program_counter += (x != kk) ? 4 : 2;
}

// Пропускает следующую инструкцию, если Vx == Vy
static void Opcode5xy0(Chip8* chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->program_counter += (x == y) ? 4 : 2;
}

// Устанавливает Vx = kk
static void Opcode6xkk(Chip8* chip8) {
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = chip8->opcode & 0x00FF;
  chip8->program_counter += 2;
}

// Устанавливает Vx = Vx + kk
static void Opcode7xkk(Chip8* chip8) {
  chip8->v[(chip8->opcode & 0x0F00) >> 8] += chip8->opcode & 0x00FF;
  chip8->program_counter += 2;
}

// Устанавливает Vx = Vy
static void Opcode8xy0(Chip8* chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = y;
  chip8->program_counter += 2;
}

// Устанавливает Vx = Vx | Vy и VF = 0
static void Opcode8xy1(Chip8* chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] |= y;
  chip8->v[0xF] = 0;
  chip8->program_counter += 2;
}

// Устанавливает Vx = Vx & Vy и VF = 0
static void Opcode8xy2(Chip8* chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] &= y;
  chip8->v[0xF] = 0;
  chip8->program_counter += 2;
}

// Устанавливает Vx = Vx ^ Vy и VF = 0
static void Opcode8xy3(Chip8* chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] ^= y;
  chip8->v[0xF] = 0;
  chip8->program_counter += 2;
}

// Устанавливает Vx = Vx + Vy и VF = 1, если Vx + Vy > 0xFF, иначе VF = 0
static void Opcode8xy4(Chip8* chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  uint8_t rem = 0xFF - chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] += y;
  chip8->v[0xF] = (y > rem) ? 1 : 0;
  chip8->program_counter += 2;
}

// Устанавливает Vx = Vx - Vy и VF = 1, если Vx >= Vy, иначе VF = 0
static void Opcode8xy5(Chip8* chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] -= y;
  chip8->v[0xF] = (x >= y) ? 1 : 0;
  chip8->program_counter += 2;
}

// Устанавливает Vx = Vy >> 1 и VF = 1, если Vy & 0x01 == 0x01, иначе VF = 0
static void Opcode8xy6(Chip8* chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = y >> 1;
  chip8->v[0xF] = ((y & 0x01) == 0x01) ? 1 : 0;
  chip8->program_counter += 2;
}

// Устанавливает Vx = Vy - Vx и VF = 1, если Vy >= Vx, иначе VF = 0
static void Opcode8xy7(Chip8* chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = y - x;
  chip8->v[0xF] = (y >= x) ? 1 : 0;
  chip8->program_counter += 2;
}

// Устанавливает Vx = Vy << 1 и VF = 1, если Vy & 0x80 == 0x80, иначе VF = 0
static void Opcode8xyE(Chip8* chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = y << 1;
  chip8->v[0xF] = ((y & 0x80) == 0x80) ? 1 : 0;
  chip8->program_counter += 2;
}

// Пропускает следующую инструкцию, если Vx != Vy
static void Opcode9xy0(Chip8* chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->program_counter += (x != y) ? 4 : 2;
}

// Устанавливает i = nnn
static void OpcodeAnnn(Chip8* chip8) {
  chip8->i = chip8->opcode & 0x0FFF;
  chip8->program_counter += 2;
}

// Устанавливает program_counter = nnn + V0
static void OpcodeBnnn(Chip8* chip8) {
  chip8->program_counter = (chip8->opcode & 0xFFF) + chip8->v[0x0];
}

/*
Устанавливает Vx = random_num & kk.
random_num находиться в диапазоне [0, 255]
*/
static void OpcodeCxkk(Chip8* chip8) {
  uint8_t random_num = rand() % (0xFF + 1);
  uint8_t kk = chip8->opcode & 0x00FF;
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = random_num & kk;
  chip8->program_counter += 2;
}

// Включает и выключает пиксели экрана. При выключении пикселя VF = 1, иначе 0
static void OpcodeDxyn(Chip8* chip8) {
  if (chip8->vblank == false) return;

  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8] % kDisplayWidth;
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4] % kDisplayHeight;
  int height = chip8->opcode & 0x000F;  // n
  int width = 8;
  int pixel;

  /*
  Пиксели включаются и выключаются в координтах (Vx, Vy)
  шириной 8 пикселей и высотой n пикселей.
  */

  chip8->v[0xF] = 0;

  // Считывает n байтов из памяти, начиная с i
  for (int yline = 0; yline < height; ++yline) {
    pixel = chip8->memory[chip8->i + yline];

    for (int xline = 0; xline < width; ++xline) {
      int sum_x = x + xline;
      int sum_y = y + yline;

      // Не рисует координаты, вышедшие за пределы экрана
      if (((pixel & (0x80 >> xline)) != 0) && (sum_x < kDisplayWidth) &&
          (sum_y < kDisplayHeight)) {
        int idx_display = sum_x + (sum_y * kDisplayWidth);

        if (chip8->display[idx_display] == kPixelOn) {
          chip8->display[idx_display] = kPixelOff;
          chip8->v[0xF] = 1;
        } else {
          chip8->display[idx_display] = kPixelOn;
        }
      }
    }
  }

  chip8->program_counter += 2;
  chip8->update = true;
  chip8->vblank = false;
}

// Пропускает следующую инструкцию, если нажата клавиша со значение Vx
static void OpcodeEx9E(Chip8* chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8] & 0xF;
  chip8->program_counter += (chip8->keyboard[x] == 1) ? 4 : 2;
}

// Пропускает следующую инструкцию, если клавиша со значение Vx не нажата
static void OpcodeExA1(Chip8* chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8] & 0xF;
  chip8->program_counter += (chip8->keyboard[x] == 0) ? 4 : 2;
}

// Устанавливает Vx = delay_timer
static void OpcodeFx07(Chip8* chip8) {
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = chip8->delay_timer;
  chip8->program_counter += 2;
}

// Ожидает нажатие и отпускание клавиши и сохраняте её в Vx
static void OpcodeFx0A(Chip8* chip8) {
  if (chip8->pressed_key == -1) {
    int k = 0;
    // Находим и запоминаем нажатую клавишу
    for (; (k < kKeyboardSize - 1) && (chip8->keyboard[k] == 0); ++k);
    if (chip8->keyboard[k] == 1) chip8->pressed_key = k;
  } else {
    // Если клавишу отпустили, записываем в Vx
    if (chip8->keyboard[chip8->pressed_key] == 0) {
      chip8->v[(chip8->opcode & 0x0F00) >> 8] = chip8->pressed_key;
      chip8->pressed_key = -1;
      chip8->program_counter += 2;
    }
  }
}

// Устанавливает delay_timer = Vx
static void OpcodeFx15(Chip8* chip8) {
  chip8->delay_timer = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->program_counter += 2;
}

// Устанавливает sound_timer = Vx
static void OpcodeFx18(Chip8* chip8) {
  chip8->sound_timer = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->program_counter += 2;
}

// Устанавливает i = i + Vx
static void OpcodeFx1E(Chip8* chip8) {
  chip8->i += chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->program_counter += 2;
}

// Устанавливает i местоположение шрифта, соответствующего Vx
static void OpcodeFx29(Chip8* chip8) {
  chip8->i = kStartFonts + 5 * (chip8->v[(chip8->opcode & 0x0F00) >> 8] & 0xF);
  chip8->program_counter += 2;
}
// Устанавливает ячейкам памяти с адресами i, i+1, i+2 значения разрядов Vx
static void OpcodeFx33(Chip8* chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->memory[chip8->i] = x / 100;
  chip8->memory[chip8->i + 1] = (x % 100) / 10;
  chip8->memory[chip8->i + 2] = x % 10;
  chip8->program_counter += 2;
}

// Копирует значения V0-Vx в память, начиная с i
static void OpcodeFx55(Chip8* chip8) {
  int idx_x = (chip8->opcode & 0x0F00) >> 8;
  for (int k = 0x0; k < idx_x + 1; ++k, ++(chip8->i))
    chip8->memory[chip8->i] = chip8->v[k];
  chip8->program_counter += 2;
}

// Копирует значения в V0-Vx из памяти, начиная с i
static void OpcodeFx65(Chip8* chip8) {
  uint8_t idx_x = (chip8->opcode & 0x0F00) >> 8;
  for (uint8_t k = 0x0; k < idx_x + 1; ++k, ++(chip8->i))
    chip8->v[k] = chip8->memory[chip8->i];
  chip8->program_counter += 2;
}

static void UnknownOpcode(Chip8* chip8) {
  printf("Unknown opcode [0x%X]\n", chip8->opcode);
  chip8->program_counter += 2;
}

void EmulateCycle(Chip8* chip8) {
  uint16_t part_one = chip8->memory[chip8->program_counter] << 8;
  uint16_t part_two = chip8->memory[chip8->program_counter + 1];
  chip8->opcode = part_one | part_two;
  switch (chip8->opcode & 0xF000) {
    case 0x0000:
      switch (chip8->opcode & 0x0FFF) {
        case 0x00E0:
          Opcode00E0(chip8);
          break;
        case 0x00EE:
          Opcode00EE(chip8);
          break;
        default:
          UnknownOpcode(chip8);
      }
      break;
    case 0x1000:
      Opcode1nnn(chip8);
      break;
    case 0x2000:
      Opcode2nnn(chip8);
      break;
    case 0x3000:
      Opcode3xkk(chip8);
      break;
    case 0x4000:
      Opcode4xkk(chip8);
      break;
    case 0x5000:
      if ((chip8->opcode & 0x000F) == 0x0000) Opcode5xy0(chip8);
      break;
    case 0x6000:
      Opcode6xkk(chip8);
      break;
    case 0x7000:
      Opcode7xkk(chip8);
      break;
    case 0x8000:
      switch (chip8->opcode & 0x000F) {
        case 0x0000:
          Opcode8xy0(chip8);
          break;
        case 0x0001:
          Opcode8xy1(chip8);
          break;
        case 0x0002:
          Opcode8xy2(chip8);
          break;
        case 0x0003:
          Opcode8xy3(chip8);
          break;
        case 0x0004:
          Opcode8xy4(chip8);
          break;
        case 0x0005:
          Opcode8xy5(chip8);
          break;
        case 0x0006:
          Opcode8xy6(chip8);
          break;
        case 0x0007:
          Opcode8xy7(chip8);
          break;
        case 0x000E:
          Opcode8xyE(chip8);
          break;
        default:
          UnknownOpcode(chip8);
      }
      break;
    case 0x9000:
      if ((chip8->opcode & 0x000F) == 0x0000) Opcode9xy0(chip8);
      break;
    case 0xA000:
      OpcodeAnnn(chip8);
      break;
    case 0xB000:
      OpcodeBnnn(chip8);
      break;
    case 0xC000:
      OpcodeCxkk(chip8);
      break;
    case 0xD000:
      OpcodeDxyn(chip8);
      break;
    case 0xE000:
      switch (chip8->opcode & 0x00FF) {
        case 0x009E:
          OpcodeEx9E(chip8);
          break;
        case 0x00A1:
          OpcodeExA1(chip8);
          break;
        default:
          UnknownOpcode(chip8);
      }
      break;
    case 0xF000:
      switch (chip8->opcode & 0x00FF) {
        case 0x0007:
          OpcodeFx07(chip8);
          break;
        case 0x000A:
          OpcodeFx0A(chip8);
          break;
        case 0x0015:
          OpcodeFx15(chip8);
          break;
        case 0x0018:
          OpcodeFx18(chip8);
          break;
        case 0x001E:
          OpcodeFx1E(chip8);
          break;
        case 0x0029:
          OpcodeFx29(chip8);
          break;
        case 0x0033:
          OpcodeFx33(chip8);
          break;
        case 0x0055:
          OpcodeFx55(chip8);
          break;
        case 0x0065:
          OpcodeFx65(chip8);
          break;
        default:
          UnknownOpcode(chip8);
      }
      break;
    default:
      UnknownOpcode(chip8);
  }
  return;
}
