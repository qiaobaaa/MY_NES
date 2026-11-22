#include "mapper_004.h"
#include <stdlib.h>

//R0 R1 R2 R3 R4 R5
static uint8_t* chr_ptr[6];
//R6 R7
static uint8_t* prg_ptr[2];
static uint8_t* prg_second_last_ptr;
static uint8_t* prg_last_ptr;

static uint8_t* chr_r0[2];
static uint8_t* chr_r1[2];

// ========== 函数实现 ==========

static uint8_t cpu_read(void* self, uint16_t addr) {
  
  Mapper004* m = (Mapper004*)self;
  
  uint8_t data = 0;
  uint8_t prg_model = m->P;
  if (addr >= 0x8000 && addr <= 0x9FFF){
    if (!prg_model){
      data = prg_ptr[0][addr - 0x8000];
    }
    else{
      data = prg_second_last_ptr[addr - 0x8000];
    }
  }
  else if (addr >= 0xA000 && addr <= 0xBFFF){
    data = prg_ptr[1][addr - 0xA000];
  }
  else if (addr >= 0xC000 && addr <= 0xDFFF)
  {
    if (!prg_model){
      data = prg_second_last_ptr[addr - 0xC000];
    }
    else{
      data = prg_ptr[0][addr - 0xC000];
    }
  }
  else if (addr >= 0xE000 && addr <= 0xFFFF){
    data = prg_last_ptr[addr - 0xE000];
  }

  return data;
  
  /*
  Mapper004* m = (Mapper004*)self;
  uint16_t base = addr & 0xE000;
  uint16_t offset = addr - base;

  switch (base) {
    case 0x8000:
      return m->P ? prg_second_last_ptr[offset] : prg_ptr[0][offset];
    case 0xA000:
      return prg_ptr[1][offset];
    case 0xC000:
      return m->P ? prg_ptr[0][offset] : prg_second_last_ptr[offset];
    case 0xE000:
      return prg_last_ptr[offset];
    default:
      return 0;
  }
  */
}

static bool cpu_write(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t data) {
  Mapper004* m = (Mapper004*)self;

  if (addr >= 0x8000) {
    if (m->base.chr_banks == 0) {
      *mapped_addr = addr & (m->base.prg_banks > 1 ? 0x7FFF : 0x3FFF);
      return true;
    }
  }
  return false;
}

static uint8_t ppu_read(void* self, uint16_t addr) {
  Mapper004* m = (Mapper004*)self;
  
  uint8_t chr_flag = m->C;
  if (addr >= 0x000 && addr <= 0x07FF){
    if(chr_flag){
      if (addr < 0x400){
        return chr_ptr[2][addr];
      }
      return chr_ptr[3][addr - 0x400];
    }
    else{
      //return chr_ptr[0][addr];
      if (addr < 0x400){
        //return chr_ptr[0][addr];
        return chr_r0[0][addr];
      }
      //return (chr_ptr[0] + 0x400)[addr-0x400];
      return chr_r0[1][addr-0x400];
    }
  }
  else if (addr >= 0x800 && addr <= 0x0FFF){
    if (chr_flag){
      if (addr < 0x0C00)
      {
        return chr_ptr[4][addr - 0x800];
      }
      return chr_ptr[5][addr - 0xC00];
    }
    else{
      //return chr_ptr[1][addr - 0x800];
      if (addr < 0x0C00)
      {
        //return chr_ptr[1][addr - 0x800];
        return chr_r1[0][addr - 0x800];
      }
      //return (chr_ptr[1] + 0x400)[addr-0xC00];
      return chr_r1[1][addr - 0xC00];
    }
  }
  else if (addr >= 0x1000 && addr <= 0x17FF){
    if (chr_flag){
      //return chr_ptr[0][addr - 0x1000];
      if (addr < 0x1400){
        //return chr_ptr[0][addr - 0x1000];
        return chr_r0[0][addr - 0x1000];
      }
      //return (chr_ptr[0] + 0x400)[addr-0x1400];
      return chr_r0[1][addr - 0x1400];
    }
    else{
      if (addr < 0x1400)
      {
        return chr_ptr[2][addr - 0x1000];
      }
      return chr_ptr[3][addr - 0x1400];
    }
  }
  else if (addr >= 0x1800 && addr <= 0x1FFF)
  {
    if (chr_flag){
      //return chr_ptr[1][addr - 0x1800];
      if (addr < 0x1C00){
        //return chr_ptr[1][addr - 0x1800];
        return chr_r1[0][addr - 0x1800];
      }
      //return (chr_ptr[1] + 0x400)[addr-0x1C00];
      return chr_r1[1][addr - 0x1C00];
    }
    else{
      if (addr < 0x1C00)
      {
        return chr_ptr[4][addr - 0x1800];
      }
      return chr_ptr[5][addr - 0x1C00];
    }
  }
  
  /*
  bool two_kb = ((addr >> 11) == 0) == (m->C == 0);  // 这个半区是不是 2KB 区
  if (two_kb) {
    return chr_ptr[(addr & 0x0800) >> 11][addr & 0x07FF];  // R0/R1
  }
  return chr_ptr[2 + ((addr & 0x0FFF) >> 10)][addr & 0x03FF];  // R2-R5
  */
}

