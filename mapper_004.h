#pragma once
// mapper_000_vtable.h
#include "mapper.h"
#include "bus.h"

extern uint32_t count;

typedef struct Mapper004 {
  Mapper base;
  union {
    struct {
      uint8_t RRR : 3;
      uint8_t xx : 2;
      uint8_t M : 1;
      uint8_t P : 1;
      uint8_t C : 1;
    };
    uint8_t bank_select;
  };
  uint8_t irq_latch;
  uint8_t irq_reload;
  uint8_t irq_enable;
  uint8_t prg_ram_protect;
  uint8_t nametable_arrangement;
} Mapper004;

Mapper004* mapper_004_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks);
void mapper_004_destroy(Mapper004* mapper);
