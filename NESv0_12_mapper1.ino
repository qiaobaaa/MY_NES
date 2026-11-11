/*
 * ESP32-P4 NES Emulator Launcher (Non-blocking State Machine)
 * 请将此代码保存为 .ino 或 .cpp 文件
 */

#ifndef CURRENT_SCREEN
#define CURRENT_SCREEN SCREEN_2_8_DSI_TOUCH_A
#endif

#include <Arduino_GFX_Library.h>
#include <U8g2lib.h>
#include <SD_MMC.h>

#include "cpu.h"
#include "ppu.h"
#include "bus.h"
#include "mapper.h"
#include "mapper_000.h"
#include "mapper_001.h"
#include "mapper_002.h"
#include "mapper_003.h"
#include "cartridge.h"
#include "io.h"
// =============================================================================
// 显示初始化
// =============================================================================
Arduino_ESP32DSIPanel *dsipanel = new Arduino_ESP32DSIPanel(
  display_cfg.hsync_pulse_width, display_cfg.hsync_back_porch, display_cfg.hsync_front_porch,
  display_cfg.vsync_pulse_width, display_cfg.vsync_back_porch, display_cfg.vsync_front_porch,
  display_cfg.prefer_speed);
Arduino_DSI_Display *gfx = new Arduino_DSI_Display(
  display_cfg.width, display_cfg.height, dsipanel, 1, true, -1, display_cfg.init_cmds, display_cfg.init_cmds_size);

// =============================================================================
// 配置参数
// =============================================================================
#define MAX_GAMES 1000
#define GAMES_PER_PAGE 10
#define MAX_RECURSION_DEPTH 2

#define KEY_A 16
#define KEY_B 17
#define KEY_SELECT 18
#define KEY_START 19
#define KEY_UP 20
#define KEY_DOWN 21
#define KEY_LEFT 22
#define KEY_RIGHT 23

enum Button { NONE, UP, DOWN, LEFT, RIGHT, START, EXIT };
enum SystemState { STATE_FILE_LIST, STATE_GAME_RUNNING };

struct GameInfo {
    String fullPath;
    String displayName;
};

CPU cpu;
PPU ppu;
Cartridge cart;
nes_joypad joypadd;
uint16_t screen[256 * 242];
bool chr_rom_flag;
// =============================================================================
// 全局变量
// =============================================================================
GameInfo gameList[MAX_GAMES];
int totalGames = 0;
int currentPage = 0;
int selectedIndex = 0;
int currentDepth = 0;
SystemState currentState = STATE_FILE_LIST;

Button lastBtn = NONE;
unsigned long keyPressStart = 0;
unsigned long lastRepeatTime = 0;
const unsigned long KEY_DEBOUNCE    = 30;
const unsigned long KEY_FIRST_DELAY = 400;
const unsigned long KEY_REPEAT_INTERVAL = 150;

bool pageChanged = true;
int oldArrowX = 0;
int oldArrowY = 0;
#define LINE_H 20
#define LIST_START_X 16
#define LIST_START_Y 16

uint32_t count;
uint32_t m_time;
// =============================================================================
// 前置声明
// =============================================================================
void drawUI();
void freeGameMemory();
void initEmulator();
void emu_run_frame();

// =============================================================================
// 扫描 SD 卡目录
// =============================================================================
void scanDirectory(File dir) {
    if (currentDepth > MAX_RECURSION_DEPTH) return;
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;
        if (entry.isDirectory()) {
            currentDepth++;
            scanDirectory(entry);
            currentDepth--;
        } else {
            String name = entry.name();
            if ((name.endsWith(".nes") || name.endsWith(".NES")) && totalGames < MAX_GAMES) {
                gameList[totalGames].fullPath = entry.path();
                gameList[totalGames].displayName = name;
                totalGames++;
            }
        }
        entry.close();
    }
}