static bool ppu_write(void* self, uint16_t addr, uint32_t* mapped_addr) {
  Mapper004* m = (Mapper004*)self;

  if (addr <= 0x1FFF && m->base.chr_banks == 0) {
    *mapped_addr = addr;
    return true;
  }
  return false;
}

static void reset(void* self) {
}

static MirrorMode mirror(void* self) {
  return MIRROR_HARDWARE;
}

static bool irq_state(void* self) {
  return false;
}

static void irq_clear(void* self) {
}

static void scanline(void* self) {
}

static void mapper_bank_select(Mapper004* m, uint8_t data) {
  m->bank_select = data;
  //0: two 2 KiB banks at $0000-$0FFF, four 1 KiB banks at $1000-$1FFF;
  //1: two 2 KiB banks at $1000-$1FFF, four 1 KiB banks at $0000-$0FFF)
}

static void bank_data(Mapper004* m, uint8_t data) {
  // 各粒度的 bank 数，直接从字节大小推，不依赖 header 单位约定
  uint16_t prg_8k = m->base.cart->prg_rom_size / PRG_8K;  // 8K PRG bank 数
  uint16_t chr_1k = m->base.cart->chr_rom_size ? m->base.cart->chr_rom_size / CHR_1K : 1;
  uint16_t chr_2k = m->base.cart->chr_rom_size ? m->base.cart->chr_rom_size / CHR_2K : 1;

  switch (m->RRR) {
    case 0:
    chr_r0[0] = cart.chr_rom + ((data & 0xFE) % chr_1k) * CHR_1K;
      chr_r0[1] = cart.chr_rom + ((data | 0x01) % chr_1k) * CHR_1K;
      // .chr_r0[0] =  cart.chr_rom + (data & 0xFE) * CHR_1K;
      //.chr_r0[1] =  cart.chr_rom + (data | 0x01) * CHR_1K;
      //chr_ptr[0] = cart.chr_rom + (data & 0xFE) * CHR_1K;
      //chr_ptr[0] = m->base.cart->chr_rom + ((data >> 1) % chr_2k) * CHR_2K;
      break;
    case 1:
if ( data == 0x00u && m->C == 0) {
  printf("6666666666666  mapper4 specis\n");
}
    chr_r1[0] = cart.chr_rom + ((data & 0xFE) % chr_1k) * CHR_1K;
      chr_r1[1] = cart.chr_rom + ((data | 0x01) % chr_1k) * CHR_1K;
      //.chr_r1[0] =  cart.chr_rom + (data & 0xFE) * CHR_1K;
      //.chr_r1[1] =  cart.chr_rom + (data | 0x01) * CHR_1K;
      //chr_ptr[1] = cart.chr_rom + (data & 0xFE) * CHR_1K;
      //chr_ptr[1] = m->base.cart->chr_rom + ((data >> 1) % chr_2k) * CHR_2K;
      break;
    case 2:
      chr_ptr[2] = cart.chr_rom + data * CHR_1K;
      //chr_ptr[2] = m->base.cart->chr_rom + (data % chr_1k) * CHR_1K;
      break;
    case 3:
      chr_ptr[3] = cart.chr_rom + data * CHR_1K;
      //chr_ptr[3] = m->base.cart->chr_rom + (data % chr_1k) * CHR_1K;
      break;
    case 4:
      chr_ptr[4] = cart.chr_rom + data * CHR_1K;
      //chr_ptr[4] = m->base.cart->chr_rom + (data % chr_1k) * CHR_1K;
      break;
    case 5:
      chr_ptr[5] = cart.chr_rom + data * CHR_1K;
      //chr_ptr[5] = m->base.cart->chr_rom + (data % chr_1k) * CHR_1K;
      break;
    case 6:
      //prg_ptr[0] = cart.prg_rom + (data & 0x3F) * PRG_8K;
      prg_ptr[0] = cart.prg_rom + (data % prg_8k) * PRG_8K;
      break;
    case 7:
      //prg_ptr[1] = cart.prg_rom + (data & 0x3F) * PRG_8K;
      prg_ptr[1] = cart.prg_rom + (data % prg_8k) * PRG_8K;
      break;
    default:
      break;
  }

  
}

