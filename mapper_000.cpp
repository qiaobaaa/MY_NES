#include "mapper_000.h"
#include <stdlib.h>
#include <stddef.h>

/* ==================== 辅助宏 ==================== */
#define Mapper000(self) ((Mapper000*)(self))

/* ==================== CPU 读 ==================== */
static bool cpu_read(Mapper* self, uint16_t addr, uint32_t* mapped_addr, uint8_t* data) {
  (void)data;  // NROM 只读，数据从 ROM 数组读取

  if (addr < 0x8000) return false;

  Mapper000* mapper = Mapper000(self);
  if (mapper->base.prg_banks > 1) {
    *mapped_addr = addr & 0x7FFF;  // 32KB 模式
  } else {
    *mapped_addr = addr & 0x3FFF;  // 16KB 镜像模式
  }
  return true;
}

/* ==================== CPU 写 ==================== */
static bool cpu_write(Mapper* self, uint16_t addr, uint32_t* mapped_addr, uint8_t data) {
  (void)self;
  (void)data;

  if (addr < 0x8000) return false;

  Mapper000* mapper = Mapper000(self);
  // 只有 CHR RAM 时才允许 PRG 写入
  if (mapper->base.chr_banks == 0) {
    *mapped_addr = addr & (mapper->base.prg_banks > 1 ? 0x7FFF : 0x3FFF);
    return true;
  }
  return false;
}

/* ==================== PPU 读 ==================== */
static bool ppu_read(Mapper* self, uint16_t addr, uint32_t* mapped_addr) {
  (void)self;

  if (addr > 0x1FFF) return false;
  *mapped_addr = addr;
  return true;
}

/* ==================== PPU 写 ==================== */
static bool ppu_write(Mapper* self, uint16_t addr, uint32_t* mapped_addr) {
  Mapper000* mapper = Mapper000(self);

  if (addr > 0x1FFF) return false;

  // CHR RAM 模式下允许写入
  if (mapper->base.chr_banks == 0) {
    *mapped_addr = addr;
    return true;
  }
  return false;
}

/* ==================== 控制函数 ==================== */
static void reset(Mapper* self) {
  //printf("666666666\n");
  (void)self;  // NROM 无状态
}

static MirrorMode mirror(Mapper* self) {
  (void)self;
  return MIRROR_HARDWARE;
}

static bool irq_state(Mapper* self) {
  (void)self;
  return false;
}

static void irq_clear(Mapper* self) {
  (void)self;
}

static void scanline(Mapper* self) {
  (void)self;
}

/* ==================== 虚函数表 ==================== */
static const MapperVTable mapper_000_vtable = {
  .cpu_read = cpu_read,
  .cpu_write = cpu_write,
  .ppu_read = ppu_read,
  .ppu_write = ppu_write,
  .reset = reset,
  .mirror = mirror,
  .irq_state = irq_state,
  .irq_clear = irq_clear,
  .scanline = NULL,  // 可选，支持 NULL
};

/* ==================== 构造函数 ==================== */
Mapper000* mapper_000_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks) {
  Mapper000* mapper = (Mapper000*)malloc(sizeof(Mapper000));
  if (!mapper) return NULL;

  mapper->base.vtable = &mapper_000_vtable;
  mapper->base.cart = cart;
  mapper->base.prg_banks = prg_banks;
  mapper->base.chr_banks = chr_banks;

  return mapper;
}

void mapper_000_destroy(Mapper000* mapper) {
  free(mapper);
}