#pragma once
// mapper_000_vtable.h
#include "mapper.h"
#include "bus.h"

typedef struct Mapper002 {
	Mapper base;
} Mapper002;

Mapper002* mapper_002_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks);
void mapper_002_destroy(Mapper002* mapper);