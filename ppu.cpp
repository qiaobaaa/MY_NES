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
  switch (ppu->ppuctrl & 0x3) {
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



void ppu_draw_background_scanline(bool mirror) {
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
          shift = left ? 6 : 4;  // 左上移6, 右上移4
        } else {
          shift = left ? 2 : 0;  // 左下移2, 右下移0
        }

        // 3. 一次性提取
        uint8_t palette_attribute = (ppu_ram_read(attribute_address) >> shift) & 3;
      }
    }
  }
}

//OAM大小为256字节，总共存在64个精灵，每个精灵占4字节大小
//在 OAM 中的数据：每个精灵只存 坐标、瓦片号、属性，不直接存像素
//8×16 模式（通过 PPU 控制寄存器开启） → 精灵高度为 16 像素，仍然使用 4 字节的 OAM 条目，
//但它会占用 两个相邻的瓦片（每块 8×8），共 32 字节 的 Pattern 数据
//精灵中的4字节每字节代表的意义解析:
//0: y坐标，精灵左上角的垂直位置（扫描线号），0 表示位于屏幕最顶端。实际可显示的范围是 0 ~ 239
//1: tile索引，指向Pattern Table（CHR‑ROM / CHR‑RAM）中的哪个 8×8（或 8×16）瓦片，0 ~ 255（在 8×8 模式下）或 0 ~ 127（在 8×16 模式下）
//2: 属性,bit7=垂直翻转(0正常，1镜像)，bit6=水平翻转(0正常，1镜像)，bit5=优先级(0精灵在前，1背景在前) ，bit4未使用，bit3-0=调色板编号
//3: x坐标，精灵左上角位置，同0解释
void ppu_draw_sprite_scanline() {
  int scanline_sprite_count = 0;
  int n;
  //遍历所有精灵，共64个
  for (n = 0; n < 0x100; n += 4) {
    //拿去x, y坐标
    uint8_t sprite_x = ppu_sprram[n + 3];
    uint8_t sprite_y = ppu_sprram[n];

    /* 跳过不在 Scanline 上的 Sprite */
    if (sprite_y > ppu.scanline || sprite_y + ppu_sprite_height() < ppu.scanline) { continue; }

    scanline_sprite_count++;

    /* 每行不能出现超过 8 个 Sprites */
    if (scanline_sprite_count > 8) { ppu_set_sprite_overflow(true); }
    //获取是否垂直或者水平翻转
    //vertical flip，垂直翻转，即上下颠倒
    bool vflip = ppu_sprram[n + 2] & 0x80;
    //horizontal flip，水平翻转，即左右颠倒
    bool hflip = ppu_sprram[n + 2] & 0x40;
    //获取精灵对应的tile地址
    uint16_t tile_address = ppu_sprite_pattern_table_address() + 16 * ppu_sprram[n + 1];
    //确定目前的行，在tile y轴偏移量
    //确定目前扫描线在 8x8 tile内部的相对行号(0~7)
    int y_in_tile = ppu.scanline & 0x7;
    //确定tile的高字节和低字节，和背景里面tile存储方式一致
    uint8_t l = ppu_ram_read(tile_address + (vflip ? (7 - y_in_tile) : y_in_tile));
    uint8_t h = ppu_ram_read(tile_address + (vflip ? (7 - y_in_tile) : y_in_tile) + 8);
    //获取对应的调色板
    uint8_t palette_attribute = ppu_sprram[n + 2] & 0x3;
    //一个调色板占四个字节，所以palette_attribute << 2
    uint16_t palette_address = 0x3f10 + (palette_attribute << 2);
    int x;
    for (x = 0; x < 8; x++) {
      //确定使用颜色在调色板中的索引
      int color = hflip ? ppu_l_h_addition_flip_table[l][h][x] : ppu_l_h_addition_table[l][h][x];

      /* color 0 为透明 */
      if (color != 0) {
        //确定这个像素的x坐标
        int screen_x = sprite_x + x;
        int idx = ppu_ram_read(palette_address + color);

        // http://wiki.nesdev.com/w/index.php/PPU_sprite_priority
        if (ppu_sprram[n + 2] & 0x20) {  // 位于背景之后
          pixelbuf_add(bbg, screen_x, sprite_y + y_in_tile + 1, idx);
        } else {  // 位于背景之前
          pixelbuf_add(fg, screen_x, sprite_y + y_in_tile + 1, idx);
        }

        /* 检查是否发生 sprite 0 hit, 并更新寄存器 */
        if (ppu_show_background() && !ppu_sprite_hit_occured && n == 0 && ppu_screen_background[screen_x][sprite_y + y_in_tile] == color) {
          ppu_set_sprite_0_hit(true);
          ppu_sprite_hit_occured = true;
        }
      }
    }
  }
}


