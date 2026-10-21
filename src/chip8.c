#include "chip8.h"

static void ClearDisplay(uint32_t *display) {
  for (uint16_t i = 0; i < DISPLAY_SIZE; ++i)
    display[i] = PIXEL_OFF;
  return;
}

bool InitChip8(Chip8 *chip8) {
  // cpu
  chip8->v = calloc(V_SIZE, sizeof(uint8_t));
  if (!chip8->v)
    return false;
  chip8->dt = chip8->st = 0;
  chip8->i = chip8->opcode = 0x0000;
  chip8->pc = 0x200;
  // memory
  chip8->memory = calloc(MEM_SIZE, sizeof(uint8_t));
  if (!chip8->memory)
    return false;
  // stack
  chip8->stack = calloc(STACK_SIZE, sizeof(uint16_t));
  if (!chip8->stack)
    return false;
  chip8->sp = 0;
  // display
  chip8->display = calloc(DISPLAY_SIZE, sizeof(uint32_t));
  if (!chip8->display)
    return false;
  ClearDisplay(chip8->display);
  chip8->update = true;
  chip8->vblank = true;
  // keyboard
  chip8->keyboard = calloc(KEYBOARD_SIZE, sizeof(uint8_t));
  if (!chip8->keyboard)
    return false;
  chip8->pressed_key = -1;

  return true;
}

void FreeChip8(Chip8 *chip8) {
  // cpu
  if(chip8->v)
    free(chip8->v);
  // memory
  if(chip8->memory)
    free(chip8->memory);
  // stack
  if(chip8->stack)
    free(chip8->stack);
  // display
  if(chip8->display)
    free(chip8->display);
  // keyboard
  if(chip8->keyboard)
    free(chip8->keyboard);
  return;
}

bool WriteFontsToMemory(uint8_t *memory) {
  if (!memory)
    return false;
  const unsigned char kfonts[80] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };
  for (uint8_t i = 0x050; i < 0xA0; ++i)
    memory[i] = kfonts[i - 0x050];
  return true;
}

bool WriteRomsToMemory(uint8_t *memory, int argc, char *argv[]) {
  if(argc == 1 || argc > 3) {
    printf("Enter the name of one game!\n");
    return false;
  }

  bool res = true;
  char game_path[30] = "../roms/";
  strcat(game_path, argv[1]);
  strcat(game_path, ".ch8");
  int fd = open(game_path, O_RDONLY);
  if (fd == -1) {
    perror(game_path);
    res = false;
  }

  if(res) {
    struct stat st;
    if (fstat(fd, &st) == -1) {
      perror("stat error");
      res = false;
    } else {
       if (st.st_size <= (MEM_SIZE - RAM_SIZE)) {
         // 0x200 - 0xFFF for roms
         if(read(fd, memory + RAM_SIZE, st.st_size) == -1) {
           perror("read error");
	   res = false;
	 }
       } else {
         printf("big size roms");
	 res = false;
       }
       close(fd);
    }
  }
  return res;
}

static void Opcode_00E0(Chip8 *chip8) {
  ClearDisplay(chip8->display);
  chip8->update = true;
  chip8->pc += 2;
}

static void Opcode_000E(Chip8 *chip8) {
  if(chip8->sp > 0) chip8->sp--;
  chip8->pc = chip8->stack[chip8->sp];
}

static void Opcode_1nnn(Chip8 *chip8) { chip8->pc = chip8->opcode & 0x0FFF; }

static void Opcode_2nnn(Chip8 *chip8) {
  if(chip8->sp <= 15) {
    chip8->stack[chip8->sp] = chip8->pc + 2;
    chip8->pc = chip8->opcode & 0x0FFF;
  }
  if(chip8->sp < 15) 
    chip8->sp++;
}

static void Opcode_3xkk(Chip8 *chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t kk = (uint8_t)(chip8->opcode & 0x00FF);
  chip8->pc += (x == kk) ? 4 : 2;
}

static void Opcode_4xkk(Chip8 *chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t kk = (uint8_t)(chip8->opcode & 0x00FF);
  chip8->pc += (x != kk) ? 4 : 2;
}

static void Opcode_5xy0(Chip8 *chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->pc += (x == y) ? 4 : 2;
}

static void Opcode_6xkk(Chip8 *chip8) {
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = chip8->opcode & 0x00FF;
  chip8->pc += 2;
}

static void Opcode_7xkk(Chip8 *chip8) {
  chip8->v[(chip8->opcode & 0x0F00) >> 8] += chip8->opcode & 0x00FF;
  chip8->pc += 2;
}

static void Opcode_8xy0(Chip8 *chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = y;
  chip8->pc += 2;
}

static void Opcode_8xy1(Chip8 *chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] |= y;
  chip8->v[0xF] = 0;
  chip8->pc += 2;
}

static void Opcode_8xy2(Chip8 *chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] &= y;
  chip8->v[0xF] = 0;
  chip8->pc += 2;
}

