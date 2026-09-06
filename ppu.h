/*
#pragma once
#include <stdint.h>
#include "cartridge.h"
// ppu.h - PPU 知道 Cartridge（访问 VRAM）
typedef struct PPU {
  Cartridge* cart;  // 用于读取 CHR ROM

  uint8_t vram[2048];  // 内部 VRAM
  uint8_t palette[32];

  uint8_t ppu_oam[0x100];
  uint8_t ppu_ram[0x4000];
  // 寄存器
  //uint8_t ppuctrl, ppumask, ppustatus;
  //uint16_t v, t;
  //uint8_t x, w;

  // PPU 寄存器
  uint8_t ppuctrl;                   // 2000: PPU 控制寄存器，WRITE
  uint8_t ppumask;                   // 2001: PPU MASK 寄存器，WRITE
  uint8_t ppustatus;                 // 2002: PPU 状态寄存器，READ
  uint8_t oamaddr;                   // 2003: PPU OAM 地址，WRITE
  uint8_t oamdata;                   // 2004: PPU OAM 数据，READ/WRITE
  uint16_t ppuscroll;                // 2005: PPU 滚动位置寄存器，WRITE x2
  uint8_t ppuscroll_x, ppuscroll_y;  // 将 PPUSCROLL 寄存器分为 X 和 Y 两个方向
  uint16_t ppuaddr;                  // 2006: PPU 地址寄存器，WRITE x2
  uint16_t ppudata;                  // 2007: PPU 数据寄存器，READ/WRITE
  uint8_t oamdma;                    // 4104: OAM DMA 寄存器（高字节），WRITE

  bool scroll_received_x;
  bool addr_received_high_byte;
  bool ready;

  int mirroring, mirroring_xor;

  int x, scanline;
} PPU;

void ppu_init(PPU* ppu, Cartridge* cart);
void ppu_clock(PPU* ppu);
*/
#pragma once
#include <stdint.h>
#include "cartridge.h"
// ppu.h - PPU 知道 Cartridge（访问 VRAM）
typedef struct PPU {
	Cartridge* cart; // 用于读取 CHR ROM

	uint8_t ram[0x4000];   // 内部 VRAM
	uint8_t sprite_ram[0x100];
	
    /* PPU 寄存器 */
    uint8_t ppuctrl;     // 2000: PPU 控制寄存器，WRITE
    uint8_t ppumask;     // 2001: PPU MASK 寄存器，WRITE
    uint8_t ppustatus;   // 2002: PPU 状态寄存器，READ
    uint8_t oamaddr;     // 2003: PPU OAM 地址，WRITE
    uint8_t oamdata;     // 2004: PPU OAM 数据，READ/WRITE
    uint16_t ppuscroll;  // 2005: PPU 滚动位置寄存器，WRITE x2
    uint8_t ppuscroll_x, ppuscroll_y;  // 将 PPUSCROLL 寄存器分为 X 和 Y 两个方向
    uint16_t ppuaddr;    // 2006: PPU 地址寄存器，WRITE x2
    uint16_t ppudata;     // 2007: PPU 数据寄存器，READ/WRITE
    uint8_t oamdma;      // 4104: OAM DMA 寄存器（高字节），WRITE

    bool scroll_received_x;
    bool addr_received_high_byte;
    bool ready;

    int mirroring, mirroring_xor;

    int x, scanline;
} PPU;

extern PPU ppu;

//void ppu_init(PPU* ppu, Cartridge* cart);
//void ppu_clock(PPU* ppu);

void ppu_init();
//void ppu_finish();  @@@

uint8_t ppu_ram_read(uint16_t address);
void ppu_ram_write(uint16_t address, uint8_t data);
uint8_t ppu_io_read(uint16_t address);
void ppu_io_write(uint16_t address, uint8_t data);

bool ppu_generate_nmi();
//void ppu_set_generate_nmi(bool yesno); @@@

void ppu_set_mirroring(uint8_t mirroring);

void ppu_run(int cycles);
void ppu_cycle();
//int ppu_scanline();      @@@
//void ppu_set_scanline(int s);  @@@
void ppu_copy(uint16_t address, uint8_t* source, int length);
void ppu_sprram_write(uint8_t data);

// PPUCTRL
bool ppu_show_background();
bool ppu_show_sprites();
bool ppu_in_vblank();
void ppu_set_in_vblank(bool value);

//.............
// Draws current screen pixels in ppu_background_pixels & ppu_sprite_pixels and clears them
//void ppu_render_screen();    @@@
void ppu_set_background_color(uint8_t color);



// PPUCTRL Functions

uint16_t ppu_base_nametable_address();
uint8_t ppu_vram_address_increment();
uint16_t ppu_sprite_pattern_table_address();
uint16_t ppu_background_pattern_table_address();
//uint8_t ppu_sprite_width();   //没必要实现，没有意义，因为所有的大精灵都是上下的，没有左右叠加在一起的，因此不需要获取宽度
uint8_t ppu_sprite_height();
bool ppu_generate_nmi();



// PPUMASK Functions

bool ppu_render_grayscale();
bool ppu_show_background_in_leftmost_8px();
bool ppu_show_sprites_in_leftmost_8px();
bool ppu_intensify_red();
bool ppu_intensify_green();
bool ppu_intensify_blue();
void ppu_set_render_grayscale(bool value);
void ppu_set_show_background_in_leftmost_8px(bool value);
void ppu_set_show_sprites_in_leftmost_8px(bool value);
void ppu_set_show_background(bool value);
void ppu_set_show_sprites(bool value);
void ppu_set_intensify_red(bool value);
void ppu_set_intensify_green(bool value);
void ppu_set_intensify_blue(bool value);



// PPUSTATUS Functions

bool ppu_sprite_overflow();
bool ppu_sprite_0_hit();
bool ppu_in_vblank();

void ppu_set_sprite_overflow(bool value);
void ppu_set_sprite_0_hit(bool value);
void ppu_set_in_vblank(bool value);

