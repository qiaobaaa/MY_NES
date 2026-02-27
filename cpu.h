#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct {
    uint8_t  a;    // 累加寄存器 Accumulator
    uint8_t  x;    // 变址寄存器 Index Register X
    uint8_t  y;    // 变址寄存器 Index Register Y
    uint8_t  sp;   // 堆栈指针   Stack Pointer
    uint8_t  p;    // 状态寄存器 Status Register
    uint16_t pc;   // 程序计数器 Program Counter
}CPU;

#endif //BEMU_CPU_H