#pragma once
#include <stdint.h>

/*
DSI CLKN  4
DSI CLKP  5
DP0   6
DN0   7
39-44 6个引脚被sd卡使用了
*/

typedef struct NES_JOYPAD{
    uint8_t offset;
    uint8_t mask;
    union {
        struct {
            uint8_t A:1;
            uint8_t B:1;
            uint8_t SELECT:1;
            uint8_t START:1; 
            uint8_t U:1; 
            uint8_t D:1; 
            uint8_t L:1;
            uint8_t R:1;
        };
        uint8_t joypad;
    };
} nes_joypad;

/*
0 - A
1 - B
2 - Select
3 - Start
4 - Up
5 - Down
6 - Left
7 - Right
*/


#define KEY_A 16
#define KEY_B 17
#define KEY_SELECT 18
#define KEY_START 19
#define KEY_UP 20
#define KEY_DOWN 21
#define KEY_LEFT 22
#define KEY_RIGHT 23


extern nes_joypad joypadd;

uint8_t io_read(uint16_t address);
void io_write(uint16_t address, uint8_t data);