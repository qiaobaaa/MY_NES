#include <Arduino.h>
#include <U8g2lib.h>
#include <SD_MMC.h>
#include <bc_key_scan.h>
#include <vector>
#include <string>

#include "chip8.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
//SPIClass spi2(HSPI);
// 根据你的显示屏类型选择构造函数（此处以I2C的SSD1306 128x64为例）
//U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/4, /* data=*/5, /* cs=*/17, /* dc=*/16, /* reset=*/6);
//U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 17, /* dc=*/ 16, /* reset=*/ 6); 
//U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 17, /* dc=*/ 16, /* reset=*/ 6);

U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, 17, 16, 6);
//uint8_t pressed_key = 0;
int8_t pressed_key = -1;
const uint32_t TARGET_FPS = 600;
const uint32_t FRAME_INTERVAL_US = 1000000 / TARGET_FPS; // ~16666us
const uint32_t CHIP8_INSTR_PER_SEC = 600;  // CHIP-8标准：600指令/秒2062
//const uint32_t INSTR_INTERVAL_US = 1000000 / CHIP8_INSTR_PER_SEC; // 单指令间隔≈1667μs
const uint32_t INSTR_INTERVAL_US = 1667;
uint32_t last_render_us = 0;
//int last_render_us = 0;

#define SDMMC_D2 13   // SDMMC Data2
#define SDMMC_D3 14   //34 // SDMMC Data3 / SPI CS
#define SDMMC_CMD 21  // SDMMC CMD / SPI MOSI
#define SDMMC_CLK 47  // SDMMC CLK / SPI SCK
#define SDMMC_D0 42   // SDMMC Data0 / SPI MISO 20
#define SDMMC_D1 45   // SDMMC Data1
#include "HardwareSerial.h"
HardwareSerial swSerial(1);
BcKeyScan Keypad(swSerial);

// 按键定义（根据实际硬件连接修改）
/*
const int KEY_UP = 7;
const int KEY_DOWN = 8;
const int KEY_LEFT = 9;
const int KEY_RIGHT = 10;
const int KEY_ENTER = 11;
const int KEY_ESC = 12;
*/

// 文件列表及导航变量
std::vector<String> fileList;  // 存储所有文件/目录路径
int currentCursor = 0;         // 当前光标位置（全局索引）
int itemsPerPage = 3;          // 每页显示数量
int currentPage = 0;           // 当前页码

// 文件操作状态
bool isFileMode = false;  // 是否进入文件操作模式
String selectedFile;      // 当前选中的文件

// 按键状态检测
bool keyUpState = HIGH;
bool keyDownState = HIGH;
bool keyLeftState = HIGH;
bool keyRightState = HIGH;
bool keyEnterState = HIGH;
bool keyEscState = HIGH;