/*
static void bank_data(Mapper004* m, uint8_t data) {
  uint16_t prg_8k = m->base.cart->prg_rom_size / PRG_8K;
  uint16_t chr_1k = m->base.cart->chr_rom_size ? m->base.cart->chr_rom_size / CHR_1K : 1;
  uint16_t chr_2k = m->base.cart->chr_rom_size ? m->base.cart->chr_rom_size / CHR_2K : 1;
  
  printf("..........  %d  %d %d\n",data, data>>1, data & 0x3F);

  switch (m->RRR) {
    case 0:
      chr_ptr[0] = cart.chr_rom + ((data >> 1) & (chr_2k - 1)) * CHR_2K; // 👈 关键！
      break;
    case 1:
      chr_ptr[1] = cart.chr_rom + ((data >> 1) & (chr_2k - 1)) * CHR_2K;
      break;
    case 2:
      chr_ptr[2] = cart.chr_rom + (data & (chr_1k - 1)) * CHR_1K;
      break;
    case 3:
      chr_ptr[3] = cart.chr_rom + (data & (chr_1k - 1)) * CHR_1K;
      break;
    case 4:
      chr_ptr[4] = cart.chr_rom + (data & (chr_1k - 1)) * CHR_1K;
      break;
    case 5:
      chr_ptr[5] = cart.chr_rom + (data & (chr_1k - 1)) * CHR_1K;
      break;
    case 6:
      prg_ptr[0] = cart.prg_rom + (data & (prg_8k - 1)) * PRG_8K;
      break;
    case 7:
      prg_ptr[1] = cart.prg_rom + (data & (prg_8k - 1)) * PRG_8K;
      break;
  }
}
*/

static void nametable_change(uint8_t data) {
  //printf("mapper set mirror %d %d\n",ppu.mirror_type,(data & 0x01)^1);
  ppu_set_mirroring((data & 0x01)^1);
  //printf("[MIR] data=%d -> type=%d  (hi=%04X)\n", data, ppu.mirror_type, ppu.v.reg);
  //ppu_set_mirroring((data & 0x01));
}

static void prg_ram_protect(Mapper004* m, uint8_t data) {
  m->base.prg_ram_protect = data;
}

static void IRQ_latch(Mapper004* m, uint8_t data) {
  m->irq_latch = data;
}

static void IRQ_reload(Mapper004* m, uint8_t data) {
  m->irq_reload = 1;
}

static void IRQ_disable(Mapper004* m, uint8_t data) {
  m->irq_enable = 0;
}

static void IRQ_enable(Mapper004* m, uint8_t data) {
  m->irq_enable = 1;
}