static void Opcode_8xy3(Chip8 *chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] ^= y;
  chip8->v[0xF] = 0;
  chip8->pc += 2;
}

static void Opcode_8xy4(Chip8 *chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  uint8_t rem = 0xFF - chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] += y;
  chip8->v[0xF] = (y > rem) ? 1 : 0;
  chip8->pc += 2;
}

static void Opcode_8xy5(Chip8 *chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] -= y;
  chip8->v[0xF] = (x >= y) ? 1 : 0;
  chip8->pc += 2;
}

static void Opcode_8xy6(Chip8 *chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = y >> 1;
  chip8->v[0xF] = ((y & 0x01) == 0x01) ? 1 : 0;
  chip8->pc += 2;
}

static void Opcode_8xy7(Chip8 *chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = y - x;
  chip8->v[0xF] = (y >= x) ? 1 : 0;
  chip8->pc += 2;
}

static void Opcode_8xyE(Chip8 *chip8) {
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = y << 1;
  chip8->v[0xF] = ((y & 0x80) == 0x80) ? 1 : 0;
  chip8->pc += 2;
}

static void Opcode_9xy0(Chip8 *chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
  chip8->pc += (x != y) ? 4 : 2;
}

static void Opcode_Annn(Chip8 *chip8) {
  chip8->i = chip8->opcode & 0x0FFF;
  chip8->pc += 2;
}

static void Opcode_Bnnn(Chip8 *chip8) {
  chip8->pc = (chip8->opcode & 0xFFF) + (uint16_t)chip8->v[0x0];
}

static void Opcode_Cxkk(Chip8 *chip8) {
  uint8_t rndm = random() % 0xFF;
  uint8_t kk = chip8->opcode & 0x00FF;
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = rndm & kk;
  chip8->pc += 2;
}

static void Opcode_Dxyn(Chip8 *chip8) {
  if (chip8->vblank == false)
    return;
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8] % 64;
  uint8_t y = chip8->v[(chip8->opcode & 0x00F0) >> 4] % 32;
  uint8_t h = chip8->opcode & 0x000F;
  uint8_t pixel;

  chip8->v[0xF] = 0;
  for (uint8_t yline = 0; yline < h; ++yline) {
    pixel = chip8->memory[chip8->i + (uint16_t)yline];
    for (uint8_t xline = 0; xline < 8; ++xline) {
      uint8_t sum_x = x + xline;
      uint8_t sum_y = y + yline;
      if ((pixel & (0x80 >> xline)) != 0 && sum_x < 64 && sum_y < 32) {
        uint16_t idx_display = sum_x + (sum_y * 64);
        if (chip8->display[idx_display] == PIXEL_ON) {
          chip8->display[idx_display] = PIXEL_OFF;
          chip8->v[0xF] = 1;
        } else {
          chip8->display[idx_display] = PIXEL_ON;
        }
      }
    }
  }
  chip8->pc += 2;
  chip8->update = true;
  chip8->vblank = false;
}

static void Opcode_Ex9E(Chip8 *chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8] & 0xF;
  chip8->pc += (chip8->keyboard[x] == 1) ? 4 : 2;
}

static void Opcode_ExA1(Chip8 *chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8] & 0xF;
  chip8->pc += (chip8->keyboard[x] == 0) ? 4 : 2;
}

static void Opcode_Fx07(Chip8 *chip8) {
  chip8->v[(chip8->opcode & 0x0F00) >> 8] = chip8->dt;
  chip8->pc += 2;
}

static void Opcode_Fx0A(Chip8 *chip8) {
  if (chip8->pressed_key == -1) {
    uint8_t k = 0;
    for (; (k < KEYBOARD_SIZE-1) && (chip8->keyboard[k] == 0); ++k)
      ;
    if (chip8->keyboard[k] == 1)
      chip8->pressed_key = k;
  } else {
    if (chip8->keyboard[chip8->pressed_key] == 0) {
      chip8->v[(chip8->opcode & 0x0F00) >> 8] = chip8->pressed_key;
      chip8->pressed_key = -1;
      chip8->pc += 2;
    }
  }
}

static void Opcode_Fx15(Chip8 *chip8) {
  chip8->dt = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->pc += 2;
}

static void Opcode_Fx18(Chip8 *chip8) {
  chip8->st = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->pc += 2;
}

static void Opcode_Fx1E(Chip8 *chip8) {
  chip8->i += (uint16_t)chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->pc += 2;
}

static void Opcode_Fx29(Chip8 *chip8) {
  chip8->i = 0x050 + 5 * (uint16_t)chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->pc += 2;
}

static void Opcode_Fx33(Chip8 *chip8) {
  uint8_t x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
  chip8->memory[chip8->i] = x / 100;
  chip8->memory[chip8->i + 1] = (x % 100) / 10;
  chip8->memory[chip8->i + 2] = x % 10;
  chip8->pc += 2;
}

static void Opcode_Fx55(Chip8 *chip8) {
  uint8_t idx_x = (chip8->opcode & 0x0F00) >> 8;
  for (uint8_t k = 0x0; k < idx_x + 1; ++k, ++(chip8->i))
    chip8->memory[chip8->i] = chip8->v[k];
  chip8->pc += 2;
}