unsigned char key[KEY_SIZE];
unsigned char chip8_fontset[80] = {
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

chip8_t chip8_core;

void chip_get_key() {
  //Keypad.setDetectMode(1);
  Keypad.checkChanges();                   // let the key scan library to update key status
  if (Keypad.isKeyChanged() == true) {     // if there is any detectable key change
    int num = Keypad.getKeyValue();
    Serial.println(num);  // print key value on Serial (use Serial Monitor to see it!)
    //Serial.println(666555);
    switch (num) {
      case 25:
        key[0] = 1;
        break;
      case 153:
        key[0] = 0;
        break;
      case 26:
        key[1] = 1;
        break;
      case 154:
        key[1] = 0;
        break;
      case 27:
        key[2] = 1;
        break;
      case 155:
        key[2] = 0;
        break;
      case 19:
        key[3] = 1;
        break;
      case 147:
        key[3] = 0;
        break;
      case 20:
        key[4] = 1;
        break;
      case 148:
        key[4] = 0;
        break;
      case 21:
        key[5] = 1;
        break;
      case 149:
        key[5] = 0;
        break;
      case 13:
        key[6] = 1;
        break;
      case 14:
        key[7] = 1;
        break;
      case 15:
        key[8] = 1;
        break;
      case 7:
        key[9] = 1;
        break;
      case 8:
        key[10] = 1;
        break;
      case 9:
        key[11] = 1;
        break;
      case 1:
        key[12] = 1;
        break;
      case 2:
        key[13] = 1;
        break;
      case 3:
        key[14] = 1;
        break;
      case 11:
        key[15] = 1;
        break;
      default:
        break;
    }
  }
  //delay(1);
}

void draw_sprite(unsigned char x, unsigned char y, unsigned char n, chip8_t* chip8) {
  unsigned char row = y, col = x;
  unsigned char byte_index;
  unsigned char bit_index;
  //u8g2.clearBuffer();
  chip8->V[0xF] = 0;
  //printf("789456         %d\n", n);
  for (byte_index = 0; byte_index < n; byte_index++) {
    unsigned char byt = chip8->memory[chip8->I + byte_index];
    for (bit_index = 0; bit_index < 8; bit_index++) {
      unsigned char bit = (byt >> bit_index) & 0x1;
      unsigned char* pixel = &chip8->gfx[(row + byte_index) % 32][(col + (7 - bit_index)) % 64];
      if (bit == 1 && *pixel == 1) {
        chip8->V[0xF] = 1;
      }
      *pixel = *pixel ^ bit;
    }
  }
  /*
  printf("666666666\n");
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 64; j++) {
      if (chip8->gfx[i][j] == 1 && j != 63) {
        u8g2.drawPixel(j, i);
      } else if (chip8->gfx[i][j] == 1 && j == 63) {
        u8g2.drawPixel(j, i);
      } else if (chip8->gfx[i][j] == 0 && j != 63) {
        //printf(" ");
      } else if (chip8->gfx[i][j] == 0 && j == 63) {
        //printf(" \n");
      }
    }
  }
  u8g2.sendBuffer();
  */
}

void real_draw(chip8_t* chip8)
{
  /*
  u8g2.clearBuffer();
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 64; j++) {
      if (chip8->gfx[i][j] == 1 && j != 63) {
        u8g2.drawPixel(j, i);
      } else if (chip8->gfx[i][j] == 1 && j == 63) {
        u8g2.drawPixel(j, i);
      } else if (chip8->gfx[i][j] == 0 && j != 63) {
        //printf(" ");
      } else if (chip8->gfx[i][j] == 0 && j == 63) {
        //printf(" \n");
      }
    }
  }
  u8g2.sendBuffer();
  */
  /*
  uint32_t now = micros();
  if (now - last_render_us < FRAME_INTERVAL_US) {
    return; // 未到渲染时间，直接返回
  }
  */
   u8g2.clearBuffer();
    // 1. 执行CHIP-8指令（按原始速度，每轮10条）
    //Serial.println(now - last_render_us);
    //Serial.println(FRAME_INTERVAL_US);
    // 2. 仅在「需要绘制」且「达到帧率间隔」时渲染
    if (chip8_has_gfx_change(&chip8_core)) {
            chip8_render(&chip8_core);       // 绘制CHIP-8区域（其余黑色）
            chip8_sync_gfx_prev(&chip8_core);// 同步上一帧
        }
    //    
    if (chip8_core.draw /*&& (now - last_render_us >= FRAME_INTERVAL_US)*/) {
        // 3. 仅当画面有变化时才渲染（避免无意义刷屏）
        //Serial.println(999999);
        if (chip8_has_gfx_change(&chip8_core)) {
            chip8_render(&chip8_core);       // 绘制CHIP-8区域（其余黑色）
            chip8_sync_gfx_prev(&chip8_core);// 同步上一帧
        }
        //chip8_core.draw = false;       // 重置绘制标志
        //last_render_us = now;     // 更新渲染时间
    }
  //  
    showFPS(u8g2); 
    u8g2.sendBuffer();
   // last_render_us = now;
}

// 对比前后帧：是否有像素变化（仅返回bool，不找区域）
bool chip8_has_gfx_change(chip8_t* chip8) {
  //Serial.println(666666);
    return memcmp(chip8->gfx, chip8->gfx_prev, sizeof(chip8->gfx)) != 0;
}

// 同步上一帧缓存
void chip8_sync_gfx_prev(chip8_t* chip8) {
    memcpy(chip8->gfx_prev, chip8->gfx, sizeof(chip8->gfx));
}

void chip8_render(chip8_t* chip8) {
    // 1. 清空U8G2缓冲区（全黑）
    //u8g2.clearBuffer();
    
    // 2. 仅绘制CHIP-8的64x32区域（其余区域自然是黑色）
    u8g2.setDrawColor(1); // 白色绘制
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            if (chip8->gfx[y][x] == 1) {
                u8g2.drawPixel(x, y); // 仅绘制亮的像素
            }
        }
    }
    //showFPS(u8g2);
    // 3. 发送全帧到OLED（U8G2自带函数，无需自定义）
    //u8g2.sendBuffer();
}

