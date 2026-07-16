#include "mapper.h"
#include "mapper_000.h"
//#include ""

/* ==================== 工厂函数 ==================== /
/
Mapper* mapper_create(uint8_t mapper_id, Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks) {
switch (mapper_id) {
case 0: return (Mapper*)mapper_000_create(cart, prg_banks, chr_banks);
// case 1: return (Mapper*)mapper_001_create(cart, prg_banks, chr_banks);
// case 2: ...
default: return NULL;
}
}
*/

/* ==================== 统一销毁 ==================== */
/*
void mapper_destroy(Mapper mapper) {
free(mapper);
}
*/

/* ==================== CPU 访问 ==================== */
bool mapper_cpu_read(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr, uint8_t* data) {
  if (!mapper || !mapped_addr) return false;
  return mapper->vtable->cpu_read(mapper, addr, mapped_addr, data);
}

bool mapper_cpu_write(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr, uint8_t data) {
  if (!mapper || !mapped_addr) return false;
  return mapper->vtable->cpu_write(mapper, addr, mapped_addr, data);
}

/* ==================== PPU 访问 ==================== */
bool mapper_ppu_read(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr) {
  if (!mapper || !mapped_addr) return false;
  return mapper->vtable->ppu_read(mapper, addr, mapped_addr);
}

bool mapper_ppu_write(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr) {
  if (!mapper || !mapped_addr) return false;
  return mapper->vtable->ppu_write(mapper, addr, mapped_addr);
}

/* ==================== 控制/状态 ==================== */
void mapper_reset(Mapper* mapper) {
  if (mapper && mapper->vtable->reset) {
    mapper->vtable->reset(mapper);
  }
}

MirrorMode mapper_mirror(Mapper* mapper) {
  if (mapper && mapper->vtable->mirror) {
    return mapper->vtable->mirror(mapper);
  }
  return MIRROR_HARDWARE;
}

bool mapper_irq_state(Mapper* mapper) {
  if (mapper && mapper->vtable->irq_state) {
    return mapper->vtable->irq_state(mapper);
  }
  return false;
}

void mapper_irq_clear(Mapper* mapper) {
  if (mapper && mapper->vtable->irq_clear) {
    mapper->vtable->irq_clear(mapper);
  }
}

void mapper_scanline(Mapper* mapper) {
  if (mapper && mapper->vtable->scanline) {
    mapper->vtable->scanline(mapper);
  }
}