// =============================================================================
// 游戏加载与内存释放
// =============================================================================
void loadNESGame(String filePath) {
    Serial.printf("\n[System] Loading: %s\n", filePath.c_str());
    
    freeGameMemory(); // 加载前先清理旧内存

    File romFile = SD_MMC.open(filePath, FILE_READ);
    
    if (!romFile) {
    printf("\n打开 /super.nes 失败！检查文件名大小写，文件放在SD根目录");
  } else {
    printf("\n==== super.nes 前16字节(iNES Header) ====\n");
    //uint8_t buf[16];
    size_t readLen;  //= romFile.read(buf, 16); //最多读16字节
  
    readLen = romFile.read(cart.header, 16);  //最多读16字节

    for (int i = 0; i < readLen; i++) {
      printf("%02X ", cart.header[i]);
    }

    uint8_t flag_6 = cart.header[6];
    uint8_t mirror = flag_6 & 0x01;
    uint8_t flag_7 = cart.header[7];
    uint8_t mapper_num = (flag_7 & 0xF0) | ((flag_6 >> 4) & 0x0F);

    // printf("prg_banks = %d, chr_banks = %d, mirror = %d, mapper_num = %d\n", cart.prg_banks, cart.chr_banks, mirror, mapper_num);
    // printf("\n==========================================");
    // delay(100000);

    cart.prg_banks = cart.header[4];
    cart.chr_banks = cart.header[5];

    cart.prg_rom_size = cart.header[4] * 16 * 1024;
    cart.chr_rom_size = cart.header[5] * 8 * 1024;
    
    chr_rom_flag = !!cart.chr_rom_size;       //高效，常用于追求极致简洁的底层开发。

    printf("prg_banks = %d, chr_banks = %d, mirror = %d, mapper_num = %d\n", cart.prg_banks, cart.chr_banks, mirror, mapper_num);
    printf("prg_rom_size = %d, chr_rom_size = %d\n", cart.prg_rom_size, cart.chr_rom_size);
    printf("\n==========================================\n");
    //delay(100000);

    if (cart.prg_rom_size != 0) {
      cart.prg_rom = (uint8_t *)heap_caps_malloc(cart.prg_rom_size, MALLOC_CAP_SPIRAM);
      if (cart.prg_rom == NULL /*|| cart.chr_rom == NULL*/) {
        printf("ERR_MEMORY_ALLOCATE_is null  %d  %d\n", cart.prg_rom_size, cart.chr_rom_size);
        if (cart.prg_rom) heap_caps_free(cart.prg_rom);
        //if (cart.chr_rom) heap_caps_free(cart.chr_rom);
        romFile.close();
        return;
      }
      //装入PRG ROM
      if (romFile.read(cart.prg_rom, cart.prg_rom_size) != (size_t)cart.prg_rom_size) {
        printf("ERR_PRG_ROM_LOAD_FAILED\n");
        heap_caps_free(cart.prg_rom);
        //heap_caps_free(cart.chr_rom);
        romFile.close();
        return;
      }
    }

    if (cart.chr_rom_size != 0) {
       cart.chr_rom = (uint8_t *)heap_caps_malloc(cart.chr_rom_size, MALLOC_CAP_SPIRAM);
      //检查空指针
      if (cart.chr_rom == NULL) {
        printf("ERR_MEMORY_ALLOCATE_is null  %d  %d\n", cart.prg_rom_size, cart.chr_rom_size);
        //if (cart.prg_rom) heap_caps_free(cart.prg_rom);
        if (cart.chr_rom) heap_caps_free(cart.chr_rom);
        romFile.close();
        return;
      }
      if (romFile.read(cart.chr_rom, cart.chr_rom_size) != (size_t)cart.chr_rom_size) {
        printf("ERR_CHR_ROM_LOAD_FAILED\n");
        //heap_caps_free(cart.prg_rom);
        heap_caps_free(cart.chr_rom);
        romFile.close();
        return;
      }
    }

    switch (mapper_num) {
      case 0:
        cart.mapper = (Mapper *)mapper_000_create(&cart, cart.header[4], cart.header[5]);
        break;
      case 1:
        cart.mapper = (Mapper *)mapper_001_create(&cart, cart.header[4], cart.header[5]);
        break;
      case 2:
        cart.mapper = (Mapper *)mapper_002_create(&cart, cart.header[4], cart.header[5]);
        break;
      case 3:
        cart.mapper = (Mapper *)mapper_003_create(&cart, cart.header[4], cart.header[5]);
      default:
        break;
    }
  }

    romFile.close();

    initEmulator(); 
    // 👇 核心补丁：防止按键穿透到游戏内部
// 强制等待菜单操作的按键全部松开，再进入游戏
while (digitalRead(KEY_START) == LOW || digitalRead(KEY_SELECT) == LOW || 
       digitalRead(KEY_A) == LOW || digitalRead(KEY_B) == LOW) {
    delay(10); // 稍微等一下防抖
}
    currentState = STATE_GAME_RUNNING;
    Serial.println("[Success] Entering emulator");
}