void showFPS(U8G2 &u8g2) {
  // 静态变量：仅在函数内部保持状态
  static uint32_t lastMicros = 0;  // 上一次统计的时间点
  static uint32_t frameCnt = 0;    // 本秒已渲染的帧数
  static uint16_t fps = 0;         // 最近一次计算得到的 FPS

  // ① 计数当前帧
  frameCnt++;

  // ② 检查是否已经过了 1 秒（1 000 000 µs）
  uint32_t now = micros();
  uint32_t elapsed = now - lastMicros;  // 可能会在 2^32 微秒后回绕，仍然安全
  if (elapsed >= 1000000UL)             // 达到或超过 1 s
  {
    // ③ 计算 FPS（取整数，足够显示）
    fps = (uint16_t)((float)frameCnt * 1000000.0f / (float)elapsed + 0.5f);

    // ④ 复位统计
    frameCnt = 0;
    lastMicros = now;
  }

  // ⑤ 在 OLED 上把 FPS 绘制出来（示例放在左上角）
  //    使用小字号避免覆盖太多画面。这里用 6×10 的字体。
  u8g2.setFont(u8g2_font_6x10_tr);
  char buf[12];
  snprintf(buf, sizeof(buf), "FPS:%3d", fps);
  u8g2.setCursor(0, 50);  // x=0, y=10 (基线位置)
  u8g2.print(buf);
}

void real_draww(chip8_t* chip8) {
  //u8g2.clearBuffer();
  
  /*
uint32_t now = micros();
  if (now - last_render_us < FRAME_INTERVAL_US) {
    return; // 未到渲染时间，直接返回
  }
  */
  u8g2.clearBuffer();
  
  // 临时缓冲区：存储每行 64 像素的位图数据（8 字节 = 64 位）
  uint8_t row_buffer[8];

  for (int i = 0; i < 32; i++) {  // 遍历 Chip8 32 行
    // 初始化行缓冲区为 0
    memset(row_buffer, 0, sizeof(row_buffer));

    // 将当前行的 64 像素打包为 8 字节（每行 8 字节×8 位=64 像素）
    for (int j = 0; j < 64; j++) {
      if (chip8->gfx[i][j]) {
        // 计算像素对应字节索引和位索引
        int byte_idx = j / 8;                    // 0~7（每行8个字节）
        int bit_idx = 7 - (j % 8);               // 位图字节的位序（U8g2 位图是高位在前）
        row_buffer[byte_idx] |= (1 << bit_idx);  // 置位
      }
    }

    // 批量绘制当前行：drawBitmap(x, y, 宽度(字节), 高度, 位图数据)
    // Chip8 分辨率 64×32，绘制到 SSD1309 128×64 屏幕可居中（x=32, y=i*2 放大）
    // 若无需放大：x=0, y=i，宽度=8（字节），高度=1
    u8g2.drawBitmap(32, i, 8, 1, row_buffer);
  }
  
  /*
   //u8g2.drawBox(0, 0, 64, 32); // 用黑色填充CHIP-8区域（替代全清）
  // 2. 绘制CHIP-8的像素矩阵（仅遍历64x32像素）
  for (int y=0; y<32; y++) {
    for (int x=0; x<64; x++) {
      if (chip8->gfx[y][x]) { // CHIP-8的像素缓存
        u8g2.drawPixel(x, y); // 只绘制亮的像素，不碰暗的
      }
    }
  }
  */
    showFPS(u8g2); 
    
  u8g2.sendBuffer();
  //last_render_us = now;
}


void chip8_init(chip8_t* chip8_core) {
  chip8_core->pc = 0x200;
  chip8_core->opcode = 0;
  chip8_core->I = 0;
  chip8_core->sp = 0;
  chip8_core->draw = false;
  memset(chip8_core->gfx, 0, sizeof(unsigned char) * 2048);
  memcpy(chip8_core->gfx_prev, chip8_core->gfx, sizeof(chip8_core->gfx));
  memset(chip8_core->V, 0, sizeof(unsigned char) * 16);
  memset(chip8_core->stack, 0, sizeof(unsigned short) * 16);
  memset(chip8_core->key, 0, sizeof(unsigned char) * 16);
  memset(chip8_core->memory, 0, sizeof(unsigned char) * 4096);
  for (int i = 0; i < 80; i++) {
    chip8_core->memory[i] = chip8_fontset[i];
  }
}

unsigned char randbyte() {
  srand((unsigned)time(NULL));
  unsigned char a = rand() % 256;
  //printf("a: %d\n", a);
  return a;
}

void chip8_tick(chip8_t* chip8_core) {
  // update timers
  if (chip8_core->delay_timer > 0) {
    --chip8_core->delay_timer;
  }
  if (chip8_core->sound_timer > 0) {
    --chip8_core->sound_timer;
    if (chip8_core->sound_timer == 0) {
      //printf("BEEP!\n");
    }
  }
}

