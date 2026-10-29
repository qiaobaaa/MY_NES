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
	//Cartridge* cart; // 用于读取 PRG ROM
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
  //uint8_t  p;    // 状态寄存器 Status Register
	union {
        struct {
            uint8_t C:1;                /*  carry flag (1 on unsigned overflow) */
            uint8_t Z:1;                /*  zero flag (1 when all bits of a result are 0) */
            uint8_t I:1;                /*  IRQ flag (when 1, no interupts will occur (exceptions are IRQs forced by BRK and NMIs)) */
            uint8_t D:1;                /*  decimal flag (1 when CPU in BCD mode) */
            uint8_t B:1;                /*  break flag (1 when interupt was caused by a BRK) */
            uint8_t U:1;                /*  unused (always 1) */
            uint8_t V:1;                /*  overflow flag (1 on signed overflow) */
            uint8_t N:1;                /*  negative flag (1 when result is negative) */
        };
        uint8_t p;                      /*  Status Register */
    };
  uint16_t pc;   // 程序计数器 Program Counter

	uint8_t irq_nmi;
	uint8_t irq_nmi_delay;
	uint8_t irq_pending;
} CPU;

extern CPU cpu;

void cpu_init(/*CPU* cpu, Cartridge* cart, PPU* ppu, APU* apu*/);
uint64_t cpu_clock(/*CPU* cpu*/);
//void cpu_interrupt();
void IRAM_ATTR cpu_run(int cycles);
void cpu_init_table(void);

void cpu_debugger();

typedef void (*opcode_handler)(void);

typedef struct Instruction {
	opcode_handler handler;
	uint8_t cycles;
}Instruction;
