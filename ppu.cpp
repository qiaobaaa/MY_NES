#include "ppu.h"
#include "ppu_lut_table.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

uint8_t ppu_screen_background[264][248];

typedef struct sprite {
  uint8_t idx[8];  //精灵的每个像素颜色索引
  uint8_t x;       //精灵的起始x坐标
  uint8_t id;      //实际是第几个精灵
  bool is_background;
} SPRITE;

SPRITE sprite_tem[8];
//uint8_t sprite_num = 0;       //这一行显示几个精灵
uint8_t sprite_scanline = 0;  //精灵实际在第几行显示；
//uint8_t ram[0x4000];  // PPU可以看见的16Kb内部 VRAM
//uint8_t sprite_ram[0x100];

//PPU CTRL .........

//Name Table首地址
static inline uint16_t ppu_base_nametable_address() {
  switch (ppu.ctrl.CTRL_N) {
    case 0: return 0x2000;
    case 1: return 0x2400;
    case 2: return 0x2800;
    case 3: return 0x2c00;
    default: return 0x2000;
  }
}

static inline uint16_t ppu_select_nametable_address(uint8_t id) {
  switch (id) {
    case 0: return 0x2000;
    case 1: return 0x2400;
    case 2: return 0x2800;
    case 3: return 0x2c00;
    default: return 0x2000;
  }
}

//VRAM地址增量，0->1, 1->32
uint8_t ppu_vram_address_increment() {
  return (ppu.ctrl.CTRL_I) ? 32 : 1;
}

//Sprite Pattern Table 首地址
uint16_t ppu_sprite_pattern_table_address() {
  return (ppu.ctrl.CTRL_S) ? 0x1000 : 0x0000;
}

//背景 Pattern Table 首地址
uint16_t ppu_background_pattern_table_address() {
  return (ppu.ctrl.CTRL_B) ? 0x1000 : 0x0000;
}

//Sprite 大小 0 -> 8*8, 1 -> 8*16
uint8_t ppu_sprite_height() {
  return (ppu.ctrl.CTRL_H) ? 16 : 8;
}

//发生 VBlank 时是否执行 NMI，0 -> Disable, 1 -> Enabled
bool ppu_generate_nmi() {
  return (ppu.ctrl.CTRL_V) ? true : false;
}

//PPU MASK .........

//背景切除, 0 -> 切除左边 8 个像素列, 1 -> 不切除
bool ppu_show_background_in_leftmost_8px() {
  return (ppu.mask.MASK_m) ? true : false;
}

//精灵切除，0 -> 切除左边8个像素列，1 -> 不切除
bool ppu_show_sprites_in_leftmost_8px() {
  return (ppu.mask.MASK_M) ? true : false;
}

//背景可见, 0 -> 不显示， 1 -> 显示
bool ppu_show_background() {
  return (ppu.mask.MASK_b) ? true : false;
}

//精灵可见，0 -> 不显示， 1 -> 显示
bool ppu_show_sprites() {
  return (ppu.mask.MASK_s) ? true : false;
}

//PPU STATUS .........

//Sprite Overflow: 同一个 Scanline 上出现超过 8 个 Sprites，该位会置一
bool ppu_sprite_overflow() {
  return (ppu.status.STATUS_O) ? 1 : 0;
}

//Hit flag
//当 Sprite 0 的非 0 像素与背景的非零像素重叠时置 1（Set when a nonzero pixel of sprite 0 overlaps a nonzero background pixel;）
//Cleared at dot 1 of the pre - render line
bool ppu_sprite_0_hit() {
  return (ppu.status.STATUS_S) ? 1 : 0;
}

//Vblank 标志, 0 -> 没有进行 Vblank, 1 -> 正在进行 Vblank
//Cleared after reading $2002 and at dot 1 of the pre-render line
bool ppu_in_vblank() {
  return (ppu.status.STATUS_V) ? 1 : 0;
}

//Set接口 .........
void ppu_set_sprite_overflow(bool val) {
  if (val) {
    ppu.status.ppu_status |= 0x20;
  } else {
    ppu.status.ppu_status &= ~0x20;
  }
}

void ppu_set_sprite_0_hit(bool val) {
  if (val) {
    ppu.status.ppu_status |= 0x40;
  } else {
    ppu.status.ppu_status &= ~0x40;
  }
}

void ppu_set_in_vblank(bool val) {
  if (val) {
    ppu.status.ppu_status |= 0x80;
  } else {
    ppu.status.ppu_status &= ~0x80;
  }
}