void freeGameMemory() {
    if (cart.prg_rom) { heap_caps_free(cart.prg_rom); cart.prg_rom = nullptr; }
    if (cart.chr_rom) { heap_caps_free(cart.chr_rom); cart.chr_rom = nullptr; }
}

void initEmulator() {
    gfx->fillScreen(RGB565_BLACK);
    joypadd.mask = 0;
  joypadd.offset = 0;
  joypadd.joypad = 0;
  //sprite_num = 0;
  memory_init(cart.prg_rom, cart.prg_rom_size);

  // 将 CHR ROM 装入 PPU 内存
  //ppu_copy(0x0000, cart.chr_rom, 0x2000);
  ppu_init();
  ppu_set_mirroring(cart.header[6] & 1);
  cpu_init();
  cpu.dma  = false;
  count = 0;
  m_time = 0;
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
// ?? 核心：这里必须是你原来 emu_run() 里面那个 for (scanline = 0; scanline < 262; scanline++) 的逻辑
// 千万别在里面写 while(true) 死循环，否则状态机就卡死了！
void emu_run_frame() {
    int rem = 0;  // 跨帧不重置
  //int ticks = 0;

uint32_t ppu_time = 0; 
uint32_t p = 0; 
uint32_t cpu_time = 0; 
uint32_t c = 0;
uint32_t d = 0;
  while (true)  //暂时不退出，游戏一直进行
  {
    //═══ 帧准备 ═══
    //palette_generate(nes);
    //apu_frame(nes);
    ppu.status.STATUS_S = 0;
    ppu.status.STATUS_O = 0;
    count  = 0;
    //ppu.STATUS_S = 0;
    //ppu.STATUS_O = 0;
    uint32_t start = micros();
    c = 0;
    m_time = 0;
    //ppu.scanline++;
    button();
    // ═══ 262 行统一循环（含可见 + vblank + pre - render）═══
    for (int scanline = 0; scanline < 262; scanline++) {
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
      // ppu.scanline++;
      //2. 可见行(0~239) 才渲染
      if (scanline < 240) {
        
        ppu_build_palette_cache();  //给每一行建立调色板缓存数据，避免重复计算
        //if (ppu.mask.MASK_s) {
        //prepare_sprite_scanline(i);
        //}
        //ppu_time = micros();
        if (ppu.mask.MASK_b) {
          ppu_draw_background_scanlinee(scanline);
        } else {
          fill_line(scanline);
        }

        if (ppu.mask.MASK_s && ppu.sprite_num != 0 && scanline == ppu.sprite_scanline && scanline != 0) {
          ppu_draw_sprite_scanline(scanline);
        }
        ppu.sprite_num = 0;
        if (ppu.mask.MASK_s) {
          prepare_sprite_scanline(scanline);
        }
        //# 3. CPU 执行 — 前半段(dot 0~254)
        //cpu_execute(nes, nes.timing.line_split);  //# 85,前半段固定为85
        //printf("1111       %d\n",ppu.fine_x);



        //int n = 85 - (ppu.ppu_count * (85/256));
        // int sub = (ppu.ppu_count * 85) / 256;
        //int n = 85 - sub;
        //  printf("%d  %d\n",ppu.ppu_count,  n);
        //int n = 85 - 85;

        int n = (ppu.ppu_count + 1) * 85 / 256;
        // printf("%d  %d\n",ppu.ppu_count,  n);
        cpu_time = micros();
        cpu_run(n);
        if (ppu.ppu_sprite_hit_occured) {
          ppu.ppu_sprite_hit_occured = false;
          ppu.status.STATUS_S = 1;
        }
        //if(ppu_sprite_hit_occured){
        cpu_run(85 - n);
        cpu_time = micros() - cpu_time;
        c+=cpu_time;

        //cpu_run(45);

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
          //nes->nes_ppu.v_reg = (nes->nes_ppu.v_reg & (uint16_t)0xFBE0) | (nes->nes_ppu.t_reg & (uint16_t)0x041F);
        }

        //if (nes->nes_mapper.mapper_hsync) {
        //nes->nes_mapper.mapper_hsync(nes);
        //}
        //cpu_run(85);
        //# 5. CPU 执行 — 后半段(dot 255~340)
        //cpu_execute(nes, ticks - nes.timing.line_split);  //# 28 or 29
        //printf("3333       %d\n",ppu.fine_x);
        cpu_time = micros();
        cpu_run(ticks - 85);
        cpu_time = micros() - cpu_time;
        c+=cpu_time;
        //printf("4444       %d\n",ppu.fine_x);
        //sprite_num = 0;
      } else {
        if (scanline == 240) {
          ppu.status.STATUS_V = 1;  //# VBlank 开始  Set VBlank flag (241 line)
          if (ppu.ctrl.CTRL_V) {
            cpu.irq_nmi = 1;
          }
          //ppu_set_sprite_0_hit(false);
          //cpu.irq_nmi = 1;
          //if (nes->nes_mapper.mapper_vsync)
          //nes->nes_mapper.mapper_vsync(nes);
        }
         
        if (scanline == 261) {
          ppu.status.ppu_status = 0;  // Clear:VBlank,Sprite 0,Overflow
          cpu.dma  = false;
        }
        
         if(cpu.dma  == true && scanline>255)
         {
          continue;
         }
         
        //cpu_execute(nes, ticks);
        cpu_time = micros();
        cpu_run(ticks);
        cpu_time = micros() - cpu_time;
        c+=cpu_time;
        if ((ppu.mask.MASK_b || ppu.mask.MASK_s) && scanline == 261) {
          // Dot 257 copies horizontal scroll bits, then dots 280-304 copy vertical bits.
          // At line granularity this is equivalent to syncing the full VRAM address.
          // https://www.nesdev.org/wiki/PPU_scrolling#During_dots_280_to_304_of_the_pre-render_scanline_(end_of_vblank)
          ppu.v.reg = ppu.t.reg;
        }
      }
      // APU 每 N 行触发一次
      //if (i % nes->timing.apu_frame_divisor == nes->timing.apu_frame_divisor - 1) {
      //nes_apu_frame(nes);
      //}
    }
    //# ═══ 帧提交 ═══
    //draw_frame(nes);
    gfx->draw16bitRGBBitmap(32, 0, screen, 256, 240);
    start = micros() - start;

    int fps = 1000000 / start;
    //printf("ppu time = %u  %u\n",p,d);
   printf("cpu time = %u\n",c);
   printf("all time = %u  %u  %u %d\n",start,count, m_time, fps);
    //printf("FPS = %u\n", fps);
    if (!digitalRead(KEY_SELECT) && !digitalRead(KEY_START)) {
            // 等待按键松开，防误触
            while(!digitalRead(KEY_SELECT) || !digitalRead(KEY_START)) { delay(10); }
            exitGame();
            break;
        }
    
  // gfx->setCursor(0, 0);
  //  gfx->println("FPS");
  //  gfx->setCursor(0, 20);
  //  gfx->fillRect(0, 0, 30, 90, RGB565_BLACK);
  //  gfx->println(fps);
    
    //printf("666\n");
    //nes_frame(nes);  //# 输入 / 退出检查
  }
   // gfx->fillScreen(RGB565_BLACK);
}

