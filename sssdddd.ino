#include <Arduino.h>

#include <WiFi.h>
#include <stdio.h>
#include <FS.h>
#include <SD.h>
#include <SD_MMC.h>

// ESP32‑P4 Slot1 原生SDMMC引脚
#define SDMMC_CLK  43
#define SDMMC_CMD  44
#define SDMMC_D0   39
#define SDMMC_D1    14
#define SDMMC_D2    9
#define SDMMC_D3    10

int clk = 43;
int cmd = 44;
int d0 = 39;
int d1 = 40;
int d2 = 41;
int d3 = 42; 
//4 pixels wide, 5 pixel high
unsigned char chip8_fontset[80] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
  0x20, 0x60, 0x20, 0x20, 0x70,  // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
  0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
  0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
  0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
  0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
  0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

void setup(void) {
  printf("7777777777777\n");
  Serial.begin(115200);
  Serial.println("未检测到SD卡");
  delay(100);
 // if(! SD_MMC.setPins(clk, cmd, d0, d1, d2, d3)){
  //      printf("Pin change failed!");
  //     return;
  //  }
//#if defined(SOC_SDMMC_USE_GPIO_MATRIX) 
if (!SD_MMC.begin()) {
    printf("Card Mount Failed");
    return;
  } 
  else
  {
    printf("Card Mounted");
  }
  //SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0, -1, -1, -1);
/*
  if (!SD_MMC.begin("/root", false)) {
    Serial.println("FFat Mount Failed");
    return;
  }
  */
//#endif
  
  
  printf("66666666666666");
/*
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0, -1, -1, -1);

  if (!SD_MMC.begin("/root", false)) {
    Serial.println("FFat Mount Failed");
    return;
  }
*/  
  // 获取并打印SD卡信息
  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("未检测到SD卡");
    while (true)
      ;
  }

  Serial.print("SD卡类型: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("未知");
  }
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD卡大小: %llu MB\n", cardSize);

 // ============新增：打开super.nes，读取前16字节(iNES头部)============
  File romFile = SD_MMC.open("/super.nes", FILE_READ);
  if (!romFile)
  {
      printf("\n打开 /super.nes 失败！检查文件名大小写，文件放在SD根目录");
  }
  else
  {
      printf("\n==== super.nes 前16字节(iNES Header) ====");
      uint8_t buf[16];
      size_t readLen = romFile.read(buf, 16); //最多读16字节
      
      for(int i=0; i<readLen; i++)
      {
          printf("%02X ", buf[i]);
      }
      printf("\n==========================================");
      romFile.close(); //必须关闭文件
  }


  // 开始扫描文件
  Serial.println("\n开始扫描根目录下的所有文件和文件夹：");
}

void loop() {
  // put your main code here, to run repeatedly:
printf("1123\n");
delay(2000);
}