uint8_t ppu_ram_read(uint16_t address) {
  uint16_t addr = address & (uint16_t)0x3FFF;
  uint8_t data = 0;
  //pattern table区域，不存在镜像直接读取
  if (addr >= 0x0000 && addr <= 0x1FFF) {
    data = ppu.ram[addr];
  }  //位于镜像区，必需进行处理不然读写不统一
  else if (addr >= 0x2000 && addr <= 0x3EFF) {
    if (ppu.mirror_type == NES_MIRROR_VERTICAL) {
      // Vertical垂直镜像,2000和2004指向第一nametable0，2800和2c00指向第二个
      /*上下镜像
			NT0 == NT2
			NT1 == NT3
			*/
      /*
			if (addr >= 0x2000 && addr <= 0x23FF)
				data = ppu.ram[addr];
			else if (addr >= 0x2400 && addr <= 0x27FF)
				data = ppu.ram[addr];
			else if (addr >= 0x2800 && addr <= 0x2BFF)
				data = ppu.ram[addr - 0x800];
			else if (addr >= 0x2C00 && addr <= 0x2FFF)
				data = ppu.ram[addr - 0x800];
			*/
      // 0x2000 - 0x27FF 保持不变
      // 0x2800 - 0x2FFF 减去 0x0800
      // 这一行代码等同于你上面所有的 if 判断
      data = ppu.ram[addr & ~0x0800];

      // 如果地址在 0x2800 以上，就减去 0x800，否则保持不变
      //uint16_t phys_addr = (addr >= 0x2800) ? (addr - 0x800) : addr;
      //data = ppu.ram[phys_addr];
    } else if (ppu.mirror_type == NES_MIRROR_HORIZONTAL) {
      // Horizontal，水平镜像，也就是左右一样
      data = ppu.ram[addr & ~0x0400];
    } else if (ppu.mirror_type == NES_MIRROR_ONE_SCREEN) {
      // 单屏模式：全部映射到 NT0
      data = ppu.ram[(addr & 0x03FF) | 0x2000];
    }
  } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
    /* Palettes (调色板) 内存 */
    addr = 0x3f00 | (addr & 0x1f);  // 地址 $3F20-$3FFF 是地址 3F00-$3F1F 的镜像
    if (addr == 0x3f10 || addr == 0x3f14 || addr == 0x3f18 || addr == 0x3f1c) {
      // 地址 $3F10/$3F14/$3F18/$3F1C 是地址 $3F00/$3F04/$3F08/$3F0C 的镜像
      addr -= 0x10;
    }
    data = ppu.ram[addr];
  }
  return data;
}

void ppu_ram_write(uint16_t address, uint8_t data) {
  if(address == 0x2100)
  {
    printf("....%x   %d\n",address, data);
  }
  uint16_t addr = address & (uint16_t)0x3FFF;
  if(address == 0x2100)
  {
    printf("....%x   %d\n",address, data);
  }
  //pattern table区域，不存在镜像直接读取
  if (addr >= 0x0000 && addr <= 0x1FFF) {
    //ppu.ram[addr] = data;
  }  //位于镜像区，必需进行处理不然读写不统一
  else if (addr >= 0x2000 && addr <= 0x3EFF) {
    if (ppu.mirror_type == NES_MIRROR_VERTICAL) {
      // Vertical垂直镜像,2000和2004指向第一nametable0，2800和2c00指向第二个
      /*上下镜像
			NT0 == NT2
			NT1 == NT3
			*/
      /*
			if (addr >= 0x2000 && addr <= 0x23FF)
				data = ppu.ram[addr];
			else if (addr >= 0x2400 && addr <= 0x27FF)
				data = ppu.ram[addr];
			else if (addr >= 0x2800 && addr <= 0x2BFF)
				data = ppu.ram[addr - 0x800];
			else if (addr >= 0x2C00 && addr <= 0x2FFF)
				data = ppu.ram[addr - 0x800];
			*/
      // 0x2000 - 0x27FF 保持不变
      // 0x2800 - 0x2FFF 减去 0x0800
      // 这一行代码等同于你上面所有的 if 判断
      ppu.ram[addr & ~0x0800] = data;

      // 如果地址在 0x2800 以上，就减去 0x800，否则保持不变
      //uint16_t phys_addr = (addr >= 0x2800) ? (addr - 0x800) : addr;
      //data = ppu.ram[phys_addr];
    } else if (ppu.mirror_type == NES_MIRROR_HORIZONTAL) {
      // Horizontal，水平镜像，也就是左右一样
      ppu.ram[addr & ~0x0400] = data;
    } else if (ppu.mirror_type == NES_MIRROR_ONE_SCREEN) {
      // 单屏模式：全部映射到 NT0
      ppu.ram[(addr & 0x03FF) | 0x2000] = data;
    }
  } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
    /* Palettes (调色板) 内存 */
    addr = 0x3f00 | (addr & 0x1f);  // 地址 $3F20-$3FFF 是地址 3F00-$3F1F 的镜像
    if (addr == 0x3f10 || addr == 0x3f14 || addr == 0x3f18 || addr == 0x3f1c) {
      // 地址 $3F10/$3F14/$3F18/$3F1C 是地址 $3F00/$3F04/$3F08/$3F0C 的镜像
      addr -= 0x10;
    }
    ppu.ram[addr] = data;
  }
}