static void Opcode_Fx65(Chip8 *chip8) {
  uint8_t idx_x = (chip8->opcode & 0x0F00) >> 8;
  for (uint8_t k = 0x0; k < idx_x + 1; ++k, ++(chip8->i))
    chip8->v[k] = chip8->memory[chip8->i];
  chip8->pc += 2;
}

static void UnknownOpcode(Chip8 *chip8) {
  printf("Unknown opcode [0x0000]: 0x%X\n", chip8->opcode);
  chip8->pc += 2;
}

void EmulateCycle(Chip8 *chip8) {
  uint16_t part_one = chip8->memory[chip8->pc] << 8;
  uint16_t part_two = chip8->memory[chip8->pc + 1];
  chip8->opcode = part_one | part_two;
  switch (chip8->opcode & 0xF000) {
  case 0x0000:
    switch (chip8->opcode & 0x000F) {
    case 0x0000: // 00E0 - clear dislplay
      Opcode_00E0(chip8);
      break;
    case 0x000E: // 000E - RET
      Opcode_000E(chip8);
      break;
    default:
      UnknownOpcode(chip8);
    };
    break;
  case 0x1000: // 1NNN - jump nnn
    Opcode_1nnn(chip8);
    break;
  case 0x2000: // 2NNN - call addr
    Opcode_2nnn(chip8);
    break;
  case 0x3000: // 3xkk - SE Vx, byte
    Opcode_3xkk(chip8);
    break;
  case 0x4000: // 4xkk - SNE Vx, byte
    Opcode_4xkk(chip8);
    break;
  case 0x5000: // 5xy0 - SE Vx, Vy
    Opcode_5xy0(chip8);
    break;
  case 0x6000: // LD Vx
    Opcode_6xkk(chip8);
    break;
  case 0x7000: // ADD Vx
    Opcode_7xkk(chip8);
    break;
  case 0x8000:
    switch (chip8->opcode & 0x000F) {
    case 0x0000: // 8xy2 LD Vx, Vy
      Opcode_8xy0(chip8);
      break;
    case 0x0001: // 8xy2 OR Vx, Vy
      Opcode_8xy1(chip8);
      break;
    case 0x0002: // 8xy2 AND Vx, Vy
      Opcode_8xy2(chip8);
      break;
    case 0x0003: // 8xy3 XOR Vx, Vy
      Opcode_8xy3(chip8);
      break;
    case 0x0004: // 8xy4 ADD Vx, Vy
      Opcode_8xy4(chip8);
      break;
    case 0x0005: // 8xy5 SUB Vx, Vy
      Opcode_8xy5(chip8);
      break;
    case 0x0006: // 8xy6 SHR Vx {, Vy}
      Opcode_8xy6(chip8);
      break;
    case 0x0007: // 8xy7 SUBN Vx, Vy
      Opcode_8xy7(chip8);
      break;
    case 0x000E: // 8xyE SHL Vx {, Vy}
      Opcode_8xyE(chip8);
      break;
    default:
      UnknownOpcode(chip8);
    }
    break;
  case 0x9000: // 9xy0 SNE Vx, Vy
    Opcode_9xy0(chip8);
    break;
  case 0xA000: // Annn LD I
    Opcode_Annn(chip8);
    break;
  case 0xB000: // Bnnn JP V0, addr
    Opcode_Bnnn(chip8);
    break;
  case 0xC000: // Cxkk RND Vx, byte
    Opcode_Cxkk(chip8);
    break;
  case 0xD000:
    Opcode_Dxyn(chip8);
    break;
  case 0xE000: // Ex9E ExA1
    switch (chip8->opcode & 0x00FF) {
    case 0x009E:
      Opcode_Ex9E(chip8);
      break;
    case 0x00A1:
      Opcode_ExA1(chip8);
      break;
    default:
      UnknownOpcode(chip8);
    }
    break;
  case 0xF000:
    switch (chip8->opcode & 0x00FF) {
    case 0x0007: // Fx07 LD Vx, DT
      Opcode_Fx07(chip8);
      break;
    case 0x000A: // Fx0A
      Opcode_Fx0A(chip8);
      break;
    case 0x0015: // Fx15 LD DT, Vx
      Opcode_Fx15(chip8);
      break;
    case 0x0018: // Fx18 LD ST, Vx
      Opcode_Fx18(chip8);
      break;
    case 0x001E: // Fx1E ADD I, Vx
      Opcode_Fx1E(chip8);
      break;
    case 0x0029: // Fx29
      Opcode_Fx29(chip8);
      break;
    case 0x0033: // Fx33 LD BSD, Vx
      Opcode_Fx33(chip8);
      break;
    case 0x0055: // Fx55 LD [I], Vx
      Opcode_Fx55(chip8);
      break;
    case 0x0065: // Fx65 LD Vx, [I]
      Opcode_Fx65(chip8);
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