void emulate_cycle(chip8_t* chip8) {
  /*
if (chip8->pc>4096)
{
return;
}
*/
  int i;
  unsigned char x, y, n;
  unsigned char kk;
  unsigned short nnn;
  //printf("....... PC: 0x%04x %d \n", chip8->pc, chip8->pc);
  unsigned short opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc + 1];
  x = (opcode >> 8) & 0x000F;  // the lower 4 bits of the high byte
  y = (opcode >> 4) & 0x000F;  // the upper 4 bits of the low byte
  n = opcode & 0x000F;         // the lowest 4 bits
  kk = opcode & 0x00FF;        // the lowest 8 bits
  nnn = opcode & 0x0FFF;       // the lowest 12 bits
  //printf("%#X %#X %#X\n", chip8->pc, opcode, opcode & 0xF000);
  //chip8->pc += 2;
  //printf("PC: 0x%04x %d Op: 0x%04x\n", chip8->pc, chip8->pc, opcode);
//delay(1);
  switch (opcode & 0xF000) {
    case 0x0000:
      switch (kk) {
        case 0x00E0:
          memset(chip8->gfx, 0, sizeof(unsigned char) * 2048);
          chip8->draw = true;
          //chip8->pc += 2;
          chip8->pc = chip8->pc + 2;
          //printf("0x00E0 PC: 0x%04x %d Op: 0x%04x\n", chip8->pc, chip8->pc, opcode);
          break;
        case 0x00EE:
          chip8->pc = chip8->stack[--chip8->sp];
          //printf("0x00EE PC: 0x%04x %d Op: 0x%04x\n", chip8->pc, chip8->pc, opcode);
          break;
        default:
          //printf("unknown opcode %#X\n", opcode);
          break;
      }
      break;
    case 0x1000:  //1nnn: jump to address nnn
      chip8->pc = nnn;
      //printf("1nnn PC: 0x%04x %d \n", chip8->pc, chip8->pc);
      break;
    case 0x2000:  //2nnn: call address nnn
      chip8->stack[chip8->sp++] = chip8->pc + 2;
      chip8->pc = nnn;
      break;
    case 0x3000:  //3xkk: skip next instr if V[x] = kk
      chip8->pc += (chip8->V[x] == kk) ? 4 : 2;
      break;
    case 0x4000:  //4xkk: skip next instr if V[x] != kk
      chip8->pc += (chip8->V[x] != kk) ? 4 : 2;
      break;
    case 0x5000:  //5xy0: skip next instr if V[x] == V[y]
      chip8->pc += (chip8->V[x] == chip8->V[y]) ? 4 : 2;
      break;
    case 0x6000:  // 6xkk: set V[x] = kk
      chip8->V[x] = kk;
      chip8->pc += 2;
      //printf("6xkk PC: 0x%04x %d \n", chip8->pc, chip8->pc);
      break;
    case 0x7000:  // 7xkk: set V[x] = V[x] + kk
      chip8->V[x] += kk;
      chip8->pc += 2;
      //printf("7xkk PC: 0x%04x %d \n", chip8->pc, chip8->pc);
      break;
    case 0x8000:
      switch (n) {
        case 0x0:
          chip8->V[x] = chip8->V[y];
          break;
        case 0x1:
          chip8->V[x] = chip8->V[x] | chip8->V[y];
          break;
        case 0x2:
          chip8->V[x] = chip8->V[x] & chip8->V[y];
          break;
        case 0x3:
          chip8->V[x] = chip8->V[x] ^ chip8->V[y];
          break;
        case 0x4:
          chip8->V[0xF] = ((int)chip8->V[x] + (int)chip8->V[y]) > 255 ? 1 : 0;
          chip8->V[x] = chip8->V[x] + chip8->V[y];
          break;
        case 0x5:
          chip8->V[0xF] = (chip8->V[x] > chip8->V[y]) ? 1 : 0;
          chip8->V[x] = chip8->V[x] - chip8->V[y];
          break;
        case 0x6:
          chip8->V[0xF] = chip8->V[x] & 0x1;
          chip8->V[x] = chip8->V[x] >> 1;
        case 0x7:
          chip8->V[0xF] = (chip8->V[y] > chip8->V[x]) ? 1 : 0;
          chip8->V[x] = chip8->V[y] - chip8->V[x];
          break;
        case 0xE:
          chip8->V[0xF] = (chip8->V[x] >> 7) & 0x1;
          chip8->V[x] = chip8->V[x] << 1;
          break;
        default:
          //printf("0x8000 unknown opcode %#X\n", opcode);
          break;
      }
      chip8->pc += 2;
      break;
    case 0x9000:
      switch (n) {
        case 0x0:
          chip8->pc += (chip8->V[x] != chip8->V[y]) ? 4 : 2;
          break;
        default:
          //printf("0x9000 unknown opcode %#X\n", opcode);
          break;
      }
      break;
    case 0xA000:  // Annn: set I to address nnn
      chip8->I = nnn;
      chip8->pc += 2;
      //printf("Annn PC: 0x%04x %d \n", chip8->pc, chip8->pc);
      break;
    case 0xB000:
      chip8->pc = nnn + chip8->V[0];
      break;
    case 0xC000:
      chip8->V[x] = randbyte() & kk;
      chip8->pc += 2;
      break;
    case 0xD000:
      //printf("Draw sprite at (V[0x%x], V[0x%x]) = (0x%x, 0x%x) of height %d\n", x, y, chip8->V[x], chip8->V[y], n);
      draw_sprite(chip8->V[x], chip8->V[y], n, chip8);
      //printf("_____\n");
      chip8->pc += 2;
      chip8->draw = true;
      break;
    case 0xE000:  // key-pressed events
      switch (kk) {
        case 0x9E:
          //Serial.println(555555);
          //chip8->pc += (key[chip8->V[x]]) ? 4 : 2;
          chip8->pc += (chip8->V[x]==pressed_key) ? 4 : 2;
          break;
        case 0xA1:
          //Serial.println(777777);
          //Serial.println(chip8->V[x],HEX);
          //chip8->pc += (!key[chip8->V[x]]) ? 4 : 2;
          chip8->pc += (chip8->V[x]!=pressed_key) ? 4 : 2;
          break;
        default:
          //printf("0xE000 unknown opcode %#X\n", opcode);
          break;
      }
      break;
    case 0xF000:
      switch (kk) {
        case 0x07:
          chip8->V[x] = chip8->delay_timer;
          chip8->pc += 2;
          break;
        case 0x0A:  //Get Key
          //Serial.println(666666);
          if(pressed_key == -1)
          {
            chip8->pc -= 2;
            return;
          }
          chip8->V[x] = pressed_key;
          chip8->pc += 2; // 指令执行完成，PC+2
          break;
          /*
          while (true) {
            //Serial.println(pressed_key);
            //
            for (int i = 0; i < KEY_SIZE; i++) {
              //delay(1000);
              if (key[i]) {
                chip8->V[x] = i;
                goto pressed;
              }
            }
            //
            // 仅当映射到有效CHIP-8键值时，解除阻塞
            if (pressed_key != 0) {
                chip8->V[x] = pressed_key; // 核心：赋值到VX寄存器
                goto pressed;
                break; // 跳出阻塞循环
            }
        }

        // 微小延时，避免CPU空转
        //delayMicroseconds(100);

pressed:
    chip8->pc += 2; // 指令执行完成，PC+2
    break;
    */
        case 0x15:
          chip8->delay_timer = chip8->V[x];
          chip8->pc += 2;
          break;
        case 0x18:
          chip8->sound_timer = chip8->V[x];
          chip8->pc += 2;
          break;
        case 0x1E:
          chip8->V[0xF] = (chip8->I + chip8->V[x] > 0xFFF) ? 1 : 0;
          chip8->I = chip8->I + chip8->V[x];
          chip8->pc += 2;
          break;
        case 0x29:
          chip8->I = FONTSET_POSITION * chip8->V[x];
          chip8->pc += 2;
          break;
        case 0x33:
          //chip8->memory[i] = (chip8->V[x] % 1000) / 100;
          chip8->memory[chip8->I] = chip8->V[x] / 100;
          chip8->memory[chip8->I + 1] = (chip8->V[x] % 100) / 10;
          chip8->memory[chip8->I + 2] = chip8->V[x] % 10;
          chip8->pc += 2;
          break;
        case 0x55:
          for (int i = 0; i <= x; i++) {
            chip8->memory[chip8->I + i] = chip8->V[i];
          }
          chip8->I = chip8->I + x + 1;
          chip8->pc += 2;
          break;
        case 0x65:
          for (int i = 0; i <= x; i++) {
            chip8->V[i] = chip8->memory[chip8->I + i];
          }
          chip8->I = chip8->I + x + 1;
          chip8->pc += 2;
          break;
        default:
          //printf("0xF000 unknown opcode %#X\n", opcode);
          break;
      }
      break;
    default:
      break;
  }
  //printf("@@@@@@@  PC: 0x%04x  %d \n", chip8->pc, chip8->pc);
}

