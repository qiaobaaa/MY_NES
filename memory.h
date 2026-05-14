#pragma once
#include <stdint.h>
#include "cpu.h"

void memory_init(uint8_t* prg_rom, int prg_rom_length);
uint8_t memory_read_byte(uint16_t address, CPU* cpu);
uint16_t memory_read_word(uint16_t address, CPU* cpu);
void memory_write_byte(uint16_t address, uint8_t data, CPU* cpu);
void memory_write_word(uint16_t address, uint16_t data, CPU* cpu);