//PPUCTRL PPU控制寄存器..........
//bit0-bit1,name table的首地址
//00: 0x2000
//01: 0x2400
//10: 0x2800
//11: 0x2c00
uint16_t ppu_base_nametable_address() {
  switch (ppu.ppuctrl & 0x3) {
    case 0: return 0x2000;
    case 1: return 0x2400;
    case 2: return 0x2800;
    case 3: return 0x2c00;
    default: return 0x2000;
  }
}

//bit2，端口0x2007 VRAM地址增量
//0: 自动增1
//1: 自动增32
uint8_t ppu_vram_address_increment() {
  return (ppu.ppuctrl & 0x04) ? 32 : 1;
}

//bit3, Sprite Pattern Table 首地址
//0: VRAM 0x0000
//1: VRAM 0x1000
uint16_t ppu_sprite_pattern_table_address() {
  return (ppu.ppuctrl & 0x08) ? 0x1000 : 0x0000;
}

//bit4, 背景 Pattern Table 首地址
//0: VRAM 0x0000
//1: VRAM 0x1000
uint16_t ppu_background_pattern_table_address() {
  return (ppu.ppuctrl & 0x10) ? 0x1000 : 0x0000;
}

//bit5, Sprite大小
//0: 88
//1: 816
uint8_t ppu_sprite_height() {
  return (ppu.ppuctrl & 0x20) ? 16 : 8;
}

//bit6, PPU 主从模式选择，NES 中没有使用

//bit7, 发生 VBlank 时是否执行 NMI
//0: Disabled
//1: Enabled
bool ppu_generate_nmi() {
  return (ppu.ppuctrl & 0x80) ? true : false;
}


//PPU MASK
//bit0, 色彩模式
//0: 彩色模式
//1: 灰度模式
bool ppu_render_grayscale() {
  return (ppu.ppumask & 0x01) ? true : false;
}

//bit1, 背景切除
//0: 切除左边8个像素列
//1: 不切除
bool ppu_show_background_in_leftmost_8px() {
  return (ppu.ppumask & 0x02) ? true : false;
}

//bit2, 主角切除
//0: 切除左边8个像素列
//1: 不切除
bool ppu_show_sprites_in_leftmost_8px() {
  return (ppu.ppumask & 0x04) ? true : false;
}

//bit3, 背景可见
//0: 不显示
//1: 显示
bool ppu_show_background() {
  return (ppu.ppumask & 0x08) ? true : false;
}

//bit4，主角可见
//0: 不显示
//1: 显示
bool ppu_show_sprites() {
  return (ppu.ppumask & 0x10) ? true : false;
}

//bit5-7，色彩增强
bool ppu_intensify_red() {
  return (ppu.ppumask & 0x20) ? true : false;
}
bool ppu_intensify_green() {
  return (ppu.ppumask & 0x40) ? true : false;
}
bool ppu_intensify_blue() {
  return (ppu.ppumask & 0x80) ? true : false;
}

void ppu_set_render_grayscale(bool val) {
  if (val) {
    ppu.ppumask |= 0x01;
  } else {
    ppu.ppumask &= ~0x01;
  }
}

void ppu_set_show_background_in_leftmost_8px(bool val) {
  if (val) {
    ppu.ppumask |= 0x01;
  } else {
    ppu.ppumask &= ~0x01;
  }
}

void ppu_set_show_sprites_in_leftmost_8px(bool val) {
  if (val) {
    ppu.ppumask |= 0x01;
  } else {
    ppu.ppumask &= ~0x01;
  }
}

void ppu_set_show_background(bool val) {
  if (val) {
    ppu.ppumask |= 0x01;
  } else {
    ppu.ppumask &= ~0x01;
  }
}

void ppu_set_show_sprites(bool val) {
  if (val) {
    ppu.ppumask |= 0x01;
  } else {
    ppu.ppumask &= ~0x01;
  }
}

void ppu_set_intensify_red(bool val) {
  if (val) {
    ppu.ppumask |= 0x01;
  } else {
    ppu.ppumask &= ~0x01;
  }
}

void ppu_set_intensify_green(bool val) {
  if (val) {
    ppu.ppumask |= 0x01;
  } else {
    ppu.ppumask &= ~0x01;
  }
}

void ppu_set_intensify_blue(bool val) {
  if (val) {
    ppu.ppumask |= 0x01;
  } else {
    ppu.ppumask &= ~0x01;
  }
}

//PPU STATUS
//bit5，Sprite Overflow
//同一个 Scanline 上出现超过 8 个 Sprites，该位会置一
bool ppu_sprite_overflow() {
  return (ppu.ppustatus & 0x20) ? 1 : 0;
}

//bit6，Hit Flag
//当 Sprite 0 的非 0 像素与背景的非零像素重叠时置 1
//Set when a nonzero pixel of sprite 0 overlaps a nonzero background pixel
//cleared at dot 1 of the pre-render line
bool ppu_sprite_0_hit() {
  return (ppu.ppustatus & 0x40) ? 1 : 0;
}

//bit7，Vblank标志
//0: 没有进行 Vblank
//1: 正在进行 Vblank
//cleared after reading $2002 and at dot 1 of the pre-render line
bool ppu_in_vblank() {
  return (ppu.ppustatus & 0x80) ? 1 : 0;
}

void ppu_set_sprite_overflow(bool val) {
  if (val) {
    ppu.ppustatus |= 0x20;
  } else {
    ppu.ppustatus &= ~0x20;
  }
}

void ppu_set_sprite_0_hit(bool val) {
  if (val) {
    ppu.ppustatus |= 0x40;
  } else {
    ppu.ppustatus &= ~0x40;
  }
}

void ppu_set_in_vblank(bool val) {
  if (val) {
    ppu.ppustatus |= 0x80;
  } else {
    ppu.ppustatus &= ~0x80;
  }
}


uint16_t ppu_get_real_ram_address(uint16_t address) {
  //小于3f00的直接返回，虽然 Nametables 之间有镜像，但那是 PPU 内部逻辑处理的，不在这个函数的职责内
  if (address < 0x3f00) {
    return address;
  } else if (address < 0x4000) {
    /* Palettes (调色板) 内存 */
    //将 $3F20-$3FFF 的访问全部重定向到 $3F00-$3F1F
    //address & 0x1f 截断 1f的区域
    //然后再叠加给0x3f00，这里就是做了一级镜像处理
    address = 0x3f00 | (address & 0x1f);  // 地址 $3F20-$3FFF 是地址 3F00-$3F1F 的镜像
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
    } else {
      return address;
    }
  }
  return 0xffff;  // 地址错误
}

/* PPU 内存 */
//uint8_t ppu_sprram[0x100];
//uint8_t ppu_ram[0x4000];
uint8_t ppu_ram_read(uint16_t address) {
  return ppu_ram[ppu_get_real_ram_address(address)];
}

void ppu_ram_write(uint16_t address, uint8_t data) {
  ppu_ram[ppu_get_real_ram_address(address)] = data;
}