uint8_t ppu_register_read(uint16_t address) {
  uint8_t data = 0;
  switch (address & 7) {
    case 2:
      data = ppu.status.ppu_status;
      ppu.open_bus = data;
      ppu.status.ppu_status &= ~(1U << 7);
      ppu.w = 0;
      break;
    case 4:
      data = ppu.sprite_ram[ppu.oamaddr];
      ppu.open_bus = data;
      break;
    case 7:
      {
        uint16_t v_addr = ppu.v.reg & (uint16_t)0x3FFF;
        //请注意，读取缓冲区仅在 PPUDATA 读取时更新。它不受写入或其他 PPU 操作（如渲染）的影响，并且会无限期地保持其值，直到下一次读取。
        if (v_addr < 0x3F00)  //正常读取,因为 PPU 总线读取速度太慢，无法在 CPU 读取所需的时间内完成
        {
          data = ppu.read_buffer;
          ppu.read_buffer = ppu_ram_read(v_addr);
        } else  //进入调色板区域
        {
          printf("hahhahha\n");
          data = ppu_ram_read(v_addr);
          //ppu.read_buffer = ppu_ram_read((v_addr & 0x0FFF) | 0x2000);
          //ppu.read_buffer = ppu_ram_read(v_addr);
          //data = ppu.read_buffer;
        }
        //After access, the video memory address will increment by an amount determined by bit 2 of $2000.
        
        ppu.v.reg += (uint16_t)((ppu.ctrl.CTRL_I) ? 32 : 1);
        //ppu.v.reg = (ppu.v.reg + ((ppu.ctrl.CTRL_I) ? 32 : 1)) & (uint16_t)0x3FFF;
        ppu.open_bus = data;
        break;
      }
    default:
      data = ppu.open_bus;
      break;
  }
  return data;
}
void ppu_register_write(uint16_t address, uint8_t data) {
  ppu.open_bus = data;  //用于读取的尽可能还原
  switch (address & 7) {
    case 0:
      {
        uint8_t old_nmi_enable = ppu.ctrl.CTRL_V;
        uint8_t new_nmi_enable = data & 0x80;

        ppu.ctrl.ppu_ctrl = data;
        ppu.t.nametable = ppu.ctrl.CTRL_N;


        if (!old_nmi_enable && new_nmi_enable && (ppu.status.STATUS_V)) {
          //cpu.triggerNMI(); 这句不是直接执行而是执行完一个cpu指令，再进行中断处理
          /*
			  "渲染阶段触发 NMI" — 不可能立即触发的条件是 PPUSTATUS bit 7 = 1，而 PPUSTATUS bit 7 在 vblank 结束时由硬件自动清除。
			  vblank 期间:  PPUSTATUS bit 7 = 1  ← 唯一可能立即触发的窗口
              vblank 结束:  PPUSTATUS bit 7 → 0  （硬件自动）
              渲染阶段:     PPUSTATUS bit 7 = 0  ← 条件不满足，不会立即触发
            所以即使你在渲染阶段写 PPUCTRL 0→1，NMI 也只会等到下一次 vblank 开始才触发，不会"在渲染阶段触发"。
			立即触发行为可能导致 NMI 重入：若 NMI handler 执行期间 vblank flag 尚未被清除，再次使能 NMI 将触发嵌套中断。
			由于 NES 的 NMI 无法被屏蔽，重入会导致栈溢出和程序崩溃。
			这是该行为唯一具有实际破坏力的场景，因此 NMI handler 应首先读取 PPUSTATUS 以清除 vblank flag。

			本节描述的 NMI 立即触发行为是真实硬件的固有特性。严格还原该行为意味着：若某 ROM 因编码顺序错误触发 NMI 重入，模拟器将与真实硬件一样发生栈溢出。这是正确的还原，而非模拟器的缺陷。
            然而，部分模拟器选择省略此行为，以换取对极少数存在此 bug 的 ROM 的兼容性。读者应根据自身目标（硬件研究 vs 游戏兼容）决定是否实现。
            本模拟器（或本书推荐做法）选择完整实现，以忠实还原硬件行为。
			*/
      cpu.irq_nmi_delay = 1;
        }
        break;
      }
    case 1:
      ppu.mask.ppu_mask = data;
      break;
    case 3:
      ppu.oamaddr = data;
      break;
    case 4:
      ppu.sprite_ram[ppu.oamaddr++] = data;
      // OAM 只有 256 字节，确保地址回绕
      ppu.oamaddr &= 0xFF;
      break;
    case 5:
      if (ppu.w == 0) {
        ppu.t.coarse_x = (data & 0xF8) >> 3;
        ppu.fine_x = data & 0x7;
        ppu.w = 1;
      } else {
        ppu.t.fine_y = data & 0x7;
        ppu.t.coarse_y = (data & 0xF8) >> 3;
        //ppu.t.fine_y = data & 0x7;
        ppu.w = 0;
      }
      break;
    case 6:
      if (ppu.w == 0) {
        /*
			ppu.t.reg &= 0x40FF;  //把Z后面的6bit清空，严格按照文档只动6位
			ppu.t.reg |= (data & 0x3F) << 8;     //把data6个bit位的赋值给上面清空的
			ppu.t.reg &= 0x3FFF; //把z清空，保持其他不变，因为变得是14位，所以就是3FFF
			*/
        //这句话的意思是：保留低 8 位，高位全部清空（包括 Z 位），然后把新数据填进去。
        ppu.t.reg = (ppu.t.reg & 0x00FF) | ((uint16_t)(data & 0x3F) << 8);
        ppu.w = 1;
      } else {
        /*
        printf("==== 执行2006第二次写，覆盖v！t=%04X → v=%04X ====\n", ppu.t.reg, ppu.v.reg);
        ppu.t.reg = (ppu.t.reg & (uint16_t)0xFF00) | (uint16_t)data;
        if(ppu.t.reg == 0x2100){
        printf("%x \n", ppu.t.reg);}
        ppu.v.reg = ppu.t.reg;
        ppu.w = 0;
        */
        //printf("[1] 进入第二次2006写，修改前 t.reg = %04X\n", ppu.t.reg);

    ppu.t.reg = (ppu.t.reg & (uint16_t)0xFF00) | (uint16_t)data;
    //printf("[2] 合并data之后 t.reg = %04X\n", ppu.t.reg);

    ppu.v.reg = ppu.t.reg;
    //printf("[3] 赋值给v后 v.reg = %04X\n", ppu.v.reg);

    ppu.w = 0;
      }
      break;
    case 7:
    //if(ppu.v.reg == 0x2100)
     //  {
         //printf("v.reg @@@@ =  %x  %d\n", ppu.v.reg, data);
     //  }
     //printf("v.reg  begin  @@@@ =  %x  %d\n", ppu.v.reg, data);
      ppu_ram_write(ppu.v.reg, data);
      ppu.v.reg += (uint16_t)((ppu.ctrl.CTRL_I) ? 32 : 1);
     // ppu.v.reg += (uint16_t)((ppu.ctrl.CTRL_I) ? 32 : 1);
     // printf("v.reggg after   =   %x\n", ppu.v.reg);

      //ppu.v.reg = (ppu.v.reg + ((ppu.ctrl.CTRL_I) ? 32 : 1)) & (uint16_t)0x3FFF;
      break;
    default:
      printf("nes_write_ppu_register error %X %X\n", address, data);
      break;
  }
}
/*
void ppu_cycle() {
  // 这样更符合实际情况: if(!ppu.ready && cpu_clock() > 29658) { ppu.ready = true; }
  // http://wiki.nesdev.com/w/index.php/PPU_power_up_state
  //if (!ppu.ready && cpu_clock() > 1) { ppu.ready = true; }
  ppu.scanline++;
  if (ppu_show_background()) {
    ppu_draw_background_scanline();
  }
  if (ppu_show_sprites()) { ppu_draw_sprite_scanline(); }
  if (ppu.scanline == 241) {
    ppu_set_in_vblank(true);
    ppu_set_sprite_0_hit(false);
    cpu_interrupt();
  } else if (ppu.scanline == 262) {
    ppu.scanline = -1;
    //ppu_sprite_hit_occured = false;
    ppu.ppu_sprite_hit_occured = false;
    ppu_set_in_vblank(false);
    
    //emu_update_screen();
  }
}

void ppu_run(int cycles) {
  while (cycles-- > 0) {
    ppu_cycle();
  }
}
*/

