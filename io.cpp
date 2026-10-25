#include "io.h"

#include <Arduino.h>
#include <stdio.h>
//NES IO 读写

uint8_t io_read(uint16_t address)
{
    /*
    uint8_t state = 0;
    if (address == 0x4016){
        state = (joypadd.joypad & (80 >> ((joypadd.offset) & joypadd.mask))) ? 1 : 0;
        printf(".....%d    %d\n",joypadd.offset, state);
        joypadd.offset++;
    }
    // NES_LOG_DEBUG("nes_read joypad %04X %d %02X %d\n",address,nes->nes_cpu.joypad.mask,nes->nes_cpu.joypad.joypad,state);
    return state;
    */
    uint8_t state = 0;
    if (address == 0x4016)
    {
        if (joypadd.mask != 0)
        {
            // 直接 1 << offset，取bit0开始
            state = (joypadd.joypad & (1 << (joypadd.offset & joypadd.mask))) ? 1 : 0;
            joypadd.offset++;
        }
        else
        {
            // strobe=1，固定返回A（bit0），offset不增加
            state = (joypadd.joypad & (1 << 0)) ? 1 : 0;
        }
    }
    return state;
}


void io_write(uint16_t address, uint8_t data)
{   
    if (address == 0x4016) {
    joypadd.mask = (data & 1)?0x00:0x07;
    if (data & 1)
        joypadd.offset = 0;
    // NES_LOG_DEBUG("nes_write joypad %04X %02X %d\n",address,data,nes->nes_cpu.joypad.mask);
    }
}
/*
static uint8_t  joy_offset = 0;
static uint8_t  joy_mask  = 0;
// 8bit：bit2=Select按下，其余松开
static uint8_t joypad = 0;

static uint8_t prev_write;
static int p = 10;

uint32_t frame_counter = 0;
uint8_t io_read(uint16_t address) {
    // Joystick 1
    if (address == 0x4016) {
        if (p++ < 9) {
            //return get_key_state(p);
           
            switch (p) {
        case 1: return 0;
        case 2: return 0;
        case 3: return !digitalRead(KEY_SELECT);
        case 4: return !digitalRead(KEY_START);
        case 5: return 0; 
        case 6: return 0;
        case 7: return 0;
        case 8: return 0; 
        default: return 0; 
        }
            return 0;
        }
    }
    return 0;
}

void io_write(uint16_t address, uint8_t data) {
    if (address == 0x4016) {
        if ((data & 1) == 0 && prev_write == 1) {
            // strobe
            p = 0;
        }
    }
    prev_write = data & 1;
}
*/
/*
uint8_t io_read(uint16_t address) {
    uint8_t state = 0;

    // 根据当前的 offset，决定读取哪个物理按键  
    // 顺序必须是: A, B, Select, Start, Up, Down, Left, Right
    if (address == 0x4016) {
        switch (joy_offset) {
        case 0: state = 0; break;
        case 1: state = 0; break;
        case 2: state = !digitalRead(KEY_SELECT); break;
        case 3: state = !digitalRead(KEY_SELECT); break;
        case 4: state = 0; break;
        case 5: state = 0; break;
        case 6: state = 0; break;
        case 7: state = 0; break;
        default: state = 0; break;
        }

        // 移位计数增加  
        joy_offset = (joy_offset + 1) & 7;
        return state;
    }
    return state;  //默认地址不是4016返回的就是0松开
}

// --- NES 模拟层：处理 4016 写入 ---  
void io_write(uint16_t address, uint8_t data) {
    if (address == 0x4016) {
        if (data & 1) {
            joy_offset = 0; // 重置偏移量，准备开始新一轮读取  
        }
    }
}
*/
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
/*
uint8_t io_read(uint16_t address)
{
    uint8_t state = 0;
    if (address == 0x4016)
    {
        switch (joy_offset) {
        case 0: state = 0; break;
        case 1: state = 0; break;
        case 2: joypad = !digitalRead(KEY_SELECT); break;
        case 3: joypad = !digitalRead(KEY_START); break;
        case 4: state = 0; break;
        case 5: state = 0; break;
        case 6: state = 0; break;
        case 7: state = 0; break;
        default: state = 0; break;
        }
        //state = (joypad & (0x80U >> (joy_offset & joy_mask))) ? 1 : 0;
        state = (joypad & (0x80U >> (joy_offset & joy_mask))) ? 1 : 0;
        //state = (joypad & (0x80U << (joy_offset & joy_mask))) ? 1 : 0;
        //state = (joypad & (1U << masked_off)) ? 0 : 1;
        //printf("R 4016 off=%d bit0=%d\n",joy_offset,state);
        //if (joy_offset==5){state=1;}
        joy_offset++;
    }
    // 调试用，正式跑ROM注释printf
    printf(".......%d   %d  \n",  joy_offset, state);
    return state; // bit1~bit7强制1，NES硬件要求
    //return 0;
}

void io_write(uint16_t address, uint8_t data)
{
    if (address == 0x4016)
    {
        //printf("W 4016 data=%d\n", data);
        joy_mask = (data & 1U) ? 0x00 : 0x07;
        if (data & 1U)
        {
            joy_offset = 0;
        }
    }
}
*/

/*
uint8_t io_read(uint16_t address)
{
    // Joystick 1
    frame_counter++;
    if (address == 0x4016) {
        if (p++ < 9) {
            //return nes_key_state(p);
            if (p == 4) {
                if(frame_counter>10000&&frame_counter<10020)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }

            return 0;
        }
    }
    return 0;
}

void io_write(uint16_t address, uint8_t data)
{
    if (address == 0x4016) {
        if ((data & 1) == 0 && prev_write == 1) {
            // strobe
            p = 0;
        }
    }
    prev_write = data & 1;
}
*/
/*
uint8_t io_read(uint16_t address) {
    
    uint8_t state = 0;

    // 根据当前的 offset，决定读取哪个物理按键  
    // 顺序必须是: A, B, Select, Start, Up, Down, Left, Right
    if (address == 0x4016) {
        switch (joy_offset) {
        case 0: state = 0; break;
        case 1: state = 0; break;
        case 2: state = 0; break;
        case 3: state = 1; break;
        case 4: state = 0; break;
        case 5: state = 0; break;
        case 6: state = 0; break;
        case 7: state = 0; break;
        default: state = 0; break;
        }
        printf("............ %d   %d\n",joy_offset, state );
        // 移位计数增加  
        joy_offset = (joy_offset + 1) & 7;
        //printf("............ %d   %d\n", )
        return state;
    }
    return state;  //默认地址不是4016返回的就是0松开
}

// --- NES 模拟层：处理 4016 写入 ---  
void io_write(uint16_t address, uint8_t data) {
    if (address == 0x4016) {
        if (data & 1) {
            joy_offset = 0; // 重置偏移量，准备开始新一轮读取  
        }
    }
}
*/


/*
uint8_t io_read(uint16_t address) {
    //return 1;
    // Joystick 1
    if (address == 0x4016) {
        if (p++ < 9) {
            //return get_key_state(p);
            if (p == 4) return 1;
            if (p == 0) return 1;
            if (p == 1) return 1;
            if (p == 2) return 1;
            if (p == 3) return 1;
            if (p == 5) return 1;
            return 0;
        }
    }
    return 1;
}

void io_write(uint16_t address, uint8_t data) {
    if (address == 0x4016) {
        if ((data & 1) == 0 && prev_write == 1) {
            // strobe
            p = 0;
        }
    }
    prev_write = data & 1;
}
*/
/*
Bit No. 15      14      13      12      11      10      9       8
        A1      B1      Select1 Start1  Up1     Down1   Left1   Right1 
Bit No. 7       6       5       4       3       2       1       0
        A2      B2      Select2 Start2  Up2     Down2   Left2   Right2 
*/
/*
uint8_t io_read(uint16_t address)
{
    
    uint8_t state = 0;
    if (address == 0x4016)
    {
        state = (joypad & (0x80U >> (joy_offset & joy_mask))) ? 1 : 0;
        if(joy_offset == 2)
        {
            state = 1;
        }
        state = 1;
        joy_offset++;
    }
    printf("%d   %d  \n",  joy_offset, state);
    return state;
    
    //
    uint8_t state = 0;
    if (address == 0x4016)
    {
        //state = (joypad & (0x80U >> (joy_offset & joy_mask))) ? 1 : 0;
        state = (joypad & (1U << (joy_offset & joy_mask))) ? 1 : 0;
        //state = (joypad & (1U << masked_off)) ? 0 : 1;
        printf("R 4016 off=%d bit0=%d\n",joy_offset,state);
        //if (joy_offset==5){state=1;}
        joy_offset++;
    }
    // 调试用，正式跑ROM注释printf
    printf(".......%d   %d  \n",  joy_offset, state);
    return state; // bit1~bit7强制1，NES硬件要求
    //return 0;
}

void io_write(uint16_t address, uint8_t data)
{
    if (address == 0x4016)
    {
        //printf("W 4016 data=%d\n", data);
        joy_mask = (data & 1U) ? 0x00 : 0x07;
        if (data & 1U)
        {
            joy_offset = 0;
        }
    }
}
*/
/*
static uint8_t prev_write;
static int p = 10;
static uint8_t strobe = 0;
static uint8_t start_pressed = 0;

uint8_t io_read(uint16_t address) {
    if (address == 0x4016) {
        uint8_t bit0;
        if (strobe) {
            p = 0;
            bit0 = 0;
        } else {
            if (p < 8) {
                printf("read p=%d\n", p);
                switch(p)
                {
                    case 0: bit0 = 0; break; // A
                    case 1: bit0 = 0; break; // B
                    case 2: bit0 = 0; break; // Select
                    case 3: bit0 = start_pressed; break; // Start
                    case 4: bit0 = 0; break; // Up
                    case 5: bit0 = 0; break; // Down
                    case 6: bit0 = 0; break; // Left
                    case 7: bit0 = 0; break; // Right
                }
                p++;
            } else {
                bit0 = 1;
            }
        }
        // bit0 = 1P按键；其余bit强制为1（推荐模拟器标准写法）
        return bit0 | 0xFE;
    }
    return 0xFF;
}

void io_write(uint16_t address, uint8_t data) {
    if (address == 0x4016) {
        uint8_t bit0 = data & 1;
        strobe = bit0;
        printf("WRITE 4016 bit0=%d\n", bit0);

        if (prev_write == 1 && bit0 == 0) {
            printf("---- STROBE FALLING EDGE, reset p=0 ----\n");
            p = 0;
            start_pressed = !start_pressed; // 每次采样翻转，制造上升沿
        }
        prev_write = bit0;
    }
}
*/
/*
uint8_t io_read(uint16_t address) {
    // Joystick 1
    if (address == 0x4016) {
        if (p++ < 9) {
            //return get_key_state(p);
            if (p == 4) return 0;
            return 1;
        }
    }
    return 1;
}

void io_write(uint16_t address, uint8_t data) {
    if (address == 0x4016) {
        if ((data & 1) == 0 && prev_write == 1) {
            // strobe
            p = 0;
        }
    }
    prev_write = data & 1;
}
*/
/*
在 NES 的控制器协议中，1 表示按键松开 (Released)，0 表示按键按下 (Pressed)。
如果你想在没有物理按键连接的情况下，让游戏认为“现在没有任何按键被按下”，将 return get_key_state(p); 改为 return 1; 是最简单且最有效的做法。

int get_key_state(int b) {
    switch(b) {
        case 1:  // A 键
            return gpio_get_level(GPIO_NUM_10) == 0 ? 0 : 1; // 假设按键按下为低电平
        // ... 其他按键
    }
}

有些 NES 游戏在启动时会检测 Start 键 才能进入主菜单。如果改为 return 1 后，游戏停在标题画面不动（没有自动进入动画），那说明该游戏需要一个 Start 信号。

在这种情况下，你可以把 Start 键（对应 p=4）强制设为 0：

if (address == 0x4016) {
    if (p++ < 9) {
        if (p == 4) return 0; // 强制模拟 Start 键被按下
        return 1;            // 其他按键均未按下
    }
}
*/