void exitGame() {
    Serial.println("[System] Exiting...");
    freeGameMemory();
    currentState = STATE_FILE_LIST;
    gfx->fillScreen(RGB565_BLUE); 
    pageChanged = true;
    drawUI(); 
}

// =============================================================================
// 按键与菜单逻辑
// =============================================================================
Button getButton() {
    if (!digitalRead(KEY_UP))    return UP;
    if (!digitalRead(KEY_DOWN))  return DOWN;
    if (!digitalRead(KEY_LEFT))  return LEFT;
    if (!digitalRead(KEY_RIGHT)) return RIGHT;
    if (!digitalRead(KEY_START)) return START;
    if (!digitalRead(KEY_SELECT))return EXIT;
    return NONE;
}

void handleMenuInput(Button btn) {
    int totalPages = (totalGames + GAMES_PER_PAGE - 1) / GAMES_PER_PAGE;
    if (totalPages == 0) return;
    pageChanged = false;

    switch (btn) {
        case UP:
            if (selectedIndex > 0) selectedIndex--;
            else { selectedIndex = totalGames - 1; pageChanged = true; }
            break;
        case DOWN:
            if (selectedIndex < totalGames - 1) selectedIndex++;
            else { selectedIndex = 0; pageChanged = true; }
            break;
        case LEFT:
            currentPage--;
            if (currentPage < 0) currentPage = totalPages - 1;
            selectedIndex = currentPage * GAMES_PER_PAGE;
            if (selectedIndex >= totalGames) selectedIndex = totalGames - 1;
            pageChanged = true;
            break;
        case RIGHT:
            currentPage++;
            if (currentPage >= totalPages) currentPage = 0;
            selectedIndex = currentPage * GAMES_PER_PAGE;
            if (selectedIndex >= totalGames) selectedIndex = totalGames - 1;
            pageChanged = true;
            break;
        case START:
            if(totalGames > 0) loadNESGame(gameList[selectedIndex].fullPath);
            return;
        default:
            break;
    }

    int newPage = selectedIndex / GAMES_PER_PAGE;
    if (newPage != currentPage) {
        currentPage = newPage;
        pageChanged = true;
    }
}