void ppu_draw_background_scanline() {
  //uint8_t pixel_shift = 7 - ppu.fine_x;
  uint8_t p = 0;  //用来计时真的屏幕坐标，p范围一定是0-255，一定要初始化
  const uint8_t dx = ppu.v.coarse_x;
  const uint8_t dy = ppu.v.coarse_y;
  uint8_t nametable_id = ppu.v.nametable;

  const uint16_t tile_y_offset = dy << 5;  //一行当中有32个tile，<<5就相当于乘32

  uint8_t y_in_tile = ppu.v.fine_y;
  uint8_t x_in_tile = ppu.fine_x;
  //printf("nametable_id  = %d\n", nametable_id);
  uint8_t real_scanline = (dy << 3) + y_in_tile;  //真正在那一行，加法 + 的优先级高于左移 <<
  for (uint8_t tile_x = dx; tile_x < 32; tile_x++) {
    //int tile_index = ppu_ram_read(ppu_base_nametable_address(nametable_id) + tile_x + tile_y_offset);
    //int tile_index = ppu_ram_read(ppu_base_nametable_address() + tile_x + tile_y_offset);
    int tile_index = ppu_ram_read(ppu_select_nametable_address(nametable_id) + tile_x + tile_y_offset);
    if(tile_x == 0 && dy==8){
    //printf("tile_index = %d,   tile_x = %d,  tile_y_offset = %d   %x\n", tile_index,  tile_x, tile_y_offset,  ppu_select_nametable_address(nametable_id) + tile_x + tile_y_offset);
    //tile_index=36;
    }
    uint16_t tile_address = ppu_background_pattern_table_address() + 16 * tile_index;
    //printf("tile_address =  %x\n", tile_address);
    uint8_t l = ppu_ram_read(tile_address + y_in_tile);
    uint8_t h = ppu_ram_read(tile_address + y_in_tile + 8);

    for (uint8_t x = x_in_tile; x < 8; x++) {
      uint8_t color = ppu_l_h_addition_table[l][h][x];
      // Color 0 is transparent
      if (color != 0) {
        //因为1个属性表管16个tile，所以tile_x >>2也就是/4，(real_scanline >> 5) * 8就等于真实的行数/32再乘8
        //uint16_t attribute_address = (ppu_base_nametable_address(nametable_id) + 0x3C0 + (tile_x >> 2) + (real_scanline >> 5) * 8);
        //uint16_t attribute_address = (ppu_base_nametable_address() + 0x3C0 + (tile_x >> 2) + (real_scanline >> 5) * 8);
        uint16_t attribute_address = (ppu_select_nametable_address(nametable_id) + 0x3C0 + (tile_x >> 2) + (real_scanline >> 5) * 8);
        //一个nametable中的后64字节是属性表，其中1个字节管理16个tile
        //每个 4×4 tile 区域对应属性表 1 字节
        //bit7-6: 左上调色板
        //bit5-4: 右上调色板
        //bit3-2: 左下调色板
        //bit1-0: 右下调色板
        //用来确定上半部分还是下半部分
        /*
				value = (bottomright << 6) | (bottomleft << 4) | (topright << 2) | (topleft << 0)
				Or equivalently:

				7654 3210
				|||| ||++- Color bits 3-2 for top left quadrant of this byte
				|||| ++--- Color bits 3-2 for top right quadrant of this byte
				||++------ Color bits 3-2 for bottom left quadrant of this byte
				++-------- Color bits 3-2 for bottom right quadrant of this byte
				*/
        bool top = (real_scanline % 32) < 16;
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

        //因为每个调色板占四个字节，背景有四个调色板，精灵也有四个调色板，所以这里要<<2
        uint16_t palette_address = 0x3F00 + (palette_attribute << 2);
        int idx = ppu_ram_read(palette_address + color);
        //ppu_screen_background[(tile_x << 3) + x][ppu.scanline] = color;
        ppu_screen_background[p][real_scanline] = color;
        pixbuf_add(p, real_scanline, idx);  //添加到屏幕坐标，真实坐标x，y分别是p, real_scaline

      } else {
        int idx = ppu_ram_read(0x3f00);
        pixbuf_add(p, real_scanline, idx);  //添加到屏幕坐标，真实坐标x，y分别是p, real_scaline,添加背景色
      }
      p++;
    }
    //pixel_shift = 7;
    x_in_tile = 0;
  }

  x_in_tile = 8;
  nametable_id ^= 1;
  //注意这里一定要<= dx, 因为fine_x精细分割分割的就是这个瓦片
  for (uint8_t tile_x = 0; tile_x <= dx; tile_x++) {
    //int tile_index = ppu_ram_read(ppu_base_nametable_address(nametable_id) + tile_x + tile_y_offset);
    //int tile_index = ppu_ram_read(ppu_base_nametable_address() + tile_x + tile_y_offset);
    int tile_index = ppu_ram_read(ppu_select_nametable_address(nametable_id) + tile_x + tile_y_offset);
    uint16_t tile_address = ppu_background_pattern_table_address() + 16 * tile_index;

    uint8_t l = ppu_ram_read(tile_address + y_in_tile);
    uint8_t h = ppu_ram_read(tile_address + y_in_tile + 8);

    if (tile_x == dx) {
      if (ppu.fine_x) {
        //如果有精细分割，x_in_tile就 == fine_x
        x_in_tile = ppu.fine_x;
      } else {
        //如果没有的话循环就应该终止，因为上个循环已经把整个tile渲染了
        break;
      }
    }

    for (uint8_t x = 0; x < x_in_tile; x++) {
      uint8_t color = ppu_l_h_addition_table[l][h][x];
      // Color 0 is transparent
      if (color != 0) {
        //因为1个属性表管16个tile，所以tile_x >>2也就是/4，(real_scanline >> 5) * 8就等于真实的行数/32再乘8
        //uint16_t attribute_address = (ppu_base_nametable_address(nametable_id) + 0x3C0 + (tile_x >> 2) + (real_scanline >> 5) * 8);
        //uint16_t attribute_address = (ppu_base_nametable_address() + 0x3C0 + (tile_x >> 2) + (real_scanline >> 5) * 8);
        uint16_t attribute_address = (ppu_select_nametable_address(nametable_id) + 0x3C0 + (tile_x >> 2) + (real_scanline >> 5) * 8);
        //一个nametable中的后64字节是属性表，其中1个字节管理16个tile
        bool top = (real_scanline % 32) < 16;
        //用来确定是左边还是右边，但是left的判断有问题，应该是bool left = (tile_x % 4 < 2);
        bool left = (tile_x % 4 < 2);

        uint8_t palette_attribute = ppu_ram_read(attribute_address);

        if (!top) {
          palette_attribute >>= 4;
        }
        if (!left) {
          palette_attribute >>= 2;
        }
        palette_attribute &= 3;

        //因为每个调色板占四个字节，背景有四个调色板，精灵也有四个调色板，所以这里要<<2
        uint16_t palette_address = 0x3F00 + (palette_attribute << 2);
        int idx = ppu_ram_read(palette_address + color);
        //ppu_screen_background[(tile_x << 3) + x][ppu.scanline] = color;
        ppu_screen_background[p][real_scanline] = color;
        pixbuf_add(p, real_scanline, idx);  //添加到屏幕坐标，真实坐标x，y分别是p, real_scaline
      } else {
        int idx = ppu_ram_read(0x3f00);
        pixbuf_add(p, real_scanline, idx);  //添加到屏幕坐标，真实坐标x，y分别是p, real_scaline,添加背景色
      }
      p++;
    }
    //x_in_tile = 8;
  }
}

