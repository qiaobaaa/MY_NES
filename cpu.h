#pragma once
/*
┌─────────────────────────────────────────────────────────────┐
│ main() │
│ │
│ ┌─────────┐ ┌─────────┐ ┌─────────┐ │
│ │ CPU │◄────►│ PPU │◄────►│ APU │ │
│ └────┬────┘ └────┬────┘ └─────────┘ │
│ │ │ │
│ │ ┌──────┴──────┐ │
│ └────────►│ Cartridge │◄───────────────────────── │
│ └─────────────┘ │
│ │
│ CPU 持有 PPU 和 Cartridge 的指针 │
│ PPU 持有 Cartridge 的指针 │
└─────────────────────────────────────────────────────────────┘

优点：

无中央总线，耦合更低
每个组件更独立
符合 C 语言的风格
*/
// cpu.h - CPU 知道 PPU 和 Cartridge
#include <stdint.h>
#include "cart.h"
#include "ppu.h"

typedef struct CPU {
Cartridge cart; // 用于读取 PRG ROM
PPU* ppu; // 用于访问 PPU 寄存器
APU* apu; // 用于访问 APU 寄存器

uint8_t ram[2048];    // CPU 内部 RAM

// 寄存器
uint8_t a, x, y, sp;
uint16_t pc;
uint8_t status;
} CPU;

void cpu_init(CPU* cpu, Cartridge* cart, PPU* ppu, APU* apu);
void cpu_clock(CPU* cpu);