void drawUI() {
    int startIdx = currentPage * GAMES_PER_PAGE;
    int endIdx = startIdx + GAMES_PER_PAGE;
    if (endIdx > totalGames) endIdx = totalGames;
    int ascent = 16; 

    if (pageChanged) {
        gfx->fillRect(LIST_START_X, LIST_START_Y - ascent, gfx->width() - LIST_START_X, LINE_H * GAMES_PER_PAGE, RGB565_BLUE);
        for (int i = startIdx; i < endIdx; i++) {
            int row = i - startIdx;
            int y = LIST_START_Y + row * LINE_H;
            gfx->setCursor(LIST_START_X, y);
            if (i == selectedIndex) gfx->print("-> ");
            else gfx->print("   ");
            gfx->print(i + 1); gfx->print(". ");
            gfx->print(gameList[i].displayName);
        }
        oldArrowX = LIST_START_X;
        oldArrowY = LIST_START_Y + (selectedIndex - startIdx) * LINE_H;
    } else {
        gfx->fillRect(oldArrowX, oldArrowY - ascent, gfx->width() - LIST_START_X, LINE_H, RGB565_BLUE);
        int oldRowIdx = startIdx + (oldArrowY - LIST_START_Y) / LINE_H;
        gfx->setCursor(LIST_START_X, oldArrowY);
        gfx->print("   ");
        gfx->print(oldRowIdx + 1); gfx->print(". ");
        gfx->print(gameList[oldRowIdx].displayName);

        int newY = LIST_START_Y + (selectedIndex - startIdx) * LINE_H;
        gfx->setCursor(LIST_START_X, newY);
        gfx->print("-> ");
        gfx->print(selectedIndex + 1); gfx->print(". ");
        gfx->print(gameList[selectedIndex].displayName);
        oldArrowY = newY;
    }

    int totalPages = (totalGames + GAMES_PER_PAGE - 1) / GAMES_PER_PAGE;
    gfx->fillRect(LIST_START_X, gfx->height() - 16 - ascent, 120, LINE_H, RGB565_BLUE);
    gfx->setCursor(LIST_START_X, gfx->height() - 16);
    gfx->print("Page: ");
    gfx->print(currentPage + 1); gfx->print("/"); gfx->print(totalPages);
}