void prepare_sprite_scanline(uint8_t scanline) {
  //0行不会有精灵
  if(scanline == 0)
  {
    return;
  }
  uint8_t sprite_count = 0;
  //uint8_t sprite_size = ppu_sprite_height() ? 16 : 8;
  uint8_t sprite_size = ppu_sprite_height();
  //printf("sprite size = %d\n", sprite_size);
  int n = 0;
  int x = 0;
  int row = 0;
  uint16_t tile_address = 0;

  //uint8_t target_scanline = ppu.scanline + 1;
  uint8_t target_scanline = scanline;

  for (n = 0; n < 0x100; n += 4) {
    uint8_t sprite_x = ppu.sprite_ram[n + 3];
    uint8_t sprite_y = ppu.sprite_ram[n] + 1;  //加1是为了表示精灵的实际位置，如果不加的话后续还要做判断
    if(ppu.sprite_ram[n]>=239)
    {
      //printf("6666666666666      %x\n", ppu.sprite_ram[n]);
      continue;
    }
    // 2. 使用 16 位整数进行比较，防止 sprite_y + sprite_size 溢出 255
    if ((uint16_t)sprite_y > target_scanline || (uint16_t)target_scanline >= (uint16_t)sprite_y + sprite_size) {
      continue;
    }
    //int s_idx=0;
    //if(sprite_count <9) s_idx = sprite_count;
    int s_idx = sprite_count;
    sprite_count++;

    if (sprite_count > 8) {
      //ppu.status.STATUS_O = 1;  //精灵溢出位置一，并跳出循环，因为一行最多8个不会再多
      ppu_set_sprite_overflow(true);
      //continue;
      break;
    }

    //判断是否水平镜像或者垂直镜像
    bool vflip = ppu.sprite_ram[n + 2] & 0x80;  //垂直
    bool hflip = ppu.sprite_ram[n + 2] & 0x40;  //水平

    uint8_t dy = target_scanline - sprite_y;  // 计算相对偏移

    uint8_t palette_attribute = ppu.sprite_ram[n + 2] & 0x3;
    uint16_t palette_address = 0x3F10 + (palette_attribute << 2);

    sprite_num++;
    sprite_tem[s_idx].x = sprite_x;
    sprite_tem[s_idx].id = n / 4;
    if (sprite_size == 8) {
      tile_address = ppu_sprite_pattern_table_address() + 16 * ppu.sprite_ram[n + 1];
      //int y_in_tile = ppu.scanline & 0x7;
      row = vflip ? (7 - dy) : dy;  // 使用 dy 而不是 scanline & 0x7

      uint8_t l = ppu_ram_read(tile_address + row);
      uint8_t h = ppu_ram_read(tile_address + row + 8);

      //未开发完
      for (x = 0; x < 8; x++) {
        //确定使用颜色在调色板中的索引
        int color = hflip ? ppu_l_h_addition_flip_table[l][h][x] : ppu_l_h_addition_table[l][h][x];
        // Color 0 is transparent
        int screen_x = sprite_x + x;
        if (color != 0) {
          //int screen_x = sprite_x + x;
          int idx = ppu_ram_read(palette_address + color);

          if (ppu.sprite_ram[n + 2] & 0x20) {  // 位于背景之后
            sprite_tem[s_idx].idx[x] = idx;    //在背景后添为0
            sprite_tem[s_idx].is_background = true;
          } else {  // 位于背景之前
            sprite_tem[s_idx].idx[x] = idx;
            sprite_tem[s_idx].is_background = false;
          }
        } else {
          sprite_tem[s_idx].idx[x] = 65;  //表示颜色是0，所以索引用了-1
        }
      }
    } else  //16模式
    {
      //1.如果是16模式的话先确定现在的target_scaline到底在上半精灵还是下半精灵；
      //2.第一步知道在上下之后看有没有垂直翻转，如果有的话，翻转块，没有保持不变；
      //3.看有没有水平翻转有的话，需要把0-7显示变成7-0显示
      //4.记录对应位置的颜色，分两大类如果color==0表示显示背景色，非0的话还要判断优先级背景前后
      //printf("sprite size = %d\n", sprite_size);
      uint8_t base_tile = ppu.sprite_ram[n + 1];
      uint8_t final_tile;
      uint16_t sprite_pattern_table_address = (base_tile & 0x1) ? 0x1000 : 0x000;
      //base_tile = base_tile >> 1;
      base_tile = base_tile & 0xFE;

      if (dy < 8)  //上半区精灵
      {
        //final_tile = vflip ? (base_tile + 1) : base_tile;
        // 如果翻转，上半区显示的是第二个图块(Index+1)且行反转
        // 如果不翻转，上半区显示的是第一个图块(Index)且行正向
        final_tile = vflip ? (base_tile + 1) : base_tile;
        row = vflip ? (7 - dy) : dy;  // 注意：这里的 dy 在 0-7 之间
                                      // 修正：如果 vflip=true，上半区实际上是原图的最后 8 行，所以应该是 7-dy 吗？
                                      // 不，最简单的逻辑是：
                                      // vflip=false: dy(0-7) -> Tile0(0-7), dy(8-15) -> Tile1(0-7)
                                      // vflip=true:  dy(0-7) -> Tile1(7-0), dy(8-15) -> Tile0(7-0)
      } else                          //下半区精灵
      {
        final_tile = vflip ? base_tile : (base_tile + 1);
        row = vflip ? (7 - (dy - 8)) : (dy - 8);
      }

      //tile_address = ppu_sprite_pattern_table_address() + 16 * final_tile;
      tile_address = sprite_pattern_table_address + 16 * final_tile;
      uint8_t l = ppu_ram_read(tile_address + row);
      uint8_t h = ppu_ram_read(tile_address + row + 8);

      for (x = 0; x < 8; x++) {
        //确定使用颜色在调色板中的索引
        int color = hflip ? ppu_l_h_addition_flip_table[l][h][x] : ppu_l_h_addition_table[l][h][x];
        // Color 0 is transparent
        int screen_x = sprite_x + x;
        if (color != 0) {
          //int screen_x = sprite_x + x;
          int idx = ppu_ram_read(palette_address + color);

          if (ppu.sprite_ram[n + 2] & 0x20) {  // 位于背景之后
                                               //sprite_tem[s_idx].idx[x] = 66;     //在背景后添为0
            sprite_tem[s_idx].idx[x] = idx;    //在背景后添为0
            sprite_tem[s_idx].is_background = true;
          } else {  // 位于背景之前
                    //sprite_tem[s_idx].idx[x] = idx;
            sprite_tem[s_idx].idx[x] = idx;
            sprite_tem[s_idx].is_background = false;
          }
        } else {
          sprite_tem[s_idx].idx[x] = 65;  //表示颜色是0，所以索引用了-1
        }
      }
    }
  }
  sprite_scanline = target_scanline;
}

