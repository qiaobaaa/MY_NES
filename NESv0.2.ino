//#include <Arduino.h>
//#include <WiFi.h>

#ifndef CURRENT_SCREEN
#define CURRENT_SCREEN SCREEN_2_8_DSI_TOUCH_A
#endif

#include <Arduino_GFX_Library.h>

#include <stdio.h>
//#include <FS.h>
//#include <SD.h>
#include <SD_MMC.h>

#include "cpu.h"
#include "ppu.h"
#include "bus.h"
#include "mapper.h"
#include "mapper_000.h"
#include "cartridge.h"

Arduino_ESP32DSIPanel *dsipanel = new Arduino_ESP32DSIPanel(
  display_cfg.hsync_pulse_width,
  display_cfg.hsync_back_porch,
  display_cfg.hsync_front_porch,
  display_cfg.vsync_pulse_width,
  display_cfg.vsync_back_porch,
  display_cfg.vsync_front_porch,
  display_cfg.prefer_speed,
  display_cfg.lane_bit_rate);
Arduino_DSI_Display *gfx = new Arduino_DSI_Display(
  display_cfg.width,
  display_cfg.height,
  dsipanel,
  1,
  true,
  -1,
  display_cfg.init_cmds,
  display_cfg.init_cmds_size);


CPU cpu;
PPU ppu;
Cartridge cart;
//uint16_t screen[260 * 260];
uint16_t screen[256 * 240];

void setup() {
  Mapper* mapper = (Mapper*)mapper_000_create(2,1);
  mapper->chr_banks = 183;
  printf("%d\n", mapper->chr_banks);

  if (!SD_MMC.begin()) {
    printf("Card Mount Failed");
    return;
  } 
  else
  {
    printf("Card Mounted");
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  printf("SD卡大小: %llu MB\n", cardSize);
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
    printf("%d\n",666);
  }
  gfx->fillScreen(RGB565_BLACK);

  //gfx->setCursor(10, 10);
  //gfx->setTextColor(RGB565_RED);
  //gfx->println("Hello World!");
  for(int i=0;i<256*240;i++)
                {
                  
                      screen[i]=0;
                  
                }
}

void loop() {
  //File romFile = SD_MMC.open("/gongfu.nes", FILE_READ);
  File romFile = SD_MMC.open("/super.nes", FILE_READ);
  if (!romFile)
  {
      printf("\n打开 /super.nes 失败！检查文件名大小写，文件放在SD根目录");
  }
  else
  {
      printf("\n==== super.nes 前16字节(iNES Header) ====\n");
      //uint8_t buf[16];
      size_t readLen; //= romFile.read(buf, 16); //最多读16字节
      /*
      for(int i=0; i<readLen; i++)
      {
          printf("%02X ", buf[i]);
      }
      printf("\n==========================================\n");
      */
      readLen = romFile.read(cart.header, 16); //最多读16字节

        for (int i = 0; i < readLen; i++)
        {
            printf("%02X ", cart.header[i]);
        }
        printf("\n==========================================");

        cart.prg_rom_size = cart.header[4] * 16 * 1024;
        cart.chr_rom_size = cart.header[5] * 8 * 1024;

        cart.prg_rom = (uint8_t*)heap_caps_malloc(cart.prg_rom_size, MALLOC_CAP_SPIRAM);
        cart.chr_rom = (uint8_t*)heap_caps_malloc(cart.chr_rom_size, MALLOC_CAP_SPIRAM);
        printf("666666  %d  %d\n", cart.prg_rom_size, cart.chr_rom_size);
        //检查空指针
        if (cart.prg_rom == NULL || cart.chr_rom == NULL)
        {
            printf("ERR_MEMORY_ALLOCATE_is null  %d  %d\n", cart.prg_rom_size, cart.chr_rom_size);
            if (cart.prg_rom) heap_caps_free(cart.prg_rom);
            if (cart.chr_rom) heap_caps_free(cart.chr_rom);
            romFile.close();
            return;
        }
        
        //装入PRG ROM
        if (romFile.read(cart.prg_rom, cart.prg_rom_size) != (size_t)cart.prg_rom_size)
        {
            printf("ERR_PRG_ROM_LOAD_FAILED\n");
            heap_caps_free(cart.prg_rom);
            heap_caps_free(cart.chr_rom);
            romFile.close();
            return;
        }
        if (romFile.read(cart.chr_rom, cart.chr_rom_size)!= (size_t)cart.chr_rom_size)
        {
            printf("ERR_CHR_ROM_LOAD_FAILED\n");
            heap_caps_free(cart.prg_rom);
            heap_caps_free(cart.chr_rom);
            romFile.close();
            return;
        }

        emu_init();
        //signal(SIGINFO, sig_info);
        emu_run();
        emu_exit();
    


      romFile.close(); //必须关闭文件
      while(1){}
  }
}

void emu_init()
{
    nes_init();
}

void emu_run()
{
    /*
    for (;;) {
        //wait_for_frame();
        int scanlines = 262;
        while (scanlines-- > 0) {
            // TODO: 能否用多线程进行优化？
            ppu_run(1);
            cpu_run(1364 / 12);
        }
    }
    */
    while(1)
    {
        int idx = ppu_ram_read(0x3f00);
        uint16_t c = palette_rgb565[idx];
        //gfx->fillScreen(c);
        //delay(20);
        /*
        int idx = ppu_ram_read(0x3f00);
                uint16_t c = palette_rgb565[idx];
                for(int i=0;i<256*240;i++)
                {
                   // if(screen[i]==0)
                   // {
                   //   screen[i]=c;
                   // }
                    printf("%ld\n",screen[i]);
                }
        */
        gfx->draw16bitRGBBitmap(32, 0, screen, 256, 240);
        
        int scanlines = 262;
        while (scanlines-- > 0) {
            // TODO: 能否用多线程进行优化？
            ppu_run(1);
            cpu_run(1364 / 12);
        }
        //gfx->draw16bitRGBBitmap(0, 0, screen, 256, 240);
    }
    
}

void nes_init()
{
    
    memory_init(cart.prg_rom, cart.prg_rom_size);

    // 将 CHR ROM 装入 PPU 内存
    ppu_copy(0x0000, cart.chr_rom, 0x2000);
    ppu_init();
    ppu_set_mirroring(cart.header[6] & 1);
    cpu_init();
    
}

void emu_exit()
{
    
}