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
#include "io.h"

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


//CPU cpu;
PPU ppu;
Cartridge cart;
nes_joypad joypadd;
//uint16_t screen[260 * 260];
uint16_t screen[256 * 242];

void setup() {

  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  pinMode(KEY_START, INPUT_PULLUP);
  pinMode(KEY_SELECT, INPUT_PULLUP);
  pinMode(KEY_UP, INPUT_PULLUP);
  pinMode(KEY_DOWN, INPUT_PULLUP);
  pinMode(KEY_LEFT, INPUT_PULLUP);
  pinMode(KEY_RIGHT, INPUT_PULLUP);

  Mapper *mapper = (Mapper *)mapper_000_create(2, 1);
  mapper->chr_banks = 183;
  printf("%d\n", mapper->chr_banks);

  if (!SD_MMC.begin()) {
    printf("Card Mount Failed");
    return;
  } else {
    printf("Card Mounted");
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  printf("SD卡大小: %llu MB\n", cardSize);
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
    printf("%d\n", 666);
  }
  gfx->fillScreen(RGB565_BLACK);

  //gfx->setCursor(10, 10);
  //gfx->setTextColor(RGB565_RED);
  //gfx->println("Hello World!");
  for (int i = 0; i < 256 * 240; i++) {

    screen[i] = 0;
  }
}

