#pragma once
#include "mapper.h"
#include "bus.h"

typedef struct Mapper001 {
	Mapper base;
    uint8_t shift_register;
    uint8_t write_record;
    union {
        struct {
            uint8_t M:2;                
            uint8_t P:2;                
            uint8_t C:1;                
            uint8_t unused:3;
        };
        uint8_t control;                     
    };
    uint8_t chr_bank0;
    uint8_t chr_bank1;
    uint8_t prg_bank;
} Mapper001;

Mapper001* mapper_001_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks);
void mapper_001_destroy(Mapper001* mapper);