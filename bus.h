#pragma once
#include <stdint.h>
#include "ppu.h"
#include "io.h"

extern uint8_t* prg_rom_ptr;
extern int prg_rom_size;
extern uint8_t interal_ram[0x0800];
extern uint8_t save_ram[0x2000];

extern Cartridge cart;
extern uint8_t *pprg_rom[2];

extern uint8_t open_buss;
void memory_init(uint8_t* prg_rom, int prg_rom_length);

static inline uint8_t ss(uint16_t address)
{
    if (address < 0xC000 && address >= 0x8000) {
        return pprg_rom[0][address - 0x8000];
    } else if (address >= 0xC000) {
        return pprg_rom[1][address - 0xC000];
    }
    return 0;
}

/* ==================================================================
 * 总线分派：两种实现，用一个宏切换，便于 A/B 对比帧率
 *
 *   BUS_USE_LUT = 1   函数指针查表（8 个 8KB 区间各对应一个函数）
 *   BUS_USE_LUT = 0   switch（原版，已长期验证）
 *
 * 两块共用同一套 DMA 特殊路径（$4014），所以只切换普通读写分派。
 *
 * 【关键】表项必须与 8 个 8KB 区间严格一一对位。
 *         错一格 → $2000-$3FFF 会被当成 RAM 读，CPU 永远等不到
 *         VBlank(bit7)，卡在第一帧；同时 $8000+ 取指错位 → 灰屏。
 *
 * 区间划分（address >> 13）：
 *   0  $0000-$1FFF  内部 RAM      4  $8000-$9FFF  PRG
 *   1  $2000-$3FFF  PPU 寄存器    5  $A000-$BFFF  PRG
 *   2  $4000-$5FFF  APU / IO      6  $C000-$DFFF  PRG
 *   3  $6000-$7FFF  Save RAM      7  $E000-$FFFF  PRG
 * ================================================================== */
#define BUS_USE_LUT  1

#if BUS_USE_LUT

typedef uint8_t (*bus_read_fn)(uint16_t);
typedef void    (*bus_write_fn)(uint16_t, uint8_t);

/* ---------- 读 ---------- */
static uint8_t bus_read_ram(uint16_t addr)  { return interal_ram[addr & 0x07FF]; }
static uint8_t bus_read_ppu(uint16_t addr)  { return ppu_register_read(addr); }
static uint8_t bus_read_io(uint16_t addr)   { return io_read(addr); }
static uint8_t bus_read_sram(uint16_t addr) { return save_ram[addr - 0x6000]; }
static uint8_t bus_read_prg(uint16_t addr)  { return cart.mapper->cpu_read(cart.mapper, addr); }

static const bus_read_fn bus_read_table[8] = {
    bus_read_ram,    /* 0  $0000-$1FFF */
    bus_read_ppu,    /* 1  $2000-$3FFF */
    bus_read_io,     /* 2  $4000-$5FFF */
    bus_read_sram,   /* 3  $6000-$7FFF */
    bus_read_prg,    /* 4  $8000-$9FFF */
    bus_read_prg,    /* 5  $A000-$BFFF */
    bus_read_prg,    /* 6  $C000-$DFFF */
    bus_read_prg     /* 7  $E000-$FFFF */
};

/* ---------- 写 ---------- */
static void bus_write_ram(uint16_t addr, uint8_t data)  { interal_ram[addr & 0x07FF] = data; }
static void bus_write_ppu(uint16_t addr, uint8_t data)  { ppu_register_write(addr, data); }
static void bus_write_io(uint16_t addr, uint8_t data)   { io_write(addr, data); }
static void bus_write_sram(uint16_t addr, uint8_t data) {
    if (!(cart.mapper->prg_ram_protect & 0x40)) {
        save_ram[addr - 0x6000] = data;
    }
}
static void bus_write_prg(uint16_t addr, uint8_t data)  { cart.mapper->register_write(cart.mapper, addr, data); }

static const bus_write_fn bus_write_table[8] = {
    bus_write_ram,   /* 0  $0000-$1FFF */
    bus_write_ppu,   /* 1  $2000-$3FFF */
    bus_write_io,    /* 2  $4000-$5FFF */
    bus_write_sram,  /* 3  $6000-$7FFF */
    bus_write_prg,   /* 4  $8000-$9FFF */
    bus_write_prg,   /* 5  $A000-$BFFF */
    bus_write_prg,   /* 6  $C000-$DFFF */
    bus_write_prg    /* 7  $E000-$FFFF */
};

#endif  /* BUS_USE_LUT */


static inline __attribute__((always_inline)) uint8_t memory_read_byte(uint16_t address) {
#if BUS_USE_LUT
    return bus_read_table[address >> 13](address);
#else
    switch (address >> 13) {
        case 0:                        // 0000 ~ 1FFF, 内部 RAM
            return interal_ram[address & 0x07FF];
        case 1:                        // 2000 ~ 3FFF, PPU 寄存器
            return ppu_register_read(address);
        case 2:                        // 4000 ~ 5FFF, APU 与 IO 寄存器
            return io_read(address);
        case 3:                        // 6000 ~ 7FFF, Save RAM
            return save_ram[address - 0x6000];
        default:                       // 8000 ~ FFFF, PRG ROM
            if (address >= 0x8000) {
                return cart.mapper->cpu_read(cart.mapper, address);
            }
            return 0;
    }
#endif
}

static inline uint16_t memory_read_word(uint16_t address) {
    return memory_read_byte(address) + (memory_read_byte(address + 1) << 8);
}


static inline __attribute__((always_inline)) void memory_write_byte(uint16_t address, uint8_t data) {
    cpu.write_burst++;
    int i; uint16_t tmp;

    /* DMA 传输：$4014 是"写即触发"，必须走特殊路径，不能进查表 */
    if (address == 0x4014) {
        cpu.dma = true;
        for (i = 0; i < 256; i++) {
            tmp = (0x100 * data) + i;
            if ((tmp >> 13) == 0) {
                ppu_sprram_write(interal_ram[tmp % 0x0800]);
            } else {
                ppu_sprram_write(cart.mapper->cpu_read(cart.mapper, tmp));
            }
        }
        return;
    }

#if BUS_USE_LUT
    bus_write_table[address >> 13](address, data);
#else
    switch (address >> 13) {
        case 0:                        // 0000 ~ 1FFF, 内部 RAM
            interal_ram[address & 0x07FF] = data;
            break;
        case 1:                        // 2000 ~ 3FFF, PPU 寄存器
            ppu_register_write(address, data);
            break;
        case 2:                        // 4000 ~ 5FFF, APU 与 IO 寄存器
            io_write(address, data);
            break;
        case 3:                        // 6000 ~ 7FFF, Save RAM
            if (!(cart.mapper->prg_ram_protect & 0x40)) {
                save_ram[address - 0x6000] = data;
            }
            break;
        default:                       // 8000 ~ FFFF, PRG ROM
            cart.mapper->register_write(cart.mapper, address, data);
            break;
    }
#endif
}

static inline void memory_write_word(uint16_t address, uint16_t data) {
    memory_write_byte(address + 1, data >> 8);
}
