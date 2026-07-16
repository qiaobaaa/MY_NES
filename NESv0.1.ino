#include "cartridge.h"
#include "ppu.h"
#include "mapper_000.h"
#include "memory.h"
#include "cpu.h"
#include "ppu.h"

Cartridge cart;

void setup() {
  // put your setup code here, to run once:
  /*
  CPU* cpu = (CPU*)malloc(sizeof(CPU));
  PPU* ppu = (PPU*)malloc(sizeof(PPU));
  cpu_init(cpu, &cart);
  Mapper* mapper = (Mapper*)mapper_000_create(&cart, 2, 1); 
  mapper->prg_banks = 6;
  mapper_000_destroy((Mapper000*)mapper);
  uint8_t *a;
  uint8_t *b;
  memory_init(a, 66);
  */
  Mapper000* mapper = mapper_000_create(&cart, 2, 6);
  cart.mapper = (Mapper*)mapper;
  cart.prg_rom = (uint8_t*)malloc(sizeof(uint8_t));  //uint8_t
  printf("malloc 返回地址: %p\n", (void*)cart.prg_rom);

  if (cart.prg_rom == NULL) {  // ✅ 检查是否分配成功
    printf("内存分配失败\n");
    //return -1;
  }
  cart.prg_rom[0] = 176;
  printf("%d\n", cart.mapper->prg_banks);
  //mapper_reset(mapper);
  //mapper_reset(cart.mapper);
  //cart.mapper->vtable->reset(cart.mapper);
  //*cart.prg_rom = 6;
  printf("%d\n", cart.prg_rom[0]);
}

void loop() {
  // put your main code here, to run repeatedly:
}
