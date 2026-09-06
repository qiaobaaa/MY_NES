#include "ppu.h"

uint8_t ppu_screen_background[264][248];
/* Precalculated tile high and low bytes addition for pattern tables */
uint8_t ppu_l_h_addition_table[256][256][8];
uint8_t ppu_l_h_addition_flip_table[256][256][8];

bool ppu_sprite_hit_occured = false;
uint8_t ppu_latch;
bool ppu_2007_first_read;
uint8_t ppu_addr_latch;

//PPU CTRL     ............

/* 第 0、1 位：Name Table 首地址
 * 00: 0x2000
 * 01: 0x2400
 * 10: 0x2800
 * 11: 0x2c00
 */
uint16_t ppu_base_nametable_address() {
    switch (ppu.ppuctrl & 0x3) {
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
uint8_t ppu_vram_address_increment() { return (ppu.ppuctrl & 0x04) ? 32 : 1; }

/* 第 3 位：Sprite Pattern Table 首地址
 * 0: VRAM 0x0000
 * 1: VRAM 0x1000
 */
uint16_t ppu_sprite_pattern_table_address() { return (ppu.ppuctrl & 0x08) ? 0x1000 : 0x0000; }

/* 第 4 位：背景 Pattern Table 首地址
 * 0: VRAM 0x0000
 * 1: VRAM 0x1000
 */
uint16_t ppu_background_pattern_table_address() { return (ppu.ppuctrl & 0x10) ? 0x1000 : 0x0000; }

/* 第 5 位：Sprite 大小
 * 0: 8x8
 * 1: 8x16
 */
uint8_t ppu_sprite_height() { return (ppu.ppuctrl & 0x20) ? 16 : 8; }

/* 第 7 位：发生 VBlank 时是否执行 NMI
 * 0: Disabled
 * 1: Enabled
 */
bool ppu_generate_nmi() { return (ppu.ppuctrl & 0x80) ? true : false; }

//PPU MASK           ............

/* 第 0 位：色彩模式
 * 0: 彩色模式
 * 1: 灰度模式
 */
bool ppu_render_grayscale() { return (ppu.ppumask & 0x01) ? true : false; }

/* 第 1 位：背景切除
 * 0: 切除左边 8 个像素列
 * 1: 不切除
 */
bool ppu_show_background_in_leftmost_8px() { return (ppu.ppumask & 0x02) ? true : false; }

/* 第 2 位：主角切除
 * 0: 切除左边 8 个像素列
 * 1: 不切除
 */
bool ppu_show_sprites_in_leftmost_8px() { return (ppu.ppumask & 0x04) ? true : false; }

/* 第 3 位：背景可见
 * 0: 不显示
 * 1: 显示
 */
bool ppu_show_background() { return (ppu.ppumask & 0x08) ? true : false; }

/* 第 4 位：主角可见
 * 0: 不显示
 * 1: 显示
 */
bool ppu_show_sprites() { return (ppu.ppumask & 0x10) ? true : false; }

/* 第 5 ~ 7 位：色彩增强 */
bool ppu_intensify_red() { return (ppu.ppumask & 0x20) ? true : false; }
bool ppu_intensify_green() { return (ppu.ppumask & 0x40) ? true : false; }
bool ppu_intensify_blue() { return (ppu.ppumask & 0x80) ? true : false; }

void ppu_set_render_grayscale(bool val) {
    if (val) { ppu.ppumask |= 0x01; }
    else { ppu.ppumask &= ~0x01; }
}

void ppu_set_show_background_in_leftmost_8px(bool val) {
    if (val) { ppu.ppumask |= 0x01; }
    else { ppu.ppumask &= ~0x01; }
}

void ppu_set_show_sprites_in_leftmost_8px(bool val) {
    if (val) { ppu.ppumask |= 0x01; }
    else { ppu.ppumask &= ~0x01; }
}

void ppu_set_show_background(bool val) {
    if (val) { ppu.ppumask |= 0x01; }
    else { ppu.ppumask &= ~0x01; }
}

void ppu_set_show_sprites(bool val) {
    if (val) { ppu.ppumask |= 0x01; }
    else { ppu.ppumask &= ~0x01; }
}

void ppu_set_intensify_red(bool val) {
    if (val) { ppu.ppumask |= 0x01; }
    else { ppu.ppumask &= ~0x01; }
}

void ppu_set_intensify_green(bool val) {
    if (val) { ppu.ppumask |= 0x01; }
    else { ppu.ppumask &= ~0x01; }
}

void ppu_set_intensify_blue(bool val) {
    if (val) { ppu.ppumask |= 0x01; }
    else { ppu.ppumask &= ~0x01; }
}

//PPUSTATUS          ............

/* 第 5 位：Sprite Overflow
 * 同一个 Scanline 上出现超过 8 个 Sprites，该位会置一
 */
bool ppu_sprite_overflow() { return (ppu.ppustatus & 0x20) ? 1 : 0; }

/* 第 6 位：Hit Flag
 * 当 Sprite 0 的非 0 像素与背景的非零像素重叠时置 1（Set when a nonzero pixel of sprite 0 overlaps a nonzero background pixel;）
 * cleared at dot 1 of the pre-render line
 */
bool ppu_sprite_0_hit() { return (ppu.ppustatus & 0x40) ? 1 : 0; }

/* 第 7 位：Vblank 标志
 * 0: 没有进行 Vblank
 * 1: 正在进行 Vblank
 * cleared after reading $2002 and at dot 1 of the pre-render line
 */
bool ppu_in_vblank() { return (ppu.ppustatus & 0x80) ? 1 : 0; }

void ppu_set_sprite_overflow(bool val) {
    if (val) { ppu.ppustatus |= 0x20; }
    else { ppu.ppustatus &= ~0x20; }
}

void ppu_set_sprite_0_hit(bool val) {
    if (val) { ppu.ppustatus |= 0x40; }
    else { ppu.ppustatus &= ~0x40; }
}

void ppu_set_in_vblank(bool val) {
    if (val) { ppu.ppustatus |= 0x80; }
    else { ppu.ppustatus &= ~0x80; }
}

uint16_t ppu_get_real_ram_address(uint16_t address) {
    if (address < 0x3f00) { return address; }
    else if (address < 0x4000) {
        /* Palettes (调色板) 内存 */
        address = 0x3f00 | (address & 0x1f); // 地址 $3F20-$3FFF 是地址 3F00-$3F1F 的镜像
        if (address == 0x3f10 || address == 0x3f14 || address == 0x3f18 || address == 0x3f1c) {
            // 地址 $3F10/$3F14/$3F18/$3F1C 是地址 $3F00/$3F04/$3F08/$3F0C 的镜像
            return address - 0x10;
        }
        else {
            return address;
        }
    }
    return 0xffff;  // 地址错误
}

uint8_t ppu_ram_read(uint16_t address) {
    return ppu.ram[ppu_get_real_ram_address(address)];
}

void ppu_ram_write(uint16_t address, uint8_t data) {
    ppu.ram[ppu_get_real_ram_address(address)] = data;
}

uint8_t ppu_io_read(uint16_t address) {
    uint8_t data; uint16_t value;
    ppu.ppuaddr &= 0x3fff;
    switch (address & 7) {
    case 2:
        value = ppu.ppustatus;
        ppu_set_in_vblank(false);
        ppu_set_sprite_0_hit(false);
        ppu.scroll_received_x = 0;
        ppu.ppuscroll = 0;
        ppu.addr_received_high_byte = 0;
        ppu_latch = value;
        ppu_addr_latch = 0;
        ppu_2007_first_read = true;
        return value;
    case 4:
        return ppu_latch = ppu.sprite_ram[ppu.oamaddr];
    case 7:
        if (ppu.ppuaddr < 0x3f00) {
            data = ppu_ram_read(ppu.ppuaddr);
            ppu_latch = 0;
        }
        else {
            data = ppu_ram_read(ppu.ppuaddr);
            ppu_latch = 0;
        }
        if (ppu_2007_first_read) {
            ppu_2007_first_read = false;
        }
        else {
            ppu.ppuaddr += ppu_vram_address_increment();
        }
        return data;
    default:
        return 0xff;
    }
}

void ppu_io_write(uint16_t address, uint8_t data) {
    address &= 7;
    ppu_latch = data;
    ppu.ppuaddr &= 0x3fff;
    switch (address) {
    case 0: if (ppu.ready) { ppu.ppuctrl = data; } break;
    case 1: if (ppu.ready) { ppu.ppumask = data; } break;
    case 3: ppu.oamaddr = data; break;
    case 4: ppu.sprite_ram[ppu.oamaddr++] = data; break;
    case 5:
        if (ppu.scroll_received_x) { ppu.ppuscroll_y = data; }
        else { ppu.ppuscroll_x = data; }
        ppu.scroll_received_x ^= 1;
        break;
    case 6:
        if (!ppu.ready) { return; }
        if (ppu.addr_received_high_byte) { ppu.ppuaddr = (ppu_addr_latch << 8) + data; }
        else { ppu_addr_latch = data; }
        ppu.addr_received_high_byte ^= 1;
        ppu_2007_first_read = true;
        break;
    case 7:
        if (ppu.ppuaddr > 0x1fff || ppu.ppuaddr < 0x4000) {
            ppu_ram_write(ppu.ppuaddr ^ ppu.mirroring_xor, data);
            ppu_ram_write(ppu.ppuaddr, data);
        }
        else {
            ppu_ram_write(ppu.ppuaddr, data);
        }
    }
    ppu_latch = data;
}

void ppu_init() {
    ppu.ppuctrl = 0; ppu.ppumask = 0; ppu.ppustatus = 0; ppu.oamaddr = 0;
    ppu.ppuscroll = 0; ppu.ppuscroll_x = 0; ppu.ppuscroll_y = 0; ppu.ppuaddr = 0;
    ppu.ppustatus |= 0xa0;
    ppu.ppudata = 0;
    ppu_2007_first_read = 0;

    ppu.ready = false;

    int l, h, x;
    for (h = 0; h < 0x100; h++) {
        for (l = 0; l < 0x100; l++) {
            for (x = 0; x < 8; x++) {
                ppu_l_h_addition_table[l][h][x] = (((h >> (7 - x)) & 1) << 1) | ((l >> (7 - x)) & 1);
                ppu_l_h_addition_flip_table[l][h][x] = (((h >> x) & 1) << 1) | ((l >> x) & 1);
            }
        }
    }
}

void ppu_sprram_write(uint8_t data) {
    ppu.sprite_ram[ppu.oamaddr++] = data;
}

void ppu_set_background_color(uint8_t color) {
    //set_bg_color(color);
}

void ppu_set_mirroring(uint8_t mirroring) {
    ppu.mirroring = mirroring;
    ppu.mirroring_xor = 0x400 << mirroring;
}

/*
void ppu_init() {
    ppu.ppuctrl = 0; ppu.ppumask = 0; ppu.ppustatus = 0; ppu.oamaddr = 0;
    ppu.ppuscroll = 0; ppu.ppuscroll_x = 0; ppu.ppuscroll_y = 0; ppu.ppuaddr = 0;
    ppu.ppustatus |= 0xa0;
    ppu.ppudata = 0;
    ppu_2007_first_read = 0;

    ppu.ready = false;

    int l, h, x;
    for (h = 0; h < 0x100; h++) {
        for (l = 0; l < 0x100; l++) {
            for (x = 0; x < 8; x++) {
                ppu_l_h_addition_table[l][h][x] = (((h >> (7 - x)) & 1) << 1) | ((l >> (7 - x)) & 1);
                ppu_l_h_addition_flip_table[l][h][x] = (((h >> x) & 1) << 1) | ((l >> x) & 1);
            }
        }
    }
}
*/
//PPU lifecycle

void ppu_run(int cycles)
{
    while (cycles-- > 0) {
        ppu_cycle();
    }
}

void ppu_cycle()
{
    if (!ppu.ready && cpu_clock() > 29658)
        ppu.ready = true;

    ppu.scanline++;
    if (ppu_shows_background()) {
        ppu_draw_background_scanline(false);
        ppu_draw_background_scanline(true);
    }

    if (ppu_shows_sprites()) ppu_draw_sprite_scanline();

    if (ppu.scanline == 241) {
        ppu_set_in_vblank(true);
        ppu_set_sprite_0_hit(false);
        cpu_interrupt();
    }
    else if (ppu.scanline == 262) {
        ppu.scanline = -1;
        ppu_sprite_hit_occured = false;
        ppu_set_in_vblank(false);
        fce_update_screen();
    }
}

void ppu_copy(uint16_t address, uint8_t* source, int length) {
    memcpy(&ppu.ram[address], source, length);
}