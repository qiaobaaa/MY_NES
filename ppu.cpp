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



void ppu_draw_background_scanline(bool mirror)
{
    int tile_x;
    //nes是256列像素，240行高，一个标准tile是8*8像素，所以这里就是32 = 256/8
    //但是这里需要注意的是精灵是有大精灵是由两个tile组成，但背景都是由一个个tile组成不存在特殊情况
    for (tile_x = ppu_shows_background_in_leftmost_8px() ? 0 : 1; tile_x < 32; tile_x++) {
        // Skipping off-screen pixels
        if (((tile_x << 3) - ppu.PPUSCROLL_X + (mirror ? 256 : 0)) > 256)
            continue;
        //ppu.scanline >>3 就等于 ppu.scanline / 8
        int tile_y = ppu.scanline >> 3;
        //tile_y << 5 就等于 tile_y *32，一行有32个tile
        //计算 tile 在 nametable 中的索引（tile_y * 32 + tile_x）
        //tile_index就是计算出来到底这个地方用的第几个tile
        int tile_index = ppu_ram_read(ppu_base_nametable_address() + tile_x + (tile_y << 5) + (mirror ? 0x400 : 0));
        //而一个tile的大小是16字节，所以这里读取tile的地址是初始地址 + 16 * tile_index
        //根据 tile_index 计算 pattern table 中的图形数据地址
        word tile_address = ppu_background_pattern_table_address() + 16 * tile_index;

        //y_in_tile: 当前扫描线在 tile 内的 Y 偏移 (0-7)  
        int y_in_tile = ppu.scanline & 0x7;
        //因为tile像素是先存了低位，再存了高位，低位大小是8字节，高位大小也是8字节
        //这里分别拿到了tile这一行的低位和高位数据，每一个像素占2bit，一个tile=128bit==16字节
        //NES PPU 中，每个 tile 的图形数据为 16 字节：
        //前 8 字节存放低位 (bit 0)
        //后 8 字节存放高位 (bit 1)
        byte l = ppu_ram_read(tile_address + y_in_tile);
        byte h = ppu_ram_read(tile_address + y_in_tile + 8);

        int x;
        for (x = 0; x < 8; x++) {
            //每个像素由这 2 bit 组成索引值 (0-3)，再结合调色板才能确定最终颜色
            byte color = ppu_l_h_addition_table[l][h][x];

            // Color 0 is transparent
            if (color != 0) {
                //每个 4×4 tile 区域对应属性表 1 字节
                //bit7-6: 左上调色板
                //bit5-4: 右上调色板
                //bit3-2: 左下调色板
                //bit1-0: 右下调色板
                word attribute_address = (ppu_base_nametable_address() + (mirror ? 0x400 : 0) + 0x3C0 + (tile_x >> 2) + (ppu.scanline >> 5) * 8);
                bool top = (ppu.scanline % 32) < 16;
                bool left = (tile_x % 4 < 2);

                //tile_x本来就是按一个tile算，所以这里要tile_x >>2 == tile_x /4, 而ppu.scanline是0 1 2 3 4 ，1一个字节列是四个tile，也就是32像素
                //ppu.scanline >> 5 = ppu.scanline/32 ,后面 * 8，是因为一行有32tile，一个字节行管四个tile，32/4 = 8
                // 列: tile_x >> 2 (每 4 个 tile 对应属性表 1 字节，32/4=8) 列偏移
                // 行: scanline >> 5 (每 32 像素对应属性表 1 行，256/32=8) 行偏移
                //用来确定上半部分还是下半部分
                //用来确定是左边还是右边，但是left的判断有问题，应该是bool left = (tile_x % 4 < 2);
                //因为每个调色板占四个字节，背景有四个调色板，精灵也有四个调色板，所以这里要<<2
                byte palette_attribute = ppu_ram_read(attribute_address);

                if (!top) {
                    palette_attribute >>= 4;
                }
                if (!left) {
                    palette_attribute >>= 2;
                }
                palette_attribute &= 3;

                word palette_address = 0x3F00 + (palette_attribute << 2);
                int idx = ppu_ram_read(palette_address + color);

                ppu_screen_background[(tile_x << 3) + x][ppu.scanline] = color;
                
                pixbuf_add(bg, (tile_x << 3) + x - ppu.PPUSCROLL_X + (mirror ? 256 : 0), ppu.scanline + 1, idx);

                // 2. 计算总移位量
                int shift = 0;
                if (top) {
                shift = left ? 6 : 4; // 左上移6, 右上移4
                }
                else {
                shift = left ? 2 : 0; // 左下移2, 右下移0
                }

                // 3. 一次性提取
                uint8_t palette_attribute = (ppu_ram_read(attribute_address) >> shift) & 3;
            }
        }
    }
}

void ppu_draw_sprite_scanline()
{
    int scanline_sprite_count = 0;
    int n;
    for (n = 0; n < 0x100; n += 4) {
        byte sprite_x = PPU_SPRRAM[n + 3];
        byte sprite_y = PPU_SPRRAM[n];

        // Skip if sprite not on scanline
        if (sprite_y > ppu.scanline || sprite_y + ppu_sprite_height() < ppu.scanline)
           continue;

        scanline_sprite_count++;

        // PPU can't render > 8 sprites
        if (scanline_sprite_count > 8) {
            ppu_set_sprite_overflow(true);
            // break;
        }

        bool vflip = PPU_SPRRAM[n + 2] & 0x80;
        bool hflip = PPU_SPRRAM[n + 2] & 0x40;

        word tile_address = ppu_sprite_pattern_table_address() + 16 * PPU_SPRRAM[n + 1];
        int y_in_tile = ppu.scanline & 0x7;
        byte l = ppu_ram_read(tile_address + (vflip ? (7 - y_in_tile) : y_in_tile));
        byte h = ppu_ram_read(tile_address + (vflip ? (7 - y_in_tile) : y_in_tile) + 8);

        byte palette_attribute = PPU_SPRRAM[n + 2] & 0x3;
        word palette_address = 0x3F10 + (palette_attribute << 2);
        int x;
        for (x = 0; x < 8; x++) {
            int color = hflip ? ppu_l_h_addition_flip_table[l][h][x] : ppu_l_h_addition_table[l][h][x];

            // Color 0 is transparent
            if (color != 0) {
                int screen_x = sprite_x + x;
                int idx = ppu_ram_read(palette_address + color);
                
                if (PPU_SPRRAM[n + 2] & 0x20) {
                    pixbuf_add(bbg, screen_x, sprite_y + y_in_tile + 1, idx);
                }
                else {
                    pixbuf_add(fg, screen_x, sprite_y + y_in_tile + 1, idx);
                }

                // Checking sprite 0 hit
                if (ppu_shows_background() && !ppu_sprite_hit_occured && n == 0 && ppu_screen_background[screen_x][sprite_y + y_in_tile] == color) {
                    ppu_set_sprite_0_hit(true);
                    ppu_sprite_hit_occured = true;
                }
            }
        }
    }
}




















