#pragma once
#include <stdint.h>

void memory_init(uint8_t* prg_rom, int prg_rom_length);
uint8_t memory_read_byte(uint16_t address);
uint16_t memory_read_word(uint16_t address);
void memory_write_byte(uint16_t address, uint8_t data);
void memory_write_word(uint16_t address, uint16_t data);