void ppu_draw_sprite_scanline(uint8_t scanline){
  //0行不会有精灵
  if(scanline == 0)
  {
    return;
  }
  //if (ppu_show_sprites() && ppu.scanline > 0 && ppu.scanline < 240 && ppu.scanline == sprite_scanline && sprite_num != 0) {
  if (ppu_show_sprites() && sprite_num != 0) {
    int background_id = ppu_ram_read(0x3f00);
	  uint16_t background_color = palette_rgb565[background_id];
	  uint16_t x = 0;
	  uint16_t y = 0;
    //for (int i = 0; i < sprite_num; i++)  //添加这一行要显示的精灵
    for (int i = sprite_num - 1; i >= 0; i--)
    {
      for (int j = 0; j < 8; j++) {
			  bool sprite_background = sprite_tem[i].is_background;
        uint8_t color_idx = sprite_tem[i].idx[j];
        x = sprite_tem[i].x + j;
			  y = scanline;
        if (color_idx != 65) {
          if (!sprite_background || screen[x + y * 256] == background_color) {
            pixbuf_add(x, y, color_idx);
          }
          /*
          if (!sprite_background) {
            pixbuf_add(x, y, color_idx);
          }
          else if(sprite_background && screen[x + y * 256] == background_color)
          {
            pixbuf_add(x, y, color_idx);
          }
          */
          //判断精灵0 hit触发, 一定是精灵0，而不是精灵1之类的，因为精灵1也可能在0号位置
          if ((ppu.mask.MASK_b || ppu.mask.MASK_s) && !ppu.status.STATUS_S && sprite_tem[i].id == 0 && ppu_screen_background[sprite_tem[i].x + j][ppu.scanline] != 0) {
            ppu_set_sprite_0_hit(true);
            //ppu_sprite_hit_occured = true;
          }
        }
      }
    }
    //sprite_num = 0; //重置每一行的精灵数量
  }

  sprite_num = 0;  //重置每一行的精灵数量
}

