#include "mapper_002.h"
#include <stdlib.h>

int dirty = -1;

static uint8_t* prg_ptr[2];
static uint8_t* chr_ptr;

static uint8_t cpu_read(void* self, uint16_t addr) {
    Mapper002* m = (Mapper002*)self;

    if (addr < 0xC000)
    {
        return prg_ptr[0][addr - PRG_32K]; 
    }
    return prg_ptr[1][addr - 0xC000];
}

static bool cpu_write(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t data) {
    Mapper002* m = (Mapper002*)self;

    if (addr >= 0x8000) {
        if (m->base.chr_banks == 0) {
            *mapped_addr = addr & (m->base.prg_banks > 1 ? 0x7FFF : 0x3FFF);
            return true;
        }
    }
    return false;
}

static uint8_t ppu_read(void* self, uint16_t addr) {
    return chr_ptr[addr];
}

static bool ppu_write(void* self, uint16_t addr, uint32_t* mapped_addr) {
    Mapper002* m = (Mapper002*)self;

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

static void irq_trigger(void* self) {
}

static void register_write(void* self, uint16_t addr, uint8_t data){
    uint8_t prg_bank_num = data & 0xF;  //提取低位字节
    /*
    if(dirty!=prg_bank_num)
    {
        dirty=prg_bank_num;
        memcpy(cpu_prg_rom, cart.prg_rom + (dirty * 0x4000), 0x4000);
    }
    */
    //memcpy(cpu_prg_rom, cart.prg_rom + (prg_bank_num * 0x4000), 0x4000);
    prg_ptr[0] = cart.prg_rom + (prg_bank_num * PRG_16K);
    //printf("hahha\n");
}

static const MapperVTable mapper_002_vtable = {
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
.irq_trigger = irq_trigger,
};

// ========== 构造函数 ==========

Mapper002* mapper_002_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks) {
    Mapper002* m = (Mapper002*)malloc(sizeof(Mapper002));
    if (!m) return NULL;

    // 设置共享的 vtable
    m->base.vtable = &mapper_002_vtable;
    //m->base.cart = cart;
    m->base.prg_banks = prg_banks;
    m->base.chr_banks = chr_banks;
    
    m->base.cart = cart;
    
    //ppu_copy(0x0000, cart->chr_rom, 0x2000);
    //mapper2最主要的就是切换prg，默认先把0给0x8000-BFFF,之后来回切换
    //0xC000-0xFFFF,一定是最后一个bank
    printf("...............%d\n", prg_banks);
    //memcpy(cpu_prg_rom, cart->prg_rom, 0x4000);
    //memcpy(cpu_prg_rom + 0x4000, cart->prg_rom + ((prg_banks-1) * 0x4000), 0x4000);
    prg_ptr[0] = cart->prg_rom;
    prg_ptr[1] = cart->prg_rom + ((prg_banks-1) * PRG_16K);
    chr_ptr = chr_rom_flag ? cart->chr_rom : nullptr;
    return m;
}


void mapper_002_destroy(Mapper002* mapper) {
    free(mapper);
}