static void register_write(void* self, uint16_t addr, uint8_t data) {
  Mapper004* m = (Mapper004*)self;
  uint8_t odd = addr & 0x01;

  if (0x8000 <= addr && addr <= 0x9FFF) {
    if (!odd) {
      mapper_bank_select(m, data);
    } else {
      bank_data(m, data);
    }
  } else if (0xA000 <= addr && addr <= 0xBFFF) {
    if (!odd) {
      nametable_change(data);
    } else {
      prg_ram_protect(m, data);
    }
  } else if (0xC000 <= addr && addr <= 0xDFFF) {
    if (!odd) {
      IRQ_latch(m, data);
    } else {
      IRQ_reload(m, data);
    }
  } else if (0xE000 <= addr && addr <= 0xFFFF) {
    if (!odd) {
      IRQ_disable(m, data);
      //IRQ_enable(m, data);
      cpu.irq_pending = 0;
    } else {
      IRQ_enable(m, data);
      //IRQ_disable(m, data);
      //cpu.irq_pending = 0;
    }
  }
}

static void irq_trigger(void* self) {
  //printf("66666666\n");
  Mapper004* m = (Mapper004*)self;
  //渲染关闭跳过
  if (!ppu.mask.MASK_s && !ppu.mask.MASK_b) return;

  if (m->irq_counter == 0 || m->irq_reload) {
    m->irq_counter = m->irq_latch;
    m->irq_reload = 0;
  } else {
    m->irq_counter--;
    //printf("66666666  %d\n", m->irq_counter);
    if (m->irq_counter == 0 && m->irq_enable) {
      //printf("666666666666  %d\n",cpu.I);
      cpu.irq_pending = 1;
    }
  }
    
  /*
  这是 MMC3 最核心的时序规则：IRQ 只在计数器"减到 0"时触发，
  "装填回 0"不触发——你自己的参考实现注释写的就是这个（nes_mapper4.c:258-261），
  参考的触发判断在 else 分支内（:276-278）。你的判断写在 if/else 外面，reload 路径也会走到。
  差别在 irq_latch == 0 时（正是你 create 的上电初值）：
  游戏若先 $E001 使能再写 latch，你的实现会每行都触发（每行 reload 到 0 就满足 counter==0 && enable），
  真实硬件则不触发（每行都装填、从未"减到 0"）。正常顺序的游戏感觉不到，但这是准确性 bug。改成：
  */
 //if (m->irq_counter ==0 && m->irq_enable){
   //cpu.irq_pending = 1;
 //}
  
}
// ========== vtable 定义（共享的） ==========

static const MapperVTable mapper_004_vtable = {
  .cpu_read = cpu_read,
  .cpu_write = cpu_write,
  .ppu_read = ppu_read,
  .ppu_write = ppu_write,
  .reset = reset,
  .mirror = mirror,
  .irq_state = irq_state,
  .irq_clear = irq_clear,
  .scanline = scanline,
  .register_write = register_write,
  .irq_trigger = irq_trigger,
};

// ========== 构造函数 ==========

Mapper004* mapper_004_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks) {
  Mapper004* m = (Mapper004*)malloc(sizeof(Mapper004));
  if (!m) return NULL;

  // 设置共享的 vtable
  m->base.vtable = &mapper_004_vtable;
  //m->base.cart = cart;
  m->base.prg_banks = prg_banks;
  m->base.chr_banks = chr_banks;
  m->base.cart = cart;
  printf("哈哈哈哈   %d    %d\n", cart->prg_rom_size, cart->prg_rom_size);
  //ppu_copy(0x0000, cart->chr_rom, 0x2000);
  //memcpy(cpu_prg_rom, cart->prg_rom, cart->prg_rom_size);
  //m->base.cart->prg_rom_size / PRG_8K;

  //初始化整个mapper4参数
  uint8_t banks = cart->prg_rom_size / PRG_8K;  //真正8k的数量
  prg_last_ptr = cart->prg_rom + (banks - 1) * PRG_8K;
  prg_second_last_ptr = cart->prg_rom + (banks - 2) * PRG_8K;

  //prg_ptr[0] = cart->prg_rom;                              // R6 = 0
  //prg_ptr[1] = cart->prg_rom + PRG_8K;                     // R7 = 1
  for (int i = 0; i < 6; i++) chr_ptr[i] = cart->chr_rom;  // R0-R5 = 0

  m->bank_select = 0;
  m->irq_counter = 0;
  m->irq_latch = 0;
  m->irq_reload = 0;
  m->irq_enable = 0;
  m->base.prg_ram_protect = 0;
  m->nametable_arrangement = 0;

  return m;
}

void mapper_004_destroy(Mapper004* mapper) {
  free(mapper);
}