void ppu_sprram_write(uint8_t data) {
  ppu.sprite_ram[ppu.oamaddr++] = data;
}

void ppu_copy(uint16_t address, uint8_t* source, int length) {
  memcpy(&ppu.ram[address], source, length);
}

void ppu_init() {
  ppu.ctrl.ppu_ctrl = 0;
  ppu.mask.ppu_mask = 0;
  ppu.status.ppu_status = 0;
  ppu.oamaddr = 0;
  ppu.ppuscroll = 0;
  //ppu.ppuscroll_x = 0;
  //ppu.ppuscroll_y = 0;
  //ppu.ppuaddr = 0;
  ppu.v.reg = 0;
  ppu.t.reg = 0;
  ppu.status.ppu_status |= 0xa0;
  ppu.w=0;
  //ppu.ppudata = 0;
  //ppu_2007_first_read = true;

  //ppu.ready = false;
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
  /*
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
    */
}

void ppu_set_mirroring(uint8_t mirroring) {
  //NES_MIRROR_HORIZONTAL
  //NES_MIRROR_VERTICAL
  if(mirroring)
  {
    ppu.mirror_type = NES_MIRROR_VERTICAL;
  }
  else
  {
    ppu.mirror_type = NES_MIRROR_HORIZONTAL;
  }
  //ppu.mirroring = mirroring;
  //ppu.mirroring_xor = 0x400 << mirroring;
}

