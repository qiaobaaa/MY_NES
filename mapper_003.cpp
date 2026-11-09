#include "mapper_003.h"
#include <stdlib.h>

static bool cpu_read(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t* data) {
    Mapper003* m = (Mapper003*)self;

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
    Mapper003* m = (Mapper003*)self;

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
    Mapper003* m = (Mapper003*)self;

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

static void register_write(void* self, uint16_t addr, uint8_t data){
    uint8_t chr_bank_num = data & 0x3;  //提取低位字节两位
    
    memcpy(ppu.ram, cart.chr_rom + chr_bank_num * 0x2000, 0x2000);
    
    //memcpy(cpu_prg_rom, cart.prg_rom + (prg_bank_num * 0x4000), 0x4000);
    //printf("hahha\n");
}

static const MapperVTable mapper_003_vtable = {
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

Mapper003* mapper_003_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks) {
    Mapper003* m = (Mapper003*)malloc(sizeof(Mapper003));
    if (!m) return NULL;

    // 设置共享的 vtable
    m->base.vtable = &mapper_003_vtable;
    //m->base.cart = cart;
    m->base.prg_banks = prg_banks;
    m->base.chr_banks = chr_banks;
    
    m->base.cart = cart;
    
    //ppu_copy(0x0000, cart->chr_rom, 0x2000);
    //mapper2最主要的就是切换prg，默认先把0给0x8000-BFFF,之后来回切换
    //0xC000-0xFFFF,一定是最后一个bank
    printf("...............%d\n", prg_banks);
     
    if (prg_banks == 1)
    {
        memcpy(cpu_prg_rom, cart->prg_rom, 0x4000);
    }
    else if (prg_banks == 2)
    {
        memcpy(cpu_prg_rom, cart->prg_rom, 0x8000);
    }
    
    //把第一个chr给ppu，块级别是8k
    memcpy(ppu.ram, cart->chr_rom, 0x2000);
    //memcpy(cpu_prg_rom + 0x4000, cart->prg_rom + ((prg_banks-1) * 0x4000), 0x4000);
    return m;
}


void mapper_003_destroy(Mapper003* mapper) {
    free(mapper);
}