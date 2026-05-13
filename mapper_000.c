#include "mapper_000.h"
#include <stdlib.h>

// ========== 函数实现 ==========

static bool cpu_read(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t* data) {
    Mapper000* m = (Mapper000*)self;

    if (addr >= 0x8000) {
        if (m->base.prg_banks > 1) {
            *mapped_addr = addr & 0x7FFF;
        }
        else {
            *mapped_addr = addr & 0x3FFF;
        }
        return true;
    }
    return false;
}

static bool cpu_write(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t data) {
    Mapper000* m = (Mapper000*)self;

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
    Mapper000* m = (Mapper000*)self;

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

// ========== vtable 定义（共享的） ==========

static const MapperVTable mapper_000_vtable = {
.cpu_read = cpu_read,
.cpu_write = cpu_write,
.ppu_read = ppu_read,
.ppu_write = ppu_write,
.reset = reset,
.mirror = mirror,
.irq_state = irq_state,
.irq_clear = irq_clear,
.scanline = scanline,
};

// ========== 构造函数 ==========

Mapper000* mapper_000_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks) {
    Mapper000* m = (Mapper000*)malloc(sizeof(Mapper000));
    if (!m) return NULL;

    // 设置共享的 vtable
    m->base.vtable = &mapper_000_vtable;
    m->base.cart = cart;
    m->base.prg_banks = prg_banks;
    m->base.chr_banks = chr_banks;

    return m;
}

void mapper_000_destroy(Mapper000* mapper) {
    free(mapper);
}
    m->base.chr_banks = chr_banks;

    return m;
}

void mapper_000_destroy(Mapper000* mapper) {
    free(mapper);
}
