#include "mapper_001.h"
#include <stdlib.h>

static bool cpu_read(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t* data) {
  Mapper001* m = (Mapper001*)self;

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
  Mapper001* m = (Mapper001*)self;

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
  Mapper001* m = (Mapper001*)self;

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

static void control(Mapper001* m, uint8_t result) {
}

static void chr_bank0(Mapper001* m, uint8_t result) {
}

static void chr_bank1(Mapper001* m, uint8_t result) {
}

static void prg_bank(Mapper001* m, uint8_t result) {
}

static void register_write(void* self, uint16_t addr, uint8_t data) {
  Mapper001* m = (Mapper001*)self;

  //这个区间会重置移位寄存器
  if (data >= 0x80 && data <= 0xFF) {
    m->shift_register = 0;
    m->write_record = 0;
    m->control = 0x0C;
    return;
  }

  if (m->write_record < 4) {
    uint8_t bit0 = data & 0x01;
    m->shift_register |= bit0 << m->write_record;
    m->write_record++;
  } else {
    uint8_t result = 0;
    result |= m->shift_register;
    uint8_t bit0 = data & 0x01;
    result |= bit0 << m->write_record;

    //第五次写入完成后重置
    m->shift_register = 0;
    m->write_record = 0;
    m->control = 0x0C;

    uint8_t select = (addr >> 13) & 0x03;
    switch (select) {
      case 0:  //Control (internal, $8000-$9FFF)
        control(m, result);
        break;
      case 1:  //CHR bank 0 (internal, $A000-$BFFF)
        chr_bank0(m, result);
        break;
      case 2:  //CHR bank 1 (internal, $C000-$DFFF)
        chr_bank1(m, result);
        break;
      case 3:  //PRG bank (internal, $E000-$FFFF)
        prg_bank(m, result);
        break;
      default:
        break;
    }
  }
}

static const MapperVTable mapper_001_vtable = {
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

Mapper001* mapper_001_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks) {
  Mapper001* m = (Mapper001*)malloc(sizeof(Mapper001));
  if (!m) return NULL;

  // 设置共享的 vtable
  m->base.vtable = &mapper_001_vtable;
  //m->base.cart = cart;
  m->base.prg_banks = prg_banks;
  m->base.chr_banks = chr_banks;

  m->base.cart = cart;

  m->shift_register = 0;
  m->write_record = 0;
  m->control = 0x0C;
  m->chr_bank0 = 0;
  m->chr_bank1 = 0;
  m->prg_bank = 0;

  //ppu_copy(0x0000, cart->chr_rom, 0x2000);
  //mapper2最主要的就是切换prg，默认先把0给0x8000-BFFF,之后来回切换
  //0xC000-0xFFFF,一定是最后一个bank
  printf("...............%d\n", prg_banks);
  //memcpy(cpu_prg_rom, cart->prg_rom, 0x4000);
  //memcpy(cpu_prg_rom + 0x4000, cart->prg_rom + ((prg_banks-1) * 0x4000), 0x4000);
  return m;
}


void mapper_001_destroy(Mapper001* mapper) {
  free(mapper);
}