#pragma once
#include <SD_MMC.h>
#include <stdint.h>
#include "cartridge.h"
#include "cpu.h"

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
extern uint16_t screen[256 * 240];
//extern uint16_t screen[260 * 260];

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
void ppu_set_in_vblank(bool yesno);

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

#define RGB567(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

// PPUSTATUS Functions

bool ppu_sprite_overflow();
bool ppu_sprite_0_hit();
bool ppu_in_vblank();

void ppu_set_sprite_overflow(bool value);
void ppu_set_sprite_0_hit(bool value);
void ppu_set_in_vblank(bool value);

void ppu_draw_background_scanline(bool mirror);
void ppu_draw_sprite_scanline();

void pixbuf_add(uint16_t x, uint16_t y, uint8_t idx);
void pixbuf_addd(uint16_t x);

const uint16_t palette_rgb565[64] = {
	RGB567(0x80,0x80,0x80),
	RGB567(0x00,0x00,0xBB),
	RGB567(0x37,0x00,0xBF),
	RGB567(0x84,0x00,0xA6),
	RGB567(0xBB,0x00,0x6A),
	RGB567(0xB7,0x00,0x1E),
	RGB567(0xB3,0x00,0x00),
	RGB567(0x91,0x26,0x00),
	RGB567(0x7B,0x2B,0x00),
	RGB567(0x00,0x3E,0x00),
	RGB567(0x00,0x48,0x0D),
	RGB567(0x00,0x3C,0x22),
	RGB567(0x00,0x2F,0x66),
	RGB567(0x00,0x00,0x00),
	RGB567(0x05,0x05,0x05),
	RGB567(0x05,0x05,0x05),
	RGB567(0xC8,0xC8,0xC8),
	RGB567(0x00,0x59,0xFF),
	RGB567(0x44,0x3C,0xFF),
	RGB567(0xB7,0x33,0xCC),
	RGB567(0xFF,0x33,0xAA),
	RGB567(0xFF,0x37,0x5E),
	RGB567(0xFF,0x37,0x1A),
	RGB567(0xD5,0x4B,0x00),
	RGB567(0xC4,0x62,0x00),
	RGB567(0x3C,0x7B,0x00),
	RGB567(0x1E,0x84,0x15),
	RGB567(0x00,0x95,0x66),
	RGB567(0x00,0x84,0xC4),
	RGB567(0x11,0x11,0x11),
	RGB567(0x09,0x09,0x09),
	RGB567(0x09,0x09,0x09),
	RGB567(0xFF,0xFF,0xFF),
	RGB567(0x00,0x95,0xFF),
	RGB567(0x6F,0x84,0xFF),
	RGB567(0xD5,0x6F,0xFF),
	RGB567(0xFF,0x77,0xCC),
	RGB567(0xFF,0x6F,0x99),
	RGB567(0xFF,0x7B,0x59),
	RGB567(0xFF,0x91,0x5F),
	RGB567(0xFF,0xA2,0x33),
	RGB567(0xA6,0xBF,0x00),
	RGB567(0x51,0xD9,0x6A),
	RGB567(0x4D,0xD5,0xAE),
	RGB567(0x00,0xD9,0xFF),
	RGB567(0x66,0x66,0x66),
	RGB567(0x0D,0x0D,0x0D),
	RGB567(0x0D,0x0D,0x0D),
	RGB567(0xFF,0xFF,0xFF),
	RGB567(0x84,0xBF,0xFF),
	RGB567(0xBB,0xBB,0xFF),
	RGB567(0xD0,0xBB,0xFF),
	RGB567(0xFF,0xBF,0xEA),
	RGB567(0xFF,0xBF,0xCC),
	RGB567(0xFF,0xC4,0xB7),
	RGB567(0xFF,0xCC,0xAE),
	RGB567(0xFF,0xD9,0xA2),
	RGB567(0xCC,0xE1,0x99),
	RGB567(0xAE,0xEE,0xB7),
	RGB567(0xAA,0xF7,0xEE),
	RGB567(0xB3,0xEE,0xFF),
	RGB567(0xDD,0xDD,0xDD),
	RGB567(0x11,0x11,0x11),
	RGB567(0x11,0x11,0x11)
};
/* NES 64色调色板 - RGB565 格式 (uint16_t) 
static const uint16_t palette_rgb565[64] = {
    0x8410, // 0: 中灰色  
    0x001B, // 1: 深蓝色  
    0x201B, // 2: 紫蓝色  
    0x4412, // 3: 紫色  
    0x6A0B, // 4: 玫红色  
    0x7606, // 5: 深红色  
    0x7800, // 6: 纯红色  
    0x5C10, // 7: 橙红色  
    0x580C, // 8: 棕色  
    0x03E0, // 9: 深绿色  
    0x03E0, // 10: 深绿色  
    0x03D0, // 11: 墨绿色  
    0x02B0, // 12: 青蓝色  
    0x0000, // 13: 黑色  
    0x0000, // 14: 极深灰  
    0x0000, // 15: 极深灰  
    0xC618, // 16: 浅灰色  
    0x07FF, // 17: 亮蓝色  
    0x2B3F, // 18: 蓝色  
    0x6B3D, // 19: 紫色  
    0x7C2D, // 20: 亮紫色  
    0x7C1F, // 21: 亮红紫  
    0x7C15, // 22: 亮红  
    0x6420, // 23: 橙色  
    0x6020, // 24: 橙色  
    0x2B40, // 25: 黄绿色  
    0x2640, // 26: 绿色  
    0x0B60, // 27: 青绿色  
    0x0B58, // 28: 浅蓝色  
    0x0220, // 29: 深灰色  
    0x0110, // 30: 深灰色  
    0x0110, // 31: 深灰色  
    0xFFFF, // 32: 白色  
    0x07EF, // 33: 亮青色  
    0x3B6F, // 34: 浅蓝色  
    0x5B6F, // 35: 浅紫色  
    0x7B5D, // 36: 粉紫色  
    0x7B55, // 37: 粉红色  
    0x7B4B, // 38: 亮红色  
    0x7B43, // 39: 亮红  
    0x7B3D, // 40: 橙红色  
    0x6B50, // 41: 黄色  
    0x4D80, // 42: 亮绿色  
    0x4B90, // 43: 浅绿色  
    0x07FF, // 44: 亮青色  
    0x8888, // 45: 灰色  
    0x0220, // 46: 深灰色  
    0x0220, // 47: 深灰色  
    0xFFFF, // 48: 白色  
    0x4B7F, // 49: 浅蓝色  
    0x6B7F, // 50: 浅蓝色  
    0x7B7F, // 51: 浅紫色  
    0x7BBE, // 52: 亮粉色  
    0x7BBA, // 53: 亮粉色  
    0x7B9E, // 54: 浅红  
    0x7B96, // 55: 浅红  
    0x7B8C, // 56: 浅橙  
    0x6B90, // 57: 浅黄  
    0x5B90, // 58: 浅黄绿  
    0x4B9E, // 59: 浅绿  
    0x3BBE, // 60: 浅青  
    0xCCCD, // 61: 亮灰色  
    0x0220, // 62: 深灰色  
    0x0220  // 63: 深灰色  
};

//rgb888 -> rgb565有两种方式
//1.比例缩放法
//RGB565 将 16 位内存分布如下：
//红色(R)：5 位(0 - 31) → 转换公式：(r * 31) / 255
//绿色(G)：6 位(0 - 63) → 转换公式：(g * 63) / 255
//蓝色(B)：5 位(0 - 31) → 转换公式：(b * 31) / 255
//最终拼接公式：
//uint16_t rgb565 = (R5 << 11) | (G6 << 5) | B5;

//2.位移截断法
//r5 = r >> 3;    // 8位 -> 5位
//g6 = g >> 2;    // 8位 -> 6位
//b5 = b >> 3;    // 8位 -> 5位
//rgb565 = (r5 << 11) | (g6 << 5) | b5;

 NES 调色板 RGB565 格式 - uint16_t
static const uint16_t palette_rgb565[64] = {
    0xA5A5,  // 0:  浅灰 - Light Gray
    0x0017,  // 1:  深蓝 - Dark Blue
    0x0817,  // 2:  蓝紫 - Blue-Violet  
    0x1014,  // 3:  紫色 - Purple
    0x180D,  // 4:  粉紫 - Pink-Purple
    0x1803,  // 5:  玫瑰红 - Rose Red
    0x1C00,  // 6:  深红 - Dark Red
    0x1207,  // 7:  棕色 - Brown
    0x0A05,  // 8:  橙棕 - Orange-Brown
    0x004C,  // 9:  墨绿 - Dark Green
    0x0061,  // 10: 森林绿 - Forest Green
    0x0078,  // 11: 青绿 - Cyan-Green
    0x0045,  // 12: 深青 - Dark Cyan
    0x0000,  // 13: 黑色 - Black
    0x0020,  // 14: 深灰 - Dark Gray
    0x0020,  // 15: 深灰 - Dark Gray (重复)

    0xE71C,  // 16: 浅灰 - Light Gray
    0x0A1F,  // 17: 亮蓝 - Bright Blue
    0x4C1F,  // 18: 蓝紫 - Blue-Violet
    0x8C39,  // 19: 紫红 - Purple-Red
    0x1C2D,  // 20: 品红 - Magenta
    0x186B,  // 21: 粉红 - Pink
    0x1863,  // 22: 橙红 - Orange-Red
    0x186B,  // 23: 橙红 - Orange-Red (重复)

    0x1CC0,  // 24: 橙黄 - Orange
    0x4C70,  // 25: 土黄 - Ochre
    0x2C50,  // 26: 橄榄绿 - Olive Green
    0x006A,  // 27: 青绿 - Cyan-Green
    0x0091,  // 28: 青色 - Cyan
    0x1122,  // 29: 中灰 - Medium Gray
    0x1122,  // 30: 中灰 - Medium Gray (重复)
    0x1122,  // 31: 中灰 - Medium Gray (重复)

    0xFFFF,  // 32: 白色 - White
    0x0ADF,  // 33: 天蓝 - Sky Blue
    0x5BDF,  // 34: 淡紫 - Lavender
    0x9B7F,  // 35: 浅紫 - Light Purple
    0x1FBF,  // 36: 粉紫 - Pink-Purple
    0x1F9F,  // 37: 洋红 - Magenta
    0x1F6D,  // 38: 珊瑚色 - Coral
    0x1F6F,  // 39: 杏色 - Apricot

    0x1FC9,  // 40: 金色 - Gold
    0x5F20,  // 41: 黄绿 - Yellow-Green
    0x33AE,  // 42: 青柠绿 - Lime Green
    0x2B55,  // 43: 薄荷绿 - Mint Green
    0x36DF,  // 44: 浅青 - Light Cyan
    0x6666,  // 45: 浅灰 - Light Gray
    0x6666,  // 46: 浅灰 - Light Gray (重复)
    0x6666,  // 47: 浅灰 - Light Gray (重复)

    0xFFFF,  // 48: 白色 - White (重复)
    0x561F,  // 49: 冰蓝 - Ice Blue
    0x761F,  // 50: 薰衣草 - Lavender
    0x9B1F,  // 51: 浅粉紫 - Light Pink-Purple
    0x1FDD,  // 52: 淡粉 - Light Pink
    0x1F99,  // 53: 浅玫瑰 - Light Rose
    0x1F6B,  // 54: 桃色 - Peach
    0x1F6D,  // 55: 杏色 - Apricot

    0x1FB5,  // 56: 杏黄 - Apricot Yellow
    0x5F53,  // 57: 柠檬绿 - Lemon Green
    0x5FBD,  // 58: 薄荷 - Mint
    0x56FF,  // 59: 浅青 - Light Cyan
    0xB3FF,  // 60: 极浅蓝 - Very Light Blue
    0xB3FF,  // 61: 极浅蓝 - Very Light Blue (重复)
    0xB3FF,  // 62: 极浅蓝 - Very Light Blue (重复)
    0xDDDD   // 63: 浅灰 - Light Gray
};
*/