// =============================================================================
// Setup
// =============================================================================
void setup() {
    Serial.begin(115200);
    pinMode(KEY_A, INPUT_PULLUP); pinMode(KEY_B, INPUT_PULLUP);
    pinMode(KEY_START, INPUT_PULLUP); pinMode(KEY_SELECT, INPUT_PULLUP);
    pinMode(KEY_UP, INPUT_PULLUP); pinMode(KEY_DOWN, INPUT_PULLUP);
    pinMode(KEY_LEFT, INPUT_PULLUP); pinMode(KEY_RIGHT, INPUT_PULLUP);

    gfx->setUTF8Print(true);
    gfx->setFont(u8g2_font_unifont_t_chinese);
    gfx->begin();
    gfx->fillScreen(RGB565_BLUE);

    if (!SD_MMC.begin()) { while (1); }
    File root = SD_MMC.open("/");
    scanDirectory(root);
    root.close();

    if (totalGames > 0) { selectedIndex = 0; currentPage = 0; }
    pageChanged = true;
    drawUI();
}

// =============================================================================
// 非阻塞主循环
// =============================================================================
void loop() {
    unsigned long now = millis();

    if (currentState == STATE_FILE_LIST) {
        Button btn = getButton();
        if (btn != NONE) {
            if (lastBtn == NONE) {
                if (now - keyPressStart > KEY_DEBOUNCE) {
                    handleMenuInput(btn);
                    if (currentState == STATE_FILE_LIST) drawUI(); 
                    keyPressStart = now;
                    lastRepeatTime = now;
                }
            } else {
                if (now - keyPressStart >= KEY_FIRST_DELAY) {
                    if (now - lastRepeatTime >= KEY_REPEAT_INTERVAL) {
                        handleMenuInput(btn);
                        if (currentState == STATE_FILE_LIST) drawUI();
                        lastRepeatTime = now;
                    }
                }
            }
        }
        if (btn == NONE) { keyPressStart = 0; lastRepeatTime = 0; }
        lastBtn = btn;
        
        delay(20); // UI 状态允许 delay
    }
    else if (currentState == STATE_GAME_RUNNING) {
        // 1. 跑一帧，绝不能有 delay 和 while(true)
        emu_run_frame(); 
        
        // 2. 映射游戏按键
        // joypadd.A = !digitalRead(KEY_A);
        // joypadd.B = !digitalRead(KEY_B);
        // joypadd.START = !digitalRead(KEY_START);
        // joypadd.SELECT = !digitalRead(KEY_SELECT);
        // joypadd.U = !digitalRead(KEY_UP);
        // joypadd.D = !digitalRead(KEY_DOWN);
        // joypadd.L = !digitalRead(KEY_LEFT);
        // joypadd.R = !digitalRead(KEY_RIGHT);

        // 3. 检查退出组合键 (SELECT + START)
        /*
        if (!digitalRead(KEY_SELECT) && !digitalRead(KEY_START)) {
            // 等待按键松开，防误触
            while(!digitalRead(KEY_SELECT) || !digitalRead(KEY_START)) { delay(10); }
            exitGame();
        }
        */
    }
}