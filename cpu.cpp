// 6502 CPU
#include "cpu.h"
#include "memory.h"
#include "stdio.h"

uint64_t cpu_cycles;

/* 用于获得 CPU 状态寄存器中的指定状态, 具体内容见后面的注释 */
#define FLAG_CARRY 0x01
#define FLAG_ZERO 0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL 0x08
#define FLAG_BREAK 0x10
#define FLAG_UNUSED 0x20
#define FLAG_OVERFLOW 0x40
#define FLAG_NEGATIVE 0x80

/* 显示 CPU 寄存器, 时钟等信息 */
void cpu_debugger(CPU* cpu) {
  printf("CPU REGISTERS:\n");
  printf("A: %x\n", cpu->a);
  printf("X: %x\n", cpu->x);
  printf("Y: %x\n", cpu->y);
  printf("SP: %x\n", cpu->sp);
  printf("P Status: %x\n", cpu->status);
  printf("PC: %x\n", cpu->pc);
  printf("\n");
  printf("CPU CLOCK: %llu\n\n", cpu_clock());
}

/* 初始化 CPU */
void cpu_init(CPU cpu) {
  // http://wiki.nesdev.com/w/index.php/CPU_power_up_state
  cpu_cycles = 0;
  uint16_t i;
  cpu->a = 0;
  cpu->x = 0;
  cpu->y = 0;
  cpu->status = 0x24;
  cpu->sp = 0xfd;
  memory_write_byte(0x4017, 0);  // frame irq enabled
  memory_write_byte(0x4015, 0);  // all channels disabled
  for (i = 0x4017; i <= 0x400f; i++) {
    memory_write_byte(i, 0);
  }

  cpu->pc = memory_read_word(0xfffc);
}

/* CPU 复位 */
void cpu_reset(CPU cpu) {
  cpu->sp -= 3;
  cpu->status |= FLAG_INTERRUPT;
  memory_write_byte(0x4015, 0);  // APU was silenced
  cpu->pc = memory_read_word(0xfffc);
}

/* 检查并设置 Zero Flag 与 Negative Flag */
void cpu_checknz(uint8_t n, CPU cpu) {
  if ((n >> 7) & 1) {
    cpu->status |= FLAG_NEGATIVE;
  } else {
    cpu->status &= ~FLAG_NEGATIVE;
  }
  if (n == 0) {
    cpu->status |= FLAG_ZERO;
  } else {
    cpu->status &= ~FLAG_ZERO;
  }
}

/* 修改 Flags */
void cpu_modify_flag(uint8_t flag, int value, CPU cpu) {
  if (value) {
    cpu->status |= flag;
  } else {
    cpu->status &= ~flag;
  }
}

/* 栈操作 */
void cpu_stack_push_byte(uint8_t data, CPU cpu) {
  memory_write_byte(0x100 + cpu->sp, data);
  cpu->sp -= 1;
}
void cpu_stack_push_word(uint16_t data, CPU* cpu) {
  memory_write_word(0x0ff + cpu->sp, data);
  cpu->sp -= 2;
}
uint8_t cpu_stack_pop_byte(CPU* cpu) {
  cpu->sp += 1;
  return memory_read_byte(0x100 + cpu->sp);
}
uint16_t cpu_stack_pop_word(CPU* cpu) {
  cpu->sp += 2;
  return memory_read_word(0x0ff + cpu->sp);
}

/* CPU 寻址方式

参考资料:
http://wiki.nesdev.com/w/index.php/CPU_addressing_modes
http://ewind.us/2015/nes-emu-5-6502-disassembler/
http://nicotine.knight.blog.163.com/blog/static/2692611220089705423961/
http://nicotine.knight.blog.163.com/blog/static/26926112200896032919/
程序代码参考了此处:
https://github.com/NJUOS/LiteNES
*/
/* 存储 CPU 经过寻址后得到的地址和该地址对应的值 */
uint16_t op_address;
uint8_t op_value;
uint8_t additional_cycles;  // 对于某些寻址方式, 如果跨页访问, 需要多使用一个 CPU Cycle

