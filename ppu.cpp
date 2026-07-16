#include "ppu.h"
#include <stdio.h>

/* 显示 PPU 寄存器等信息 */
void ppu_debugger(PPU* ppu) {
    printf("PPU REGISTERS:\n");
    printf("PPUCTRL:   %x\n", ppu->ppuctrl);
    printf("PPUMASK:   %x\n", ppu->ppumask);
    printf("PPUSTATUS: %x\n", ppu->ppustatus);
    printf("OAMADDR:   %x\n", ppu->oamaddr);
    printf("OAMDATA:   %x\n", ppu->oamdata);
    printf("PPUSCROLL: %x\n", ppu->ppuscroll);
    printf(" (X: %x, Y: %x)\n", ppu->ppuscroll_x, ppu->ppuscroll_y);
    printf("PPUADDR:   %x\n", ppu->ppuaddr);
    printf("PPUDATA:   %x\n", ppu->ppudata);
    printf("\n");
    printf("PPU SCANLINE: %x\n\n", ppu->scanline);
}

/* 第 0、1 位：Name Table 首地址
 * 00: 0x2000
 * 01: 0x2400
 * 10: 0x2800
 * 11: 0x2c00
 */
uint16_t ppu_base_nametable_address(PPU* ppu) {
    switch(ppu->ppuctrl & 0x3) {
        case 0: return 0x2000;
        case 1: return 0x2400;
        case 2: return 0x2800;
        case 3: return 0x2c00;
        default: return 0x2000;
    }
}

/* 第 2 位：端口 0x2007 VRAM 地址增量
 * 0: 自动增 1
 * 1: 自动增 32
 */
uint8_t ppu_vram_address_increment(PPU* ppu) { 
  return (ppu->ppuctrl & 0x04) ? 32 : 1; 
}