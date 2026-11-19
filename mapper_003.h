#pragma once
// mapper_000_vtable.h
#include "mapper.h"
#include "bus.h"

typedef struct Mapper003 {
	Mapper base;
} Mapper003;

Mapper003* mapper_003_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks);
void mapper_003_destroy(Mapper003* mapper);