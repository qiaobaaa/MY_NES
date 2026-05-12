#pragma once
#include <stdint.h>
// cartridge.h - Cartridge 是数据存储中心
typedef struct Cartridge {
	Mapper* mapper;

	uint8_t* prg_rom;     // PRG ROM 数据
	uint8_t* chr_rom;     // CHR ROM 数据
	uint8_t prg_banks;
	uint8_t chr_banks;
} Cartridge;

bool cart_cpu_read(Cartridge* cart, uint16_t addr, uint8_t* data);
bool cart_ppu_read(Cartridge* cart, uint16_t addr, uint8_t* data);