void loop() {
  //File romFile = SD_MMC.open("/gongfu.nes", FILE_READ);
  //File romFile = SD_MMC.open("/gongfuz.nes", FILE_READ);
  File romFile = SD_MMC.open("/jingang.nes", FILE_READ);
  //File romFile = SD_MMC.open("/super.nes", FILE_READ);
  //File romFile = SD_MMC.open("/70.nes", FILE_READ);
  //File romFile = SD_MMC.open("/7100.nes", FILE_READ);
  //File romFile = SD_MMC.open("/tanke.nes", FILE_READ);
  //File romFile = SD_MMC.open("/Championship Lode Runner.nes", FILE_READ);
  //File romFile = SD_MMC.open("/0262.nes", FILE_READ);
  //zhadanren.nes
  //Championship Lode Runner (J
  //Bomberman.nes
  //c:\Users\86183\Desktop\rom\taojinzhe.nes
  //File romFile = SD_MMC.open("/Bomberman.nes", FILE_READ);
  //File romFile = SD_MMC.open("/taojinzhe.nes", FILE_READ);
  if (!romFile) {
    printf("\n打开 /super.nes 失败！检查文件名大小写，文件放在SD根目录");
  } else {
    printf("\n==== super.nes 前16字节(iNES Header) ====\n");
    //uint8_t buf[16];
    size_t readLen;  //= romFile.read(buf, 16); //最多读16字节
    /*
      for(int i=0; i<readLen; i++)
      {
          printf("%02X ", buf[i]);
      }
      printf("\n==========================================\n");
      */
    readLen = romFile.read(cart.header, 16);  //最多读16字节

    for (int i = 0; i < readLen; i++) {
      printf("%02X ", cart.header[i]);
    }

    uint8_t flag_6 = cart.header[6];
    uint8_t mirror = flag_6 & 0x01;
    uint8_t flag_7 = cart.header[7];
    uint8_t mapper_num = (flag_7 & 0xF0) | ((flag_6 >> 4) & 0x0F);

    printf("mirror = %d, mapper_num = %d\n", mirror, mapper_num);
    printf("\n==========================================");
    //delay(100000);

    cart.prg_rom_size = cart.header[4] * 16 * 1024;
    cart.chr_rom_size = cart.header[5] * 8 * 1024;

    cart.prg_rom = (uint8_t *)heap_caps_malloc(cart.prg_rom_size, MALLOC_CAP_INTERNAL);  //MALLOC_CAP_INTERNAL//MALLOC_CAP_SPIRAM
    cart.chr_rom = (uint8_t *)heap_caps_malloc(cart.chr_rom_size, MALLOC_CAP_INTERNAL);
    printf("666666  %d  %d\n", cart.prg_rom_size, cart.chr_rom_size);
    //检查空指针
    if (cart.prg_rom == NULL || cart.chr_rom == NULL) {
      printf("ERR_MEMORY_ALLOCATE_is null  %d  %d\n", cart.prg_rom_size, cart.chr_rom_size);
      if (cart.prg_rom) heap_caps_free(cart.prg_rom);
      if (cart.chr_rom) heap_caps_free(cart.chr_rom);
      romFile.close();
      return;
    }

    //装入PRG ROM
    if (romFile.read(cart.prg_rom, cart.prg_rom_size) != (size_t)cart.prg_rom_size) {
      printf("ERR_PRG_ROM_LOAD_FAILED\n");
      heap_caps_free(cart.prg_rom);
      heap_caps_free(cart.chr_rom);
      romFile.close();
      return;
    }
    if (romFile.read(cart.chr_rom, cart.chr_rom_size) != (size_t)cart.chr_rom_size) {
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



    romFile.close();  //必须关闭文件
    while (1) {}
  }
}

void emu_init() {
  nes_init();
}

void emu_run() {
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
  while (1) {
    //int idx = ppu_ram_read(0x3f00);
    //uint16_t c = palette_rgb565[idx];
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
    //uint32_t start = micros();
    //gfx->draw16bitRGBBitmap(32, 0, screen, 256, 240);
    //delay(0);
    /*
        for(int i=0;i<256*240;i++)
                {
                  
                      screen[i]=0;
                  
                }
          */
    //start = micros() - start;
    //printf("%u\n",start);
    //delay(10);
    //start = micros();
    //uint32_t start = micros();
    uint32_t startt = micros();
    //uint32_t s = 0;
    int scanlines = 262;
    button();
    //printf("@@@@@@@@@@@  %d\n", joypadd.joypad);
    while (scanlines-- > 0) {
      // TODO: 能否用多线程进行优化？
      //uint32_t start = micros();
      ppu_run(1);
      //start = micros() - start;
      //printf("%u\n",start);
      //start = micros();
      cpu_run(1364 / 12);
      //start = micros() - start;
      //s+=start;
      //printf("......%u\n",start);
    }
    //button();
    //printf("@@@@@@@@@@@  %d\n", joypadd.joypad);
    //startt = micros() - startt;
    //printf("cpu time = %u\n",s);
    //printf("all time = %u\n",startt);
    gfx->draw16bitRGBBitmap(32, 0, screen, 256, 240);
    startt = micros() - startt;
    //printf("cpu time = %u\n",s);
    printf("all time = %u\n", startt);
    //printf("........%u\n",start);
    //start = micros() - start;
    //printf("%u\n",start);
    //delay(12);
    //gfx->draw16bitRGBBitmap(0, 0, screen, 256, 240);
  }
}

void nes_init() {
  joypadd.mask = 0;
  joypadd.offset = 0;
  joypadd.joypad = 0;
  memory_init(cart.prg_rom, cart.prg_rom_size);

  // 将 CHR ROM 装入 PPU 内存
  ppu_copy(0x0000, cart.chr_rom, 0x2000);
  ppu_init();
  ppu_set_mirroring(cart.header[6] & 1);
  cpu_init();
}

void emu_exit() {
}

void button() {
  if (!digitalRead(KEY_A)) {
    joypadd.A = 1;
  } else {
    joypadd.A = 0;
  }

  if (!digitalRead(KEY_B)) {
    joypadd.B = 1;
  } else {
    joypadd.B = 0;
  }

  if (!digitalRead(KEY_START)) {
    joypadd.START = 1;
  } else {
    joypadd.START = 0;
  }

  if (!digitalRead(KEY_SELECT)) {
    joypadd.SELECT = 1;
  } else {
    joypadd.SELECT = 0;
  }

  if (!digitalRead(KEY_UP)) {
    joypadd.U = 1;
  } else {
    joypadd.U = 0;
  }

  if (!digitalRead(KEY_DOWN)) {
    joypadd.D = 1;
  } else {
    joypadd.D = 0;
  }

  if (!digitalRead(KEY_LEFT)) {
    joypadd.L = 1;
  } else {
    joypadd.L = 0;
  }

  if (!digitalRead(KEY_RIGHT)) {
    joypadd.R = 1;
  } else {
    joypadd.R = 0;
  }
}

void nes_run() {
  //cpu_reset(nes);
  int rem = 0;  // 跨帧不重置
  //int ticks = 0;

  while (true)  //暂时不退出，游戏一直进行
  {
    //═══ 帧准备 ═══
    //palette_generate(nes);
    //apu_frame(nes);
    ppu.status.STATUS_S = 0;
    ppu.status.STATUS_O = 0;
    //ppu.STATUS_S = 0;
    //ppu.STATUS_O = 0;

    // ═══ 262 行统一循环（含可见 + vblank + pre - render）═══
    for (int i = 0; i < 262; i++) {
      //1. 计算本行 CPU ticks（分数累加）
      int ticks = 113;
      rem += 2;
      if (rem >= 3) {
        rem -= 3;
        ticks = 114;
      }
      //else
      //{
      //    ticks = 113;
      //}

      //2. 可见行(0~239) 才渲染
      if (i < 240) {
        if (ppu.mask.MASK_s) {
          prepare_sprite_line(nes, scanline);
        }

        if (ppu.mask.MASK_b) {
          render_bg_line(nes, scanline);
        } else {
          fill_line(nes, scanline, palette[0]);
        }

        if (ppu.mask.MASK_s) {
          render_sprite_line(nes, scanline);
        }

        //# 3. CPU 执行 — 前半段(dot 0~254)
        cpu_execute(nes, nes.timing.line_split);  //# 85,前半段固定为85

        //4. PPU 行末地址回绕(dot 256 / 257)
        if (ppu.mask.MASK_b || ppu.mask.MASK_s) {
          ppu.oamaddr = 0;
          // https://www.nesdev.org/wiki/PPU_scrolling#At_dot_256_of_each_scanline
          if ((ppu.v.fine_y) < 7) {
            ppu.v.fine_y++;
          } else {
            ppu.v.fine_y = 0;
            uint8_t y = (uint8_t)(ppu.v.coarse_y);
            if (y == 29) {
              y = 0;
              ppu.v.reg ^= 0x0800;
            } else if (y == 31) {
              y = 0;
            } else {
              y++;
            }
            ppu.v.coarse_y = y;
          }
          // https://www.nesdev.org/wiki/PPU_scrolling#At_dot_257_of_each_scanline
          // v: ....A.. ...BCDEF <- t: ....A.. ...BCDEF
          ppu.v.reg = (ppu.v.reg & (uint16_t)0xFBE0) | (ppu.t.reg & (uint16_t)0x041F);
        }

        if (nes->nes_mapper.mapper_hsync) {
          nes->nes_mapper.mapper_hsync(nes);
        }

        //# 5. CPU 执行 — 后半段(dot 255~340)
        cpu_execute(nes, ticks - nes.timing.line_split);  //# 28 or 29
      } else {
        if (i == 240) {
          ppu.status.STATUS_V = 1;  //# VBlank 开始  Set VBlank flag (241 line)
          if (ppu.ctrl.CTRL_V)
            nes->nes_cpu.irq_nmi = 1;
          if (nes->nes_mapper.mapper_vsync)
            nes->nes_mapper.mapper_vsync(nes);
        }

        if (i == 261) {
          ppu.status.ppu_status = 0;  // Clear:VBlank,Sprite 0,Overflow
        }

        cpu_execute(nes, ticks);
        if ((ppu.mask.MASK_b || ppu.mask.MASK_s) && i == 261) {
          // Dot 257 copies horizontal scroll bits, then dots 280-304 copy vertical bits.
          // At line granularity this is equivalent to syncing the full VRAM address.
          // https://www.nesdev.org/wiki/PPU_scrolling#During_dots_280_to_304_of_the_pre-render_scanline_(end_of_vblank)
          ppu.v.reg = ppu.t.reg;
        }
      }
      // APU 每 N 行触发一次
      if (i % nes->timing.apu_frame_divisor == nes->timing.apu_frame_divisor - 1) {
        nes_apu_frame(nes);
      }
    }
    //# ═══ 帧提交 ═══
    draw_frame(nes);
    nes_frame(nes);  //# 输入 / 退出检查
  }
}
