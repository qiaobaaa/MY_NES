#pragma once
// mapper_000_vtable.h
#include "mapper.h"

typedef struct Mapper000 {
	Mapper base;
} Mapper000;

Mapper000* mapper_000_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks);
void mapper_000_destroy(Mapper000* mapper);
