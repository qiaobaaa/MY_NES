void setup() {
  /  /*
  MALLOC_CAP_SPIRAM
→ 只从 PSRAM 分配
MALLOC_CAP_INTERNAL
→ 只从内部 RAM分配
MALLOC_CAP_DEFAULT
→ 自动分配（先内部，不够再 PSRAM）
  */
  nes.cartridge.prg_rom = (uint8_t *)heap_caps_malloc(nes.cartridge.prg_rom_size, MALLOC_CAP_SPIRAM);
  nes.cartridge.chr_rom = (uint8_t *)heap_caps_malloc(nes.cartridge.chr_rom_size, MALLOC_CAP_SPIRAM);

}

void loop() {
  // put your main code here, to run repeatedly:

}