static inline void pixbuf_add(uint16_t x, uint16_t y, uint8_t idx) {
  //printf(" %u   %u  %d\n", x, y, palette_rgb565[idx]);
  //if(x>=0&&x<256&&y>=0&&y<240){
  screen[x + y * 256] = palette_rgb565[idx];
  //}
}

void fill_line(uint8_t scanline) {
  int idx = ppu_ram_read(0x3f00);
  for (int i = 0; i < 256; i++) {
    pixbuf_add(i, scanline, idx);
  }
}

void ppu_draw_background_scanlinee(bool mirror) {
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

        //ppu_screen_background[(tile_x << 3) + x][ppu.scanline] = color;
        int screen_x = (tile_x << 3) + x - ppu.ppuscroll_x + (mirror ? 256 : 0);
        //if (screen_x >= 256) screen_x = screen_x - 256;
        if (screen_x >= 0 && screen_x < 256 && ppu.scanline < 241) {
          ppu_screen_background[screen_x][ppu.scanline] = color;
          pixbuf_add(screen_x, ppu.scanline, idx);
        }
        //ppu_screen_background[(tile_x << 3) + x][ppu.scanline] = color;

        //pixbuf_add(bg, (tile_x << 3) + x - ppu.PPUSCROLL_X + (mirror ? 256 : 0), ppu.scanline + 1, idx);
        //pixbuf_add((tile_x << 3) + x - ppu.ppuscroll_x + (mirror ? 256 : 0), ppu.scanline + 1, idx);
        //if(ppu.scanline<240 && (tile_x << 3) + x - ppu.ppuscroll_x + (mirror ? 256 : 0) >=0)
        //{
        //ppu_screen_background[(tile_x << 3) + x - ppu.ppuscroll_x + (mirror ? 256 : 0)][ppu.scanline] = color;
        //pixbuf_add((tile_x << 3) + x - ppu.ppuscroll_x + (mirror ? 256 : 0), ppu.scanline+1, idx);
        //}
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
      } else {
        int id = ppu_ram_read(0x3f00);
        int screen_x = (tile_x << 3) + x - ppu.ppuscroll_x + (mirror ? 256 : 0);
        //if (screen_x >= 0 && screen_x < 256 && ppu.scanline < 241)
        //pixbuf_add((tile_x << 3) + x - ppu.ppuscroll_x + (mirror ? 256 : 0), ppu.scanline, id);
        if((ppu.scanline<240 && (tile_x << 3) + x - ppu.ppuscroll_x + (mirror ? 256 : 0) >=0))
        {
          pixbuf_add((tile_x << 3) + x - ppu.ppuscroll_x + (mirror ? 256 : 0), ppu.scanline, id);
        }
      }
    }
  }
}