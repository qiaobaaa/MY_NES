#pragma once
#include <stdint.h>
#include "mapper.h"

typedef struct Mapper Mapper;

// cartridge.h - Cartridge 是数据存储中心
typedef struct Cartridge {
	Mapper* mapper;
	uint8_t header[16];
	int prg_rom_size; // PRG ROM 大小 (Byte)
	int chr_rom_size; // CHR ROM 大小 (Byte)
	//int prg_ram_size; // PRG RAM 大小 (Byte)
	
	uint8_t* prg_rom;     // PRG ROM 数据
	uint8_t* chr_rom;     // CHR ROM 数据
	uint8_t prg_banks;
	uint8_t chr_banks;
} Cartridge;

extern Cartridge cart;