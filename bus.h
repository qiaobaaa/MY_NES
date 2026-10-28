#pragma once
#include <stdint.h>
#include "ppu.h"
#include "io.h"

extern uint8_t* prg_rom_ptr;
extern int prg_rom_size;
extern uint8_t interal_ram[0x0800];
extern uint8_t save_ram[0x2000];


void memory_init(uint8_t* prg_rom, int prg_rom_length);
//uint8_t memory_read_byte(uint16_t address);
//uint16_t memory_read_word(uint16_t address);
//void memory_write_byte(uint16_t address, uint8_t data);
//void memory_write_word(uint16_t address, uint16_t data);

static inline uint8_t memory_read_byte(uint16_t address) {
    switch(address >> 13) {
        case 0:                        // 0000 ~ 1FFF, 内部 RAM
            //return interal_ram[address % 0x0800];
            return interal_ram[address & 0x07FF];
        case 1:                        // 2000 ~ 3FFF, PPU 寄存器
            //printf("111   %x\n", address);
            return ppu_register_read(address);
        case 2:                      // APU 与 IO 寄存器
            // cpu.a = 0x01;;
            return io_read(address);
            /*
            uint8_t ret_val = io_read(address);
            // 临时强制：只要读 0x4016，返回0x01
            if(address == 0x4016)
            {
                ret_val = 0x01;
            }
            return ret_val;}
            */
        case 3:                        // Save RAM
            return save_ram[address - 0x6000];
            //return interal_ram[address & 0x1FFF];
        default:                       // PRG ROM
            return prg_rom_ptr[(address - 0x8000) % prg_rom_size];
    }
}

static inline uint16_t memory_read_word(uint16_t address) {
    //printf("000   %x\n", address);
    return memory_read_byte(address) + (memory_read_byte(address + 1) << 8);
}

static inline void memory_write_byte(uint16_t address, uint8_t data) {
    int i; uint16_t tmp;
    /* DMA 传输 */
    if (address == 0x4014) {
        for(i = 0; i < 256; i++) {
            tmp = (0x100 * data) + i;
            if((tmp >> 13) == 0) {
                ppu_sprram_write(interal_ram[tmp % 0x0800]);
            } else {
                ppu_sprram_write(prg_rom_ptr[(address - 0x8000) % prg_rom_size]);
            }
        }
        return;
    }

    switch(address >> 13) {
        case 0:                        // 0000 ~ 1FFF, 内部 RAM
            //interal_ram[address % 0x0800] = data;
            interal_ram[address & 0x07FF] = data;
            break;
        case 1:                        // 2000 ~ 3FFF, PPU 寄存器
            //if(address == 0x2000)
            //{
                //data |= 0x20;
                //printf("FORCED 8x16 MODE: 0x%02X\n", data);
            //}
            //printf("789   %x\n", address);
            ppu_register_write(address, data);
            //if(address == 0x2000)
            //{
                //printf("@@@@@ %d\n",data);
            //}
            break;
        case 2:                        // APU 与 IO 寄存器
            io_write(address, data);
            break;
        case 3:                        // Save RAM
            save_ram[address - 0x6000] = data;
            //interal_ram[address & 0x1FFF] = data;
            break;
        default:                       // PRG ROM
            prg_rom_ptr[(address - 0x8000) % prg_rom_size] = data;
    }
}

static inline void memory_write_word(uint16_t address, uint16_t data) {
    //printf("456   %x\n", address);
    memory_write_byte(address + 1, data >> 8);
}