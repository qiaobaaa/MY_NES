#include "ppu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

uint8_t ppu_screen_background[264][248];
/* Precalculated tile high and low bytes addition for pattern tables */
//uint8_t ppu_l_h_addition_table[256][256][8];
//uint8_t ppu_l_h_addition_flip_table[256][256][8];

uint8_t (*ppu_l_h_addition_table)[256][8];
uint8_t (*ppu_l_h_addition_flip_table)[256][8];

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
    //小于3f00的直接返回，虽然 Nametables 之间有镜像，但那是 PPU 内部逻辑处理的，不在这个函数的职责内
    if (address < 0x3f00) { return address; }
    else if (address < 0x4000) {
        /* Palettes (调色板) 内存 */
        //将 $3F20-$3FFF 的访问全部重定向到 $3F00-$3F1F
        //address & 0x1f 截断 1f的区域
        //然后再叠加给0x3f00，这里就是做了一级镜像处理
        address = 0x3f00 | (address & 0x1f); // 地址 $3F20-$3FFF 是地址 3F00-$3F1F 的镜像
        if (address == 0x3f10 || address == 0x3f14 || address == 0x3f18 || address == 0x3f1c) {
            // 地址 $3F10/$3F14/$3F18/$3F1C 是地址 $3F00/$3F04/$3F08/$3F0C 的镜像
            //NES PPU 的调色板在物理上只有 32 字节（0‑31），
            //但在 $3F10、$3F14、$3F18、$3F1C 这四个位置会出现“重复”。
            //这四个地址对应的是 * *universal background color * *($3F00)
            //以及 * *每行的第二颜色 * *（$3F04、$3F08、$3F0C）在 sprite
            //调色板中的镜像（在真正的硬件里，它们指向同一块 RAM）。
            //因此我们把它们重新映射回原始位置：
            //$3F10->$3F00
            //$3F14->$3F04
            //$3F18->$3F08
            //$3F1C->$3F0C
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
    ppu_2007_first_read = true;

    ppu.ready = false;
    /*
    int l, h, x;
    for (h = 0; h < 0x100; h++) {
        for (l = 0; l < 0x100; l++) {
            for (x = 0; x < 8; x++) {
                ppu_l_h_addition_table[l][h][x] = (((h >> (7 - x)) & 1) << 1) | ((l >> (7 - x)) & 1);
                ppu_l_h_addition_flip_table[l][h][x] = (((h >> x) & 1) << 1) | ((l >> x) & 1);
            }
        }
    }
    */
    ppu_l_h_addition_table = (uint8_t (*)[256][8])heap_caps_malloc(256*256*8, MALLOC_CAP_SPIRAM);
    ppu_l_h_addition_flip_table = (uint8_t (*)[256][8])heap_caps_malloc(256*256*8, MALLOC_CAP_SPIRAM);
    if(!ppu_l_h_addition_table || !ppu_l_h_addition_flip_table){
        printf("pattern table malloc fail!\n");
        return;
    }

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
    if(ppu.scanline<241){
        //pixbuf_addd(ppu.scanline);
    if (ppu_show_background()) {
        //                                                                                                                               pixbuf_addd(ppu.scanline);
        ppu_draw_background_scanline(false);
        //ppu_draw_background_scanline(true);
    }

   if (ppu_show_sprites()) ppu_draw_sprite_scanline();
    }
    if (ppu.scanline == 241) {
        ppu_set_in_vblank(true);
        ppu_set_sprite_0_hit(false);
        cpu_interrupt();
    }
    else if (ppu.scanline == 262) {
        ppu.scanline = -1;
        ppu_sprite_hit_occured = false;
        ppu_set_in_vblank(false);
        //fce_update_screen();
    }
}

void ppu_copy(uint16_t address, uint8_t* source, int length) {
    memcpy(&ppu.ram[address], source, length);
}

//Rendering
// Rendering

void ppu_draw_background_scanline(bool mirror)
{
    int tile_x;
    //nes是256列像素，240行高，一个标准tile是8*8像素，所以这里就是32 = 256/8
    //但是这里需要注意的是精灵是有大精灵是由两个tile组成，但背景都是由一个个tile组成不存在特殊情况
    for (tile_x = ppu_show_background_in_leftmost_8px() ? 0 : 1; tile_x < 32; tile_x++) {
        // Skipping off-screen pixels
        if (((tile_x << 3) - ppu.ppuscroll_x + (mirror ? 256 : 0)) > 256)
            continue;

        // 通过 tile 在屏幕上的位置，获取 tile 在内存中的地址
        //ppu.scanline >>3 就等于 ppu.scanline / 8
        int tile_y = ppu.scanline >> 3;

        //tile_y << 5 就等于 tile_y *32，一行有32个tile
        // 计算 tile 在 nametable 中的索引（tile_y * 32 + tile_x）
        int tile_index = ppu_ram_read(ppu_base_nametable_address() + tile_x + (tile_y << 5) + (mirror ? 0x400 : 0));
        //tile_index就是计算出来到底这个地方用的第几个tile
        //而一个tile的大小是16字节，所以这里读取tile的地址是初始地址 + 16 * tile_index
        // 根据 tile_index 计算 pattern table 中的图形数据地址
        uint16_t tile_address = ppu_background_pattern_table_address() + 16 * tile_index;

        //y_in_tile计算出来是tile的具体哪一行
        // y_in_tile: 当前扫描线在 tile 内的 Y 偏移 (0-7)
        int y_in_tile = ppu.scanline & 0x7;
        //因为tile像素是先存了低位，再存了高位，低位大小是8字节，高位大小也是8字节
        //这里分别拿到了tile这一行的低位和高位数据，每一个像素占2bit，一个tile=128bit==16字节
        // NES PPU 中，每个 tile 的图形数据为 16 字节：
        // 前 8 字节存放低位 (bit 0)
        // 后 8 字节存放高位 (bit 1)
        // 每个像素由这 2 bit 组成索引值 (0-3)，再结合调色板才能确定最终颜色
        uint8_t l = ppu_ram_read(tile_address + y_in_tile);
        uint8_t h = ppu_ram_read(tile_address + y_in_tile + 8);

        int x;
        for (x = 0; x < 8; x++) {
            uint8_t color = ppu_l_h_addition_table[l][h][x];

            // Color 0 is transparent
            if (color != 0) {
                //一个nametable中的后64字节是属性表，其中1个字节管理16个tile
                //每个 4×4 tile 区域对应属性表 1 字节
                //bit7-6: 左上调色板
                //bit5-4: 右上调色板
                //bit3-2: 左下调色板
                //bit1-0: 右下调色板
                //tile_x本来就是按一个tile算，所以这里要tile_x >>2 == tile_x /4, 而ppu.scanline是0 1 2 3 4 ，1一个字节列是四个tile，也就是32像素
                //ppu.scanline >> 5 = ppu.scanline/32 ,后面 * 8，是因为一行有32tile，一个字节行管四个tile，32/4 = 8
                // 列: tile_x >> 2 (每 4 个 tile 对应属性表 1 字节，32/4=8)   列偏移
                // 行: scanline >> 5 (每 32 像素对应属性表 1 行，256/32=8)    行偏移
                uint16_t attribute_address = (ppu_base_nametable_address() + (mirror ? 0x400 : 0) + 0x3C0 + (tile_x >> 2) + (ppu.scanline >> 5) * 8);
                //用来确定上半部分还是下半部分
                bool top = (ppu.scanline % 32) < 16;
                //用来确定是左边还是右边，但是left的判断有问题，应该是bool left = (tile_x % 4 < 2);
                bool left = (tile_x % 4 < 2);

                uint8_t palette_attribute = ppu_ram_read(attribute_address);

                //top的是高位四位,因此下半部分就需要右移4位
                // 第一次移位：区分上下
                // top=true  → 保持 (左上/右上)
                // top=false → 右移4位 (左下/右下)
                
                if (!top) {
                    palette_attribute >>= 4;
                }
                //左边就是在高四位或者低四位的高两位，因此右边就需要右移两位
                // 第二次移位：区分左右
                // left=true  → 保持 (左上/左下)
                // left=false → 再右移2位 (右上/右下)
                if (!left) {
                    palette_attribute >>= 2;
                }
                palette_attribute &= 3;
                
                // 2. 计算总移位量  
                /*
                int shift = 0;
                if (top) {
                    shift = left ? 6 : 4; // 左上移6, 右上移4  
                }
                else {
                    shift = left ? 2 : 0; // 左下移2, 右下移0  
                }

                // 3. 一次性提取  
                palette_attribute = (ppu_ram_read(attribute_address) >> shift) & 3;
                */

                //因为每个调色板占四个字节，背景有四个调色板，精灵也有四个调色板，所以这里要<<2
               uint16_t palette_address = 0x3F00 + (palette_attribute << 2);
                int idx = ppu_ram_read(palette_address + color);

                ppu_screen_background[(tile_x << 3) + x][ppu.scanline] = color;

                //pixbuf_add(bg, (tile_x << 3) + x - ppu.PPUSCROLL_X + (mirror ? 256 : 0), ppu.scanline + 1, idx);
                pixbuf_add((tile_x << 3) + x - ppu.ppuscroll_x + (mirror ? 256 : 0), ppu.scanline + 1, idx);
                /*
                // 2. 计算总移位量  
                int shift = 0;
                if (top) {
                    shift = left ? 6 : 4; // 左上移6, 右上移4  
                }
                else {
                    shift = left ? 2 : 0; // 左下移2, 右下移0  
                }
                */
                // 3. 一次性提取  
                //uint8_t palette_attribute = (ppu_ram_read(attribute_address) >> shift) & 3;
            }
        }
    }
}

//OAM大小为256字节，总共存在64个精灵，每个精灵占4字节大小
//在 OAM 中的数据：每个精灵只存 坐标、瓦片号、属性，不直接存像素
//8×16 模式（通过 PPU 控制寄存器开启） → 精灵高度为 16 像素，仍然使用 4 字节的 OAM 条目，
//但它会占用 两个相邻的瓦片（每块 8×8），共 32 字节 的 Pattern 数据
//精灵中的4字节每字节代表的意义解析:
//0: y坐标，精灵左上角的垂直位置（扫描线号），0 表示位于屏幕最顶端。实际可显示的范围是 0 ~ 239
//1: tile索引，指向Pattern Table（CHR‑ROM / CHR‑RAM）中的哪个 8×8（或 8×16）瓦片，0 ~ 255（在 8×8 模式下）或 0 ~ 127（在 8×16 模式下）
//2: 属性,bit7=垂直翻转(0正常，1镜像)，bit6=水平翻转(0正常，1镜像)，bit5=优先级(0精灵在前，1背景在前) ，bit4未使用，bit3-0=调色板编号
//3: x坐标，精灵左上角位置，同0解释
void ppu_draw_sprite_scanline()
{
    int scanline_sprite_count = 0;
    int n;
    //遍历所有精灵，共64个
    for (n = 0; n < 0x100; n += 4) {
        uint8_t sprite_x = ppu.sprite_ram[n + 3];
        uint8_t sprite_y = ppu.sprite_ram[n];

        // Skip if sprite not on scanline
        // 跳过不在 Scanline 上的 Sprite
        if (sprite_y > ppu.scanline || sprite_y + ppu_sprite_height() < ppu.scanline)
            continue;

        scanline_sprite_count++;

        // PPU can't render > 8 sprites
        // 每行不能出现超过 8 个 Sprites
        if (scanline_sprite_count > 8) {
            ppu_set_sprite_overflow(true);
            // break;
        }
        //获取是否垂直或者水平翻转
        //vertical flip，垂直翻转，即上下颠倒
        bool vflip = ppu.sprite_ram[n + 2] & 0x80;
        //horizontal flip，水平翻转，即左右颠倒
        bool hflip = ppu.sprite_ram[n + 2] & 0x40;
        //获取精灵对应的tile地址
        uint16_t tile_address = ppu_sprite_pattern_table_address() + 16 * ppu.sprite_ram[n + 1];
        //确定目前的行，在tile y轴偏移量
        //确定目前扫描线在 8x8 tile内部的相对行号(0~7)
        int y_in_tile = ppu.scanline & 0x7;
        //确定tile的高字节和低字节，和背景里面tile存储方式一致
        uint8_t l = ppu_ram_read(tile_address + (vflip ? (7 - y_in_tile) : y_in_tile));
        uint8_t h = ppu_ram_read(tile_address + (vflip ? (7 - y_in_tile) : y_in_tile) + 8);
        //获取对应的调色板
        uint8_t palette_attribute = ppu.sprite_ram[n + 2] & 0x3;
        //一个调色板占四个字节，所以palette_attribute << 2
        uint16_t palette_address = 0x3F10 + (palette_attribute << 2);
        int x;
        for (x = 0; x < 8; x++) {
            //确定使用颜色在调色板中的索引
            int color = hflip ? ppu_l_h_addition_flip_table[l][h][x] : ppu_l_h_addition_table[l][h][x];

            // Color 0 is transparent
            if (color != 0) {
                int screen_x = sprite_x + x;
                int idx = ppu_ram_read(palette_address + color);

                if (ppu.sprite_ram[n + 2] & 0x20) {  // 位于背景之后
                    //pixbuf_add(bbg, screen_x, sprite_y + y_in_tile + 1, idx);
                    pixbuf_add(screen_x, sprite_y + y_in_tile + 1, idx);
                }
                else {                               // 位于背景之前
                    //pixbuf_add(fg, screen_x, sprite_y + y_in_tile + 1, idx);
                    pixbuf_add(screen_x, sprite_y + y_in_tile + 1, idx);
                }

                // Checking sprite 0 hit
                if (ppu_show_background() && !ppu_sprite_hit_occured && n == 0 && ppu_screen_background[screen_x][sprite_y + y_in_tile] == color) {
                    ppu_set_sprite_0_hit(true);
                    ppu_sprite_hit_occured = true;
                }
            }
        }
    }
}

void pixbuf_add(uint16_t x, uint16_t y, uint8_t idx)
{
    //printf(" %u   %u\n", x, y);
    screen[x + (y-1)*256 ] = palette_rgb565[idx];
}

void pixbuf_addd(uint16_t x)
{
    //printf(" %u   %u\n", x, y);
    int idx = ppu_ram_read(0x3f00);
    for(int i=0;i<256;i++)
    {
        screen[i + x*256 ] = palette_rgb565[idx];
    }
}
/*
对于sprite 0 hit的自己理解:
    Sprite 0 Hit（精灵 0 命中）是 NES PPU 中一个非常特殊且“古怪”的硬件特性。虽然它听起来像是一个碰撞检测功能，
但实际上它被 NES 开发者用作一种精确的时钟同步机制，用来解决 NES 最大的硬件缺陷之一：无法在扫描线中间精确地改变滚动位置（Scrolling）
后续演化出来的新的软件功能:
1. 修改 背景图块的偏移量 或者 调色板
2. 配合mapper实现特殊功能，比如通过 Mapper 瞬间切换整套图块数据
但是在最开始或者说主要功能还是进行滚动的管理

硬件定义:
在 PPU 渲染每一行扫描线时，硬件会持续监控一个特定的条件。如果以下条件同时满足，PPU 就会将一个标志位（Flag）在状态寄存器（$2002）中设置为 1：
    1. 它是“精灵 0”：只有 OAM 中索引为 0 的那个精灵（第一个精灵）能触发这个机制。
    2. 像素颜色匹配：精灵 0 的当前像素颜色 ≠0（非透明），且这个颜色与此时背景（Background）在该位置的颜色完全相同。
    3. 背景可见：背景层必须处于开启状态。
简单来说： 当“精灵 0”的一个非透明像素与“背景”的一个像素在屏幕上重叠且颜色一致时，硬件就会大喊一声：“命中（Hit）了！”

为什么需要这个东西--核心:画面撕裂
NES 的滚动是通过修改 PPU 的滚动寄存器（$2005）实现的。但是，如果你在扫描线渲染到一半时突然修改滚动值，会发生什么？
    结果： 屏幕上半部分使用旧的滚动值，下半部分使用新的滚动值。在分界线处会出现一个明显的横向断层（Tearing/撕裂）。
开发者想要的是： 在每一帧的同一行（比如在状态栏上方）精确地切换滚动值，从而实现流畅的地图滚动。

特点:
    1. 每行只检测一次
    2. 在真实的 NES 硬件中，$2002 寄存器的 Sprite 0 Hit 标志位在被 CPU 读取之后，或者在进入下一行扫描时，会被硬件自动清零

最核心的作用: 用精灵 0 作为“坐标标尺”，来告知 CPU 当前的渲染位置

实时性Hit发生在 160 行，修改也发生在 160 行的瞬间。 这种“即时响应”正是Sprite 0 Hit机制最强大的地方
*/

//1. 地址线(Address Bus) →“货架的编号”
//地址线决定了 PPU / CPU 能“看到”多少个内存单元。
//CPU(16位地址线)：2^16 = 65536个地址（即 64 KB）。这意味着 CPU 能够给 65536 个不同的内存位置编号。
//PPU(14位地址线)：2^14 = 16384个地址（即 16 KB）。这意味着 PPU 能够给 16, 384 个不同的内存位置编号。
//关键点：地址线不传输实际的数据，它只传输 “我要访问哪个位置” 的信号。
//2. 数据线(Data Bus) →“货架上货物的宽度”
//数据线决定了在选定某个地址后，一次能传输多少数据。
//NES 的数据线是 8 位的。
//这意味着无论地址线是多少位，一旦选定了某个地址，通过数据线传输的“包裹”大小固定为 1 个字节(8 bits)。
//所以，你的观察完全正确：虽然 CPU 能定位 64KB 的空间，但它每次“伸手”拿东西，一次只能拿 1 个字节。
