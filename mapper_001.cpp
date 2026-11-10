#include "mapper_001.h"
#include <stdlib.h>

int dirtyy = 0;

static bool cpu_read(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t* data) {
  Mapper001* m = (Mapper001*)self;

  if (addr >= 0x8000) {
    if (m->base.prg_banks > 1) {
      *mapped_addr = addr & 0x7FFF;
    } else {
      *mapped_addr = addr & 0x3FFF;
    }
    return true;
  }
  return false;
}

static bool cpu_write(void* self, uint16_t addr, uint32_t* mapped_addr, uint8_t data) {
  Mapper001* m = (Mapper001*)self;

  if (addr >= 0x8000) {
    if (m->base.chr_banks == 0) {
      *mapped_addr = addr & (m->base.prg_banks > 1 ? 0x7FFF : 0x3FFF);
      return true;
    }
  }
  return false;
}

static bool ppu_read(void* self, uint16_t addr, uint32_t* mapped_addr) {
  if (addr <= 0x1FFF) {
    *mapped_addr = addr;
    return true;
  }
  return false;
}

static bool ppu_write(void* self, uint16_t addr, uint32_t* mapped_addr) {
  Mapper001* m = (Mapper001*)self;

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

/*
static void apply_control(Mapper001* m) {
  if (m->M==0)
  {
    ppu_set_mirroring(2);
  }else if (m->M==1)
  {
    ppu_set_mirroring(3);
  }else if (m->M==2)
  {
    ppu_set_mirroring(1);
  }else if (m->M==3)
  {
    ppu_set_mirroring(0);
  }
  
  //因为已知一定是PP一定是3，也就是fix last bank at $C000 and switch 16 KiB bank at $8000
  uint32_t start = micros();
  memcpy(cpu_prg_rom, cart.prg_rom, 0x4000);
  start = micros() - start;
  printf("16kb memcpy %u\n", start);
  memcpy(cpu_prg_rom + 0x4000, cart.prg_rom + (m->base.prg_banks-1) * 0x4000, 0x4000);
}
*/
static void apply_control(Mapper001* m) {
  if (m->M==0)
  {
    ppu_set_mirroring(2);
  }else if (m->M==1)
  {
    ppu_set_mirroring(3);
  }else if (m->M==2)
  {
    ppu_set_mirroring(1);
  }else if (m->M==3)
  {
    ppu_set_mirroring(0);
  }
  
  //因为已知一定是PP一定是3，也就是fix last bank at $C000 and switch 16 KiB bank at $8000
 // uint32_t start = micros();
  //memcpy(cpu_prg_rom, cart.prg_rom, 0x4000);
  pprg_rom[0] = cart.prg_rom;
  //start = micros() - start;
  //printf("16kb memcpy %u %p %p  %p\n", start, pprg_rom[0], pprg_rom[1], cart.prg_rom);
  //memcpy(cpu_prg_rom + 0x4000, cart.prg_rom + (m->base.prg_banks-1) * 0x4000, 0x4000);
  pprg_rom[1] = cart.prg_rom + (m->base.prg_banks-1) * 0x4000;
 // printf("16kb memcpy %u %p %p  %p\n", start, pprg_rom[0], pprg_rom[1], cart.prg_rom);
}

static void control(Mapper001* m, uint8_t result) {
  m->control = result;
  if (m->M==0)
  {
    ppu_set_mirroring(2);
  }else if (m->M==1)
  {
    ppu_set_mirroring(3);
  }else if (m->M==2)
  {
    ppu_set_mirroring(1);
  }else if (m->M==3)
  {
    ppu_set_mirroring(0);
  }
}

/*
每取一次操作码就清零 → nes_cpu.c:1610 write_burst = 0;
每次 nes_write_cpu 就 +1 → nes_cpu.c:239 write_burst++;

唯一的例外：data & 0x80（bit7=1 的复位写）永不丢弃，
所以 nes_mapper1.c:218 的条件是 (data & 0x80) == 0 && write_burst > 1——两个条件同时成立才丢。
*/

static void chr_bank0(Mapper001* m, uint8_t result) {
  uint8_t chr_num = result & 0x0F;
  if (m->C)
  {
    memcpy(ppu.ram, cart.chr_rom + chr_num * 0x1000, 0x1000);
  }else
  {
    chr_num = chr_num >> 1;
    memcpy(ppu.ram, cart.chr_rom + chr_num * 0x2000, 0x2000);
  }
}

static void chr_bank1(Mapper001* m, uint8_t result) {
   if (m->C == 0)
   {
     return;
   }
   //Select 4 KiB CHR bank at PPU $1000 (ignored in 8 KiB mode)
   memcpy(ppu.ram + 0x1000, cart.chr_rom + result * 0x1000, 0x1000);
}

/*
static void prg_bank(Mapper001* m, uint8_t result) {
  
  uint8_t prg_num = result & 0x0F;
if(dirtyy == prg_num)
{
   return ;
}
dirtyy = prg_num;
  //交换整个32k prg, 放弃低位也就是bit0
  if (m->P == 0 || m->P == 1)
  {
    prg_num = prg_num >> 1;
    //uint32_t start = micros();
    memcpy(cpu_prg_rom, cart.prg_rom + 0x8000 * prg_num, 0x8000);
    //start = micros() - start;
    //printf("32kb memcpy %u\n", start);
  }else if(m->P == 2){
    //2: fix first bank at $8000 and switch 16 KiB bank at $C000
    uint32_t start = micros();
    memcpy(cpu_prg_rom, cart.prg_rom, 0x4000);
    //uint32_t start = micros();
    memcpy(cpu_prg_rom + 0x4000, cart.prg_rom + 0x4000 * prg_num, 0x4000);
    count+=2;
    start = micros() - start;
    m_time += start;
    //printf("......111   16kb memcpy %u\n", start);
  }else if(m->P == 3){
    //3: fix last bank at $C000 and switch 16 KiB bank at $8000
    uint32_t start = micros();
    memcpy(cpu_prg_rom, cart.prg_rom + 0x4000 * prg_num, 0x4000);
    //start = micros() - start;
    //printf("......222   16kb memcpy %u\n", start);
    memcpy(cpu_prg_rom + 0x4000, cart.prg_rom + (m->base.prg_banks-1) * 0x4000, 0x4000);
    count+=2;
    start = micros() - start;
    m_time += start;
  }
  
}
*/
static void prg_bank(Mapper001* m, uint8_t result) {
  
  uint8_t prg_num = result & 0x0F;

//dirtyy = prg_num;
  //交换整个32k prg, 放弃低位也就是bit0
  if (m->P == 0 || m->P == 1)
  {
    prg_num = prg_num >> 1;
    //uint32_t start = micros();
    //memcpy(cpu_prg_rom, cart.prg_rom + 0x8000 * prg_num, 0x8000);
    pprg_rom[0] = cart.prg_rom  + 0x8000 * prg_num;
    pprg_rom[1] = pprg_rom[0] + 0x4000;
    //start = micros() - start;
    //printf("32kb memcpy %u\n", start);
  }else if(m->P == 2){
    //2: fix first bank at $8000 and switch 16 KiB bank at $C000
    //uint32_t start = micros();
    //memcpy(cpu_prg_rom, cart.prg_rom, 0x4000);
    pprg_rom[0] = cart.prg_rom;
    //uint32_t start = micros();
    //memcpy(cpu_prg_rom + 0x4000, cart.prg_rom + 0x4000 * prg_num, 0x4000);
    pprg_rom[1] = cart.prg_rom + 0x4000 * prg_num;
    count+=2;
    //start = micros() - start;
    //m_time += start;
    //printf("......111   16kb memcpy %u\n", start);
  }else if(m->P == 3){
    //3: fix last bank at $C000 and switch 16 KiB bank at $8000
    //uint32_t start = micros();
    //memcpy(cpu_prg_rom, cart.prg_rom + 0x4000 * prg_num, 0x4000);
    pprg_rom[0] = cart.prg_rom + 0x4000 * prg_num;
    //start = micros() - start;
    //printf("......222   16kb memcpy %u\n", start);
    //memcpy(cpu_prg_rom + 0x4000, cart.prg_rom + (m->base.prg_banks-1) * 0x4000, 0x4000);
    pprg_rom[1] = cart.prg_rom + (m->base.prg_banks-1) * 0x4000;
    count+=2;
    //start = micros() - start;
    //m_time += start;
  }
  
}


static void register_write(void* self, uint16_t addr, uint8_t data) {
  Mapper001* m = (Mapper001*)self;

  //这个区间会重置移位寄存器，也就是bit7是1会进行重置  0x80~0xFF
  if (data & 0x80) {
    m->shift_register = SHIFT_FLAG;     //使用哨兵符合原始结构     
    m->control |= CONTROL_VALUE;             //硬件是 OR：保留 M（镜像）和 C（CHR 8K/4K），只把 P 强制成 3。
    apply_control(m);
    return;
  }
   
  uint8_t finished = m->shift_register & 0x01; //第五次bit0才会是1
  //游戏逻辑天然会计数不需要去纠结到底是第几次
  m->shift_register = (uint8_t)((m->shift_register >> 1) | ((data & 1) << 4));
  
  if (!finished) return;
  
    uint8_t result = m->shift_register;

    //第五次写入完成后重置，第五次只会重置移位寄存器和计数器
    m->shift_register = SHIFT_FLAG;

    uint8_t select = (addr >> 13) & 0x03;
    switch (select) {
      case 0:  //Control (internal, $8000-$9FFF)
        control(m, result);
        break;
      case 1:  //CHR bank 0 (internal, $A000-$BFFF)
        //chr_bank0(m, result);
        break;
      case 2:  //CHR bank 1 (internal, $C000-$DFFF)
        //chr_bank1(m, result);
        break;
      case 3:  //PRG bank (internal, $E000-$FFFF)
        prg_bank(m, result);
        break;
      default:
        break;
    }
  
  /*
  if (m->write_record < 4) {
    uint8_t bit0 = data & 0x01;
    (m->shift_register >> m->write_record) |= (bit0 << (4 -m->write_record));
    m->write_record++;
  } else {
    uint8_t result = m->shift_register;
    uint8_t bit0 = data & 0x01;
    result |= bit0 << (4- m->write_record);

    //第五次写入完成后重置，第五次只会重置移位寄存器和计数器
    m->shift_register = SHIFT_FLAG;
    m->write_record = 0;
    //m->control = 0x0C;

    uint8_t select = (addr >> 13) & 0x03;
    switch (select) {
      case 0:  //Control (internal, $8000-$9FFF)
        control(m, result);
        break;
      case 1:  //CHR bank 0 (internal, $A000-$BFFF)
        chr_bank0(m, result);
        break;
      case 2:  //CHR bank 1 (internal, $C000-$DFFF)
        chr_bank1(m, result);
        break;
      case 3:  //PRG bank (internal, $E000-$FFFF)
        prg_bank(m, result);
        break;
      default:
        break;
    }
  }
  */  
}

static const MapperVTable mapper_001_vtable = {
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
};

// ========== 构造函数 ==========

Mapper001* mapper_001_create(Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks) {
  Mapper001* m = (Mapper001*)malloc(sizeof(Mapper001));
  if (!m) return NULL;

  // 设置共享的 vtable
  m->base.vtable = &mapper_001_vtable;
  //m->base.cart = cart;
  m->base.prg_banks = prg_banks;
  m->base.chr_banks = chr_banks;

  m->base.cart = cart;

  m->shift_register = SHIFT_FLAG;
  m->control = CONTROL_VALUE;
  m->chr_bank0 = 0;
  m->chr_bank1 = 0;
  m->prg_bank = 0;
  
  apply_control(m);

  pprg_rom[0] = cart->prg_rom;
  //start = micros() - start;
  //printf("16kb memcpy %u\n", start);
  //memcpy(cpu_prg_rom + 0x4000, cart.prg_rom + (m->base.prg_banks-1) * 0x4000, 0x4000);
  pprg_rom[1] = cart->prg_rom + (m->base.prg_banks-1) * 0x4000;
  //上电默认初始把第一个chr给ppu，块级别是8k
  //memcpy(ppu.ram, cart->chr_rom, 0x2000);
  
  //ppu_copy(0x0000, cart->chr_rom, 0x2000);
  //mapper2最主要的就是切换prg，默认先把0给0x8000-BFFF,之后来回切换
  //0xC000-0xFFFF,一定是最后一个bank
  printf("...............%d   %p  %p  %p\n", prg_banks,  pprg_rom[0],pprg_rom[1], cart->prg_rom);
  //memcpy(cpu_prg_rom, cart->prg_rom, 0x4000);
  //memcpy(cpu_prg_rom + 0x4000, cart->prg_rom + ((prg_banks-1) * 0x4000), 0x4000);
  return m;
}


void mapper_001_destroy(Mapper001* mapper) {
  free(mapper);
}