int get_key() {
  Keypad.checkChanges();                   // let the key scan library to update key status
  if (Keypad.isKeyChanged() == true) {     // if there is any detectable key change
    //Serial.println(Keypad.getKeyValue());  // print key value on Serial (use Serial Monitor to see it!)
    return Keypad.getKeyValue();
  }
  delay(1);
  return 66;
}

// 递归遍历目录（增加过滤，移除isHidden()）
void listDir(fs::FS& fs, const char* dirname, uint8_t levels) {
  File root = fs.open(dirname);
  if (!root) return;
  if (!root.isDirectory()) return;

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();

    // 过滤系统目录和隐藏文件（仅保留可靠判断）
    if (fileName.indexOf("System Volume Information") != -1 ||  // 跳过系统还原目录
        fileName.startsWith(".")) {                             // 跳过以.开头的隐藏文件（Linux/macOS）
      file.close();
      file = root.openNextFile();
      continue;
    }

    String path = String(dirname) + "/" + fileName;
    fileList.push_back(path);

    if (file.isDirectory() && levels) {
      listDir(fs, path.c_str(), levels - 1);
    }
    file.close();  // 关闭文件句柄避免泄漏
    file = root.openNextFile();
  }
  root.close();  // 关闭根目录句柄
}

// 初始化SD卡
bool initSDCard() {
  if (!SD_MMC.begin("/root", true)) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 20, "SD Card Failed!");
    u8g2.sendBuffer();
    delay(2000);
    return false;
  }
  fileList.clear();
  listDir(SD_MMC, "/", 0);  // 0表示遍历所有层级
  return true;
}