/* implied (1 字节)

隐含寻址. 与累加器寻址类似, 不过指令所需的操作数不在 A 中, 而在其他寄存器中
*/
void cpu_addressing_implied() {
  additional_cycles = 0;
}
/* accumulator (1 字节)

缩写: A
累加器寻址. 指令所需操作数在累加器 A 中, 无需操作数
*/
void cpu_addressing_accumulator() {
  additional_cycles = 0;
}
/* immediate (2 字节)

缩写: #v
立即数寻址. 后面跟一个 8 位的立即数
*/
void cpu_addressing_immediate(CPU cpu) {
  op_value = memory_read_byte(cpu->pc);
  cpu->pc++;
  additional_cycles = 0;
}
/* zeropage (2 字节)

缩写: d
零页寻址. 地址 00 ~ FF 为零页地址
*/
void cpu_addressing_zeropage(CPU cpu) {
  op_address = memory_read_byte(cpu->pc);
  op_value = memory_read_byte(op_address);
  cpu->pc++;
  additional_cycles = 0;
}
/* zeropage, X-indexed (2 字节)

缩写: d,x
使用寄存器 X 的零页寻址. 在零页寻址的基础上, 地址与 X 中的值相加
*/
void cpu_addressing_zeropage_x(CPU cpu) {
  op_address = (memory_read_byte(cpu->pc) + cpu->x) & 0xff;
  op_value = memory_read_byte(op_address);
  cpu->pc++;
  additional_cycles = 0;
}
/* zeropage, Y-indexed (2 字节)

缩写: d,y
使用寄存器 Y 的零页寻址. 在零页寻址的基础上, 地址与 Y 中的值相加
*/
void cpu_addressing_zeropage_y(CPU cpu) {
  op_address = (memory_read_byte(cpu->pc) + cpu->y) & 0xff;
  op_value = memory_read_byte(op_address);
  cpu->pc++;
  additional_cycles = 0;
}
/* absolute (3 字节)

缩写: a
直接寻址. 操作数即为内存地址, 低位在前, 高位在后
*/
void cpu_addressing_absolute(CPU cpu) {
  op_address = memory_read_word(cpu->pc);
  op_value = memory_read_byte(op_address);
  cpu->pc += 2;
  additional_cycles = 0;
}
/* absolute, X-indexed (3 字节)

缩写: a,x
使用寄存器 X 的直接变址寻址. 16 位地址做为基地址, 与寄存器 X 的内容相加
*/
void cpu_addressing_absolute_x(CPU cpu) {
  op_address = memory_read_word(cpu->pc) + cpu->x;
  op_value = memory_read_byte(op_address);
  cpu->pc += 2;
  if ((op_address >> 8) != (cpu->pc >> 8)) {
    additional_cycles = 1;
  } else {
    additional_cycles = 0;
  }
}
/* absolute, Y-indexed (3 字节)

缩写: a,y
使用寄存器 Y 的直接变址寻址. 16 位地址做为基地址, 与寄存器 Y 的内容相加
*/
void cpu_addressing_absolute_y(CPU cpu) {
  op_address = (memory_read_word(cpu->pc) + cpu->y) & 0xffff;
  op_value = memory_read_byte(op_address);
  cpu->pc += 2;
  if ((op_address >> 8) != (cpu->pc >> 8)) {
    additional_cycles = 1;
  } else {
    additional_cycles = 0;
  }
}
/* relative (2 字节)

缩写: label
相对寻址. 用于条件转移指令. 指令第二字节为偏移量, 可正可负.
*/
void cpu_addressing_relative(CPU cpu) {
  op_address = memory_read_byte(cpu->pc);
  cpu->pc++;
  if (op_address & 0x80) { op_address -= 0x100; }
  op_address += cpu->pc;
  if ((op_address >> 8) != (cpu->pc >> 8)) {
    additional_cycles = 1;
  } else {
    additional_cycles = 0;
  }
}