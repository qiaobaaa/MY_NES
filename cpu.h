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
#include "cartridge.h"
#include "ppu.h"

typedef struct CPU {
	Cartridge* cart; // 用于读取 PRG ROM
	//PPU* ppu; // 用于访问 PPU 寄存器
	//APU* apu; // 用于访问 APU 寄存器

	//uint8_t ram[2048];    // CPU 内部 RAM
	//uint8_t sram[8192];
  //uint8_t ram[0x10000];   //cpu可看见的全部64KB空间
	// 寄存器
	uint8_t  a;    // 累加寄存器 Accumulator
  uint8_t  x;    // 变址寄存器 Index Register X
  uint8_t  y;    // 变址寄存器 Index Register Y
  uint8_t  sp;   // 堆栈指针   Stack Pointer
  uint8_t  p;    // 状态寄存器 Status Register
  uint16_t pc;   // 程序计数器 Program Counter
} CPU;

extern CPU cpu;

void cpu_init(/*CPU* cpu, Cartridge* cart, PPU* ppu, APU* apu*/);
uint64_t cpu_clock(/*CPU* cpu*/);
void cpu_interrupt();
void IRAM_ATTR cpu_run(int cycles);
void cpu_init_table(void);

void cpu_debugger();

typedef void (*opcode_handler)(void);

typedef struct Instruction {
	opcode_handler handler;
	uint8_t cycles;
}Instruction;