// 绘制文件列表页面
void drawFileList() {
  u8g2.clearBuffer();

  // 计算当前页显示范围
  int startIdx = currentPage * itemsPerPage;
  int endIdx = min(startIdx + itemsPerPage, (int)fileList.size());

  // 绘制标题
  u8g2.drawStr(0, 10, "Files:");

  // 绘制文件列表
  for (int i = startIdx; i < endIdx; i++) {
    int y = (i - startIdx) * 10 + 20;
    String fileName = fileList[i].substring(fileList[i].lastIndexOf('/') + 1);

    // 光标高亮
    if (i == currentCursor) {
      u8g2.drawBox(0, y - 8, 128, 10);
      u8g2.setDrawColor(0);
    }

    u8g2.drawStr(2, y, fileName.c_str());
    u8g2.setDrawColor(1);
  }

  // 绘制页码
  String pageInfo = String(currentPage + 1) + "/" + String((fileList.size() + itemsPerPage - 1) / itemsPerPage);
  u8g2.drawStr(100, 60, pageInfo.c_str());

  u8g2.sendBuffer();
}

// 文件操作函数（示例）
void handleFileOperation(String filePath) {
  isFileMode = true;
  selectedFile = filePath;
  chip8_init(&chip8_core);
  File file = SD_MMC.open(filePath, FILE_READ);
  //File file = SD_MMC.open("/test_opcode.ch8", FILE_READ);
  if (!file) {
    Serial.println("无法打开文件 /test_opcode.ch8");
    return;
  }

  // 获取文件大小
  size_t fileSize = file.size();
  if (fileSize > sizeof(chip8_core.memory) - 0x200) {
    Serial.println("文件过大，无法加载到内存！");
    file.close();
    return;
  }

  // 读取文件内容到 chip8.memory[0x200] 开始
  size_t bytesRead = file.read(chip8_core.memory + 0x200, fileSize);
  if (bytesRead != fileSize) {
    Serial.println("读取文件失败！");
  } else {
    Serial.printf("成功加载 %d 字节到内存 0x200 起始位置\n", bytesRead);
  }

  file.close();
  u8g2.clearBuffer();
  Keypad.setDetectMode(1);
  while (1) {
    /*
    uint32_t now = micros();
    //int now = micros();
    chip_get_key();
    emulate_cycle(&chip8_core);
    chip8_tick(&chip8_core);
    // 1. 执行CHIP-8指令（按原始速度，每轮10条）
    //Serial.println(now - last_render_us);
    //Serial.println(FRAME_INTERVAL_US);
    // 2. 仅在「需要绘制」且「达到帧率间隔」时渲染
    if (chip8_core.draw && (now - last_render_us >= FRAME_INTERVAL_US)) {
        // 3. 仅当画面有变化时才渲染（避免无意义刷屏）
        //Serial.println(999999);
        if (chip8_has_gfx_change(&chip8_core)) {
            chip8_render(&chip8_core);       // 绘制CHIP-8区域（其余黑色）
            chip8_sync_gfx_prev(&chip8_core);// 同步上一帧
        }
        chip8_core.draw = false;       // 重置绘制标志
        last_render_us = now;     // 更新渲染时间
    }
    */
    
    //u8g2.clearBuffer();
    //chip_get_key();
    uint32_t start_us = micros();
    pressed_key = -1;
    /*
    int num = 0;
    Keypad.checkChanges();                   // let the key scan library to update key status
  if (Keypad.isKeyChanged() == true) {     // if there is any detectable key change
    num = Keypad.getKeyValue();  // print key value on Serial (use Serial Monitor to see it!)
    //return Keypad.getKeyValue();
  }
  */
  Keypad.checkChanges(); 
            int num = Keypad.getKeyValue(); // 获取当前按住的键值（无则返回0/无效值）
            //Serial.println(num);
            
      if (num != 0) { // 有按键按下
            // 映射物理键值到CHIP-8键值（0-0xF），并赋值V[x]
            switch (num) {
                case 25: pressed_key = 7; break;
                case 26: pressed_key = 4; break;
                case 27: pressed_key = 1; break;
                case 19: pressed_key = 8; break;
                case 20: pressed_key = 5; break;
                case 21: pressed_key = 2; break;
                case 13: pressed_key = 9; break;
                case 14: pressed_key = 6; break;
                case 15: pressed_key = 3; break;
                case 7:  pressed_key = 14; break;
                case 8:  pressed_key = 13; break;
                case 9:  pressed_key = 12; break;
                case 1:  pressed_key = 15; break;
                case 2:  pressed_key = 11; break;
                case 3:  pressed_key = 0; break;
                case 11: pressed_key = 10; break;
                default: pressed_key = -1; break; // 无效键
            }
        }
    //Serial.println(pressed_key);
    emulate_cycle(&chip8_core);
    
    //
    /*
    if (chip8_core.pc == 552) {
      break;
    }
    */
    //
    if (chip8_core.draw) {
      real_draw(&chip8_core);
      chip8_core.draw = false;
    }
    chip8_tick(&chip8_core);
    
    uint32_t exec_us = micros() - start_us; // 计算指令执行耗时
    
//delayMicroseconds(300);
    // 补全延时：确保总间隔≈1667μs
    if (exec_us < INSTR_INTERVAL_US) {
        delayMicroseconds(INSTR_INTERVAL_US - exec_us);
    }
    
    
    if (num == 5) {
      // while (digitalRead(KEY_ESC) == LOW)
      //   ;  // 消抖
      isFileMode = false;
      break;
    }
    
    //showFPS(u8g2);                     // ← 前面提供的函数
    //delay(9);
// ---- 3️⃣ 发送缓冲区到 OLED ----
//u8g2.sendBuffer(); 
    //Sleep(160);
  }
  /*
  while (isFileMode) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 20, "Editing:");
    u8g2.drawStr(0, 35, filePath.c_str());
    u8g2.drawStr(0, 50, "[ESC] Back");
    u8g2.sendBuffer();

    // 检测退出键
    if (get_key() == 5) {
      //while (digitalRead(KEY_ESC) == LOW)
      //  ;  // 消抖
      isFileMode = false;
    }
    delay(50);
  }
  */
  Keypad.setDetectMode(0);
}

