#pragma once
#include <stdint.h>
#include "cart.h"
// ppu.h - PPU 知道 Cartridge（访问 VRAM）
typedef struct PPU {
	Cartridge* cart; // 用于读取 CHR ROM

	uint8_t vram[2048];   // 内部 VRAM
	uint8_t palette[32];

	// 寄存器
	uint8_t ppuctrl, ppumask, ppustatus;
	uint16_t v, t;
	uint8_t x, w;
} PPU;

void ppu_init(PPU* ppu, Cartridge* cart);
void ppu_clock(PPU* ppu);
