#include "mapper_004.h"
#include <stdlib.h>

uint8_t* mapper_004_chr_ptr[5];
// ========== 函数实现 ==========

static bool cpu_read(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t* data) {
  Mapper004* m = (Mapper004*)self;

  if (addr >= 0x8000) {
    if (m->base.prg_banks > 1) {
      *mapped_addr = addr & 0x7FFF;
    } else {
      *mapped_addr = addr & 0x3FFF;
    }
    return true;
  }
  return false;
}

static bool cpu_write(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t data) {
  Mapper004* m = (Mapper004*)self;

  if (addr >= 0x8000) {
    if (m->base.chr_banks == 0) {
      *mapped_addr = addr & (m->base.prg_banks > 1 ? 0x7FFF : 0x3FFF);
      return true;
    }
  }
  return false;
}

static bool ppu_read(void* self, uint16_t addr, uint32_t* mapped_addr) {
  if (addr <= 0x1FFF) {
    *mapped_addr = addr;
    return true;
  }
  return false;
}

static bool ppu_write(void* self, uint16_t addr, uint32_t* mapped_addr) {
  Mapper004* m = (Mapper004*)self;

  if (addr <= 0x1FFF && m->base.chr_banks == 0) {
    *mapped_addr = addr;
    return true;
  }
  return false;
}

static void reset(void* self) {
}

static MirrorMode mirror(void* self) {
  return MIRROR_HARDWARE;
}

static bool irq_state(void* self) {
  return false;
}

static void irq_clear(void* self) {
}

static void scanline(void* self) {
}

static void mapper_bank_select(void* self, uint8_t data) {
}
static void bank_data(void* self, uint8_t data) {
}

static void register_write(void* self, uint16_t addr, uint8_t data) {
  Mapper004* m = (Mapper004*)self;
  uint8_t even = addr & 0x01;
  if (0x8000 <= addr && addr <= 0x9FFE && !even) {
    mapper_bank_select(m, data);
  } else if (0x8001 <= addr && addr <= 0x9FFF && even) {
    bank_data(m, data);
  }
}
// ========== vtable 定义（共享的） ==========

static const MapperVTable mapper_004_vtable = {
  .cpu_read = cpu_read,
  .cpu_write = cpu_write,
  .ppu_read = ppu_read,
  .ppu_write = ppu_write,
  .reset = reset,
  .mirror = mirror,
  .irq_state = irq_state,
  .irq_clear = irq_clear,
  .scanline = scanline,
  .register_write = register_write,
};

// ========== 构造函数 ==========

Mapper004* mapper_004_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks) {
  Mapper004* m = (Mapper004*)malloc(sizeof(Mapper004));
  if (!m) return NULL;

  // 设置共享的 vtable
  m->base.vtable = &mapper_004_vtable;
  //m->base.cart = cart;
  m->base.prg_banks = prg_banks;
  m->base.chr_banks = chr_banks;
  m->base.cart = cart;
  printf("哈哈哈哈   %d    %d\n", cart->prg_rom_size, cart->prg_rom_size);
  ppu_copy(0x0000, cart->chr_rom, 0x2000);
  memcpy(cpu_prg_rom, cart->prg_rom, cart->prg_rom_size);
  return m;
}

void mapper_004_destroy(Mapper004* mapper) {
  free(mapper);
}