// 按键处理
void handleKeys() {
  // 上键
  /*
  Serial.println(get_key());
  if (get_key() == 66) {
    //return;
  }
  */
Keypad.checkChanges();                   // let the key scan library to update key status
  if (Keypad.isKeyChanged() == true) {     // if there is any detectable key change
   // Serial.println(Keypad.getKeyValue());  // print key value on Serial (use Serial Monitor to see it!)
    if (Keypad.getKeyValue() == 21 && keyUpState == HIGH) {
    keyUpState = LOW;
    if (currentCursor > 0) {
      currentCursor--;
      // 自动翻页
      if (currentCursor < currentPage * itemsPerPage) {
        currentPage = max(0, currentPage - 1);
      }
    }
    drawFileList();
  } else {
    keyUpState = HIGH;
  }

  // 下键
  if (Keypad.getKeyValue() == 19 && keyDownState == HIGH) {
    keyDownState = LOW;
    if (currentCursor < fileList.size() - 1) {
      currentCursor++;
      // 自动翻页
      if (currentCursor >= (currentPage + 1) * itemsPerPage) {
        currentPage = min((int)((fileList.size() - 1) / itemsPerPage), currentPage + 1);
      }
    }
    drawFileList();
  } else {
    keyDownState = HIGH;
  }

  // 左键（上一页）
  if (Keypad.getKeyValue() == 26 && keyLeftState == HIGH) {
    keyLeftState = LOW;
    if (currentPage > 0) {
      currentPage--;
      currentCursor = currentPage * itemsPerPage;  // 光标回到页首
    }
    drawFileList();
  } else {
    keyLeftState = HIGH;
  }

  // 右键（下一页）
  if (Keypad.getKeyValue() == 14 && keyRightState == HIGH)0055 {
    keyRightState = LOW;
    int maxPage = (fileList.size() + itemsPerPage - 1) / itemsPerPage - 1;
    if (currentPage < maxPage) {
      currentPage++;
      currentCursor = currentPage * itemsPerPage;  // 光标回到页首
    }
    drawFileList();
  } else {
    keyRightState = HIGH;
  }

  // 确定键
  if (Keypad.getKeyValue() ==17 && keyEnterState == HIGH) {
    keyEnterState = LOW;
    handleFileOperation(fileList[currentCursor]);
    drawFileList();  // 返回后重绘列表
  } else {
    keyEnterState = HIGH;
  }
  }
/*
  if (get_key() == 26 && keyUpState == HIGH) {
    keyUpState = LOW;
    if (currentCursor > 0) {
      currentCursor--;
      // 自动翻页
      if (currentCursor < currentPage * itemsPerPage) {
        currentPage = max(0, currentPage - 1);
      }
    }
    drawFileList();
  } else {
    keyUpState = HIGH;
  }

  // 下键
  if (get_key() == 14 && keyDownState == HIGH) {
    keyDownState = LOW;
    if (currentCursor < fileList.size() - 1) {
      currentCursor++;
      // 自动翻页
      if (currentCursor >= (currentPage + 1) * itemsPerPage) {
        currentPage = min((int)((fileList.size() - 1) / itemsPerPage), currentPage + 1);
      }
    }
    drawFileList();
  } else {
    keyDownState = HIGH;
  }

  // 左键（上一页）
  if (get_key() == 19 && keyLeftState == HIGH) {
    keyLeftState = LOW;
    if (currentPage > 0) {
      currentPage--;
      currentCursor = currentPage * itemsPerPage;  // 光标回到页首
    }
    drawFileList();
  } else {
    keyLeftState = HIGH;
  }

  // 右键（下一页）
  if (get_key() == 21 && keyRightState == HIGH) {
    keyRightState = LOW;
    int maxPage = (fileList.size() + itemsPerPage - 1) / itemsPerPage - 1;
    if (currentPage < maxPage) {
      currentPage++;
      currentCursor = currentPage * itemsPerPage;  // 光标回到页首
    }
    drawFileList();
  } else {
    keyRightState = HIGH;
  }

  // 确定键
  if (get_key() == 17 && keyEnterState == HIGH) {
    keyEnterState = LOW;
    handleFileOperation(fileList[currentCursor]);
    drawFileList();  // 返回后重绘列表
  } else {
    keyEnterState = HIGH;
  }
*/
  //delay(1);  // 消抖延迟
}

