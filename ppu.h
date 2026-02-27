#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
  /* PPU 寄存器 */
  uint8_t ppuctrl;                   // 2000: PPU 控制寄存器，WRITE
  uint8_t ppumask;                   // 2001: PPU MASK 寄存器，WRITE
  uint8_t ppustatus;                 // 2002: PPU 状态寄存器，READ
  uint8_t oamaddr;                   // 2003: PPU OAM 地址，WRITE
  uint8_t oamdata;                   // 2004: PPU OAM 数据，READ/WRITE
  uint16_t ppuscroll;                // 2005: PPU 滚动位置寄存器，WRITE x2
  uint8_t ppuscroll_x, ppuscroll_y;  // 将 PPUSCROLL 寄存器分为 X 和 Y 两个方向
  uint16_t ppuaddr;                  // 2006: PPU 地址寄存器，WRITE x2
  uint16_t ppudata;                  // 2007: PPU 数据寄存器，READ/WRITE
  uint8_t oamdma;                    // 4104: OAM DMA 寄存器（高字节），WRITE

  bool scroll_received_x;
  bool addr_received_high_byte;
  bool ready;

  int mirroring, mirroring_xor;

  int x, scanline;

  uint8_t ppu_sprram[0x100];
  uint8_t ppu_ram[0x4000];
} PPU;

#endif