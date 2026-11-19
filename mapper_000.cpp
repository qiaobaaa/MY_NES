#include "mapper_000.h"
#include <stdlib.h>

static uint8_t* prg_ptr;
static uint8_t* chr_ptr;
static uint16_t prg_size;
// ========== 函数实现 ==========

static uint8_t cpu_read(void* self, uint16_t addr) {
    Mapper000* m = (Mapper000*)self;
    return prg_ptr[(addr - PRG_32K) % prg_size];
   //return prg_ptr[(addr - PRG_32K)];
}

static bool cpu_write(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t data) {
    Mapper000* m = (Mapper000*)self;
    return true;
}

static uint8_t ppu_read(void* self, uint16_t addr) {
    return chr_ptr[addr];
}

static bool ppu_write(void* self, uint16_t addr, uint32_t* mapped_addr) {
    Mapper000* m = (Mapper000*)self;
    return true;
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
    
}

static void irq_trigger(void* self) {
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
.register_write = register_write,
.irq_trigger = irq_trigger,
};

// ========== 构造函数 ==========

Mapper000* mapper_000_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks) {
    Mapper000* m = (Mapper000*)malloc(sizeof(Mapper000));
    if (!m) return NULL;

    // 设置共享的 vtable
    m->base.vtable = &mapper_000_vtable;
    //m->base.cart = cart;
    m->base.prg_banks = prg_banks;
    m->base.chr_banks = chr_banks;
    //m->base.cart = cart;
    printf("哈哈哈哈   %d    %d\n",cart->prg_rom_size , cart->prg_rom_size);
    //ppu_copy(0x0000, cart->chr_rom, 0x2000);
    //memcpy(cpu_prg_rom, cart->prg_rom, cart->prg_rom_size);
    prg_ptr = cart->prg_rom;   //mapper0不存在切换直接赋值就可以了
    chr_ptr = chr_rom_flag ? cart->chr_rom : nullptr;
    prg_size = prg_banks * PRG_16K;
    return m;
}

void mapper_000_destroy(Mapper000* mapper) {
    free(mapper);
}