void setup() {
  Serial.begin(9600);
  swSerial.begin(9600, SERIAL_8N1, 8, 9);
  //spi2.begin(4, -1, 5, 17); // SCK, MISO, MOSI, SS(CS)
  //spi2.setFrequency(4000000);      // SPI时钟频率，SSD1309最大支持10MHz
  u8g2.begin();
  //u8g2.setFont(u8g2_font_ncenB08_tr);  // 设置字体
//u8g2_font_6x10_tr
u8g2.setFont(u8g2_font_6x10_tr);
  // 初始化按键引脚
  Serial.println(666666);
  /*
  pinMode(KEY_UP, INPUT_PULLUP);
  pinMode(KEY_DOWN, INPUT_PULLUP);
  pinMode(KEY_LEFT, INPUT_PULLUP);
  pinMode(KEY_RIGHT, INPUT_PULLUP);
  pinMode(KEY_ENTER, INPUT_PULLUP);
  pinMode(KEY_ESC, INPUT_PULLUP);
  */
#if defined(SOC_SDMMC_USE_GPIO_MATRIX)
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0, SDMMC_D1, SDMMC_D2, SDMMC_D3);
#endif
  // 初始化SD卡
  if (!initSDCard()) {
    while (1)
      ;  // SD卡初始化失败则停止
  }

  drawFileList();
}

void loop() {
  if (!isFileMode) {
    handleKeys();
  }
}