/*
索引分组：
0 - 灰度基准色
1 - 8 - 低亮度组(蓝→绿)
9 - 15 - 暗色组(绿→黑→灰)
16 - 23 - 中亮度组(灰→粉→橙)
24 - 31 - 暖色组(橙→绿→中灰)
32 - 39 - 亮色组(白→粉→杏)
40 - 47 - 明亮组(金→青→浅灰)
48 - 55 - 高亮组(白→浅粉)
56 - 63 - 最亮组(杏黄→极浅蓝→浅灰)
*/

/*
为什么在“预定义数组”时选择比例缩放法？
在静态数组中，性能不再是考量因素（因为计算只在你的电脑上执行一次，结果直接写死在代码里），此时唯一的衡量标准就是：颜色还原度（Color Fidelity）。

1. 避免“颜色偏移” (Color Shift)
位移法 (>> 3) 本质上是粗暴地砍掉低 3 位。这会导致颜色整体产生微小的“向下漂移”。
    比例缩放法：尽可能地将0…255的连续区间均匀地分布在0…31之间。
    结果：它能更真实地还原原作者设计的颜色亮度。
2. 极值处理更自然
    比例缩放法在处理接近白色或接近黑色的过渡色时，比位移法更平滑。

// 使用浮点数或高精度整数运算进行一次性预计算
uint16_t r5 = (uint16_t)((r * 31.0f) / 255.0f + 0.5f); // +0.5f 是为了实现四舍五入
uint16_t g6 = (uint16_t)((g * 63.0f) / 255.0f + 0.5f);
uint16_t b5 = (uint16_t)((b * 31.0f) / 255.0f + 0.5f);
uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;

*/