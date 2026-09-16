#include "io.h"
//NES IO 读写

static uint8_t prev_write;
static int p = 10;

uint8_t io_read(uint16_t address) {
    // Joystick 1
    if (address == 0x4016) {
        if (p++ < 9) {
            //return get_key_state(p);
            //if (p == 4) return 0;
            return 1;
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