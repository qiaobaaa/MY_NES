#include "ppu.h"
#include "ppu_lut_table.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

uint8_t ppu_screen_background[264][248];
/* Precalculated tile high and low bytes addition for pattern tables */
//uint8_t ppu_l_h_addition_table[256][256][8];
//uint8_t ppu_l_h_addition_flip_table[256][256][8];

//uint8_t (*ppu_l_h_addition_table)[256][8];
//uint8_t (*ppu_l_h_addition_flip_table)[256][8];

bool ppu_sprite_hit_occured = false;
uint8_t ppu_latch;
bool ppu_2007_first_read;
uint8_t ppu_addr_latch;

typedef struct sprite{
    uint8_t idx[8];  //精灵的每个像素颜色索引
    uint8_t x;       //精灵的起始x坐标
    uint8_t id;      //实际是第几个精灵
}SPRITE;

SPRITE sprite_tem[8];
uint8_t sprite_num = 0; //这一行显示几个精灵
uint8_t sprite_scanline = 0; //精灵实际在第几行显示；

uint8_t ppu_register_read(uint16_t address)
{
    uint8_t data = 0;
    switch (address & 7) {
    case 2:
        data = ppu.status.ppu_status;
        //ppu.status.STATUS_V = 0;  //Vblank flag, cleared on read. Unreliable; see below.
        ppu.status.ppu_status &= ~(1U << 7);
        ppu.w = 0; //读取ppustatus会重置w内部寄存器
        return data;
    case 4:
        return ppu.sprite_ram[ppu.oamaddr];
    case 7:
        {
        //正常模式
        uint16_t v_addr = ppu.v.reg & (uint16_t)0x3FFF;
        if (v_addr < 0x3f00) {
            data = ppu.read_buffer;
            //ppu_ram_read暂未实现，
            //ppu.read_buffer = ppu_ram_read(ppu.ppuaddr);
        }
        //调色板$3F00 - 3FFF之间,暂未实现
        /*
        data = read_palette(v_addr);                // 调色板：直接返回调色板数据，不是read_buffer
    uint16_t mirror = v_addr ^ 0x1000;
    ppu.read_buffer = ppu_ram_read(mirror);     // 偷偷读镜像地址更新read_buffer
    非调色板区域：`data = read_buffer`，然后 `read_buffer = ppu_ram_read(v_addr)`
调色板区域：`data = 调色板值`，然后 `read_buffer = ppu_ram_read(v_addr ^ 0x1000)`
不要把这两个逻辑写混，这是 PPU 读 $2007 最经典的隐性坑。
        */
        else {
            
        }
        ppu.v.reg += (uint16_t)((ppu.ctrl.CTRL_I) ? 32 : 1);
        return data;
        }
    default:
        return 0xff;
    }
}