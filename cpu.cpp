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

/* indirect (3 字节)
缩写: (a)
间接寻址. 对应地址内存单元中的数做为地址.
*/
void cpu_addressing_indirect(CPU cpu) {
  uint16_t arg_addr = memory_read_word(cpu->pc, cpu);

  /* 据说这是 6502 的 Bug */
  if ((arg_addr & 0xff) == 0xff) {
    // 有 Bug 的情况下
    op_address = (memory_read_byte(arg_addr & 0xff00, cpu) << 8) + memory_read_byte(arg_addr, cpu);
  } else {
    // 正常情况下
    op_address = memory_read_word(arg_addr, cpu);
  }
  cpu->pc += 2;
  additional_cycles = 0;
}

/* indirect, X-indexed (2 字节)
缩写: (d,x)
先变址 X 后间接寻址. 以 X 做为变址, 与基地址相加, 然后间接寻址
*/
void cpu_addressing_indirect_x(CPU cpu) {
  uint8_t arg_addr = memory_read_byte(cpu->pc, cpu);
  op_address = (memory_read_byte((arg_addr + cpu->x + 1) & 0xff, cpu) << 8) | memory_read_byte((arg_addr + cpu->x) & 0xff, cpu);
  op_value = memory_read_byte(op_address, cpu);
  cpu->pc++;
  additional_cycles = 0;
}

/* indirect, Y-indexed (2 字节)
缩写: (d),y
后变址 Y 间接寻址. 对操作数中的零页地址先做一次间接寻址, 得到 16 位地址, 再与 Y 相加, 对相加后得到的地址进行直接寻址.
*/
void cpu_addressing_indirect_y(CPU cpu) {
  uint8_t arg_addr = memory_read_byte(cpu->pc, cpu);
  op_address = (((memory_read_byte((arg_addr + 1) & 0xff, cpu) << 8) | memory_read_byte(arg_addr, cpu)) + cpu->y) & 0xffff;
  op_value = memory_read_byte(op_address, cpu);
  cpu->pc++;
  if ((op_address >> 8) != (cpu->pc >> 8)) {
    additional_cycles = 1;
  } else {
    additional_cycles = 0;
  }
}

/* CPU 指令 ************************************************************/

/* ALU ******/
void cpu_ora(CPU* cpu) {
  cpu->a |= op_value;
  cpu_checknz(cpu->a, cpu);
}

void cpu_and(CPU* cpu) {
  cpu->a &= op_value;
  cpu_checknz(cpu->a, cpu);
}

void cpu_eor(CPU* cpu) {
  cpu->a ^= op_value;
  cpu_checknz(cpu->a, cpu);
}

void cpu_asl(CPU* cpu) {
  cpu_modify_flag(FLAG_CARRY, op_value & 0x80, cpu);
  op_value <<= 1;
  cpu_checknz(op_value, cpu);
  memory_write_byte(op_address, op_value, cpu);
}

void cpu_asla(CPU* cpu) {
  cpu_modify_flag(FLAG_CARRY, cpu->a & 0x80, cpu);
  cpu->a <<= 1;
  cpu_checknz(cpu->a, cpu);
}

void cpu_rol(CPU* cpu) {
  uint8_t tmp = cpu->status & FLAG_CARRY;
  cpu_modify_flag(FLAG_CARRY, op_value & 0x80, cpu);
  op_value <<= 1;
  op_value |= tmp ? 1 : 0;
  memory_write_byte(op_address, op_value, cpu);
  cpu_checknz(op_value, cpu);
}

void cpu_rola(CPU* cpu) {
  uint8_t tmp = cpu->status & FLAG_CARRY;
  cpu_modify_flag(FLAG_CARRY, cpu->a & 0x80, cpu);
  cpu->a <<= 1;
  cpu->a |= tmp ? 1 : 0;
  cpu_checknz(cpu->a, cpu);
}

void cpu_ror(CPU* cpu) {
  uint8_t tmp = cpu->status & FLAG_CARRY;
  cpu_modify_flag(FLAG_CARRY, op_value & 0x01, cpu);
  op_value >>= 1;
  op_value |= (tmp ? 1 : 0) << 7;
  memory_write_byte(op_address, op_value, cpu);
  cpu_checknz(op_value, cpu);
}

void cpu_rora(CPU* cpu) {
  uint8_t tmp = cpu->status & FLAG_CARRY;
  cpu_modify_flag(FLAG_CARRY, cpu->a & 0x01, cpu);
  cpu->a >>= 1;
  cpu->a |= (tmp ? 1 : 0) << 7;
  cpu_checknz(cpu->a, cpu);
}

void cpu_lsr(CPU* cpu) {
  cpu_modify_flag(FLAG_CARRY, op_value & 0x01, cpu);
  op_value >>= 1;
  memory_write_byte(op_address, op_value, cpu);
  cpu_checknz(op_value, cpu);
}

void cpu_lsra(CPU* cpu) {
  cpu_modify_flag(FLAG_CARRY, cpu->a & 0x01, cpu);
  cpu->a >>= 1;
  cpu_checknz(cpu->a, cpu);
}

void cpu_adc(CPU* cpu) {
  uint16_t tmp;
  tmp = op_value + cpu->a + ((cpu->status & FLAG_CARRY) ? 1 : 0);
  cpu_modify_flag(FLAG_CARRY, tmp & 0xff00, cpu);
  cpu_modify_flag(FLAG_OVERFLOW, ((op_value ^ tmp) & (cpu->a ^ tmp)) & 0x80, cpu);
  cpu->a = (uint8_t)(tmp & 0xff);
  cpu_checknz(cpu->a, cpu);
}

void cpu_sbc(CPU* cpu) {
  uint16_t tmp;
  tmp = cpu->a - op_value - (1 - ((cpu->status & FLAG_CARRY) ? 1 : 0));
  cpu_modify_flag(FLAG_CARRY, (tmp & 0xff00) == 0, cpu);
  cpu_modify_flag(FLAG_OVERFLOW, ((cpu->a ^ op_value) & (cpu->a ^ tmp)) & 0x80, cpu);
  cpu->a = (uint8_t)(tmp & 0xff);
  cpu_checknz(cpu->a, cpu);
}

/* Branching ******/

void cpu_bmi(CPU* cpu) {
  if (cpu->status & FLAG_NEGATIVE) { cpu->pc = op_address; }
}
void cpu_bcs(CPU* cpu) {
  if (cpu->status & FLAG_CARRY) { cpu->pc = op_address; }
}
void cpu_beq(CPU* cpu) {
  if (cpu->status & FLAG_ZERO) { cpu->pc = op_address; }
}
void cpu_bvs(CPU* cpu) {
  if (cpu->status & FLAG_OVERFLOW) { cpu->pc = op_address; }
}

void cpu_bpl(CPU* cpu) {
  if (!(cpu->status & FLAG_NEGATIVE)) { cpu->pc = op_address; }
}
void cpu_bcc(CPU* cpu) {
  if (!(cpu->status & FLAG_CARRY)) { cpu->pc = op_address; }
}
void cpu_bne(CPU* cpu) {
  if (!(cpu->status & FLAG_ZERO)) { cpu->pc = op_address; }
}
void cpu_bvc(CPU* cpu) {
  if (!(cpu->status & FLAG_OVERFLOW)) { cpu->pc = op_address; }
}

/* Comapre ******/

void cpu_bit(CPU* cpu) {
  cpu_modify_flag(FLAG_OVERFLOW, op_value & 0x40, cpu);
  cpu_modify_flag(FLAG_NEGATIVE, op_value & 0x80, cpu);
  cpu_modify_flag(FLAG_ZERO, !(op_value & cpu->a), cpu);
}

void cpu_cmp(CPU* cpu) {
  int tmpc = cpu->a - op_value;
  cpu_modify_flag(FLAG_CARRY, tmpc >= 0, cpu);
  cpu_checknz((uint8_t)tmpc, cpu);
}

void cpu_cpx(CPU* cpu) {
  int tmpc = cpu->x - op_value;
  cpu_modify_flag(FLAG_CARRY, tmpc >= 0, cpu);
  cpu_checknz((uint8_t)tmpc, cpu);
}

void cpu_cpy(CPU* cpu) {
  int tmpc = cpu->y - op_value;
  cpu_modify_flag(FLAG_CARRY, tmpc >= 0, cpu);
  cpu_checknz((uint8_t)tmpc, cpu);
}

/* Flag ******/

void cpu_clc(CPU* cpu) {
  cpu_modify_flag(FLAG_CARRY, 0, cpu);
}
void cpu_cli(CPU* cpu) {
  cpu_modify_flag(FLAG_INTERRUPT, 0, cpu);
}
void cpu_cld(CPU* cpu) {
  cpu_modify_flag(FLAG_DECIMAL, 0, cpu);
}
void cpu_clv(CPU* cpu) {
  cpu_modify_flag(FLAG_OVERFLOW, 0, cpu);
}
void cpu_sec(CPU* cpu) {
  cpu_modify_flag(FLAG_CARRY, 1, cpu);
}
void cpu_sei(CPU* cpu) {
  cpu_modify_flag(FLAG_INTERRUPT, 1, cpu);
}
void cpu_sed(CPU* cpu) {
  cpu_modify_flag(FLAG_DECIMAL, 1, cpu);
}

/* Inc & Dec ******/

void cpu_dec(CPU* cpu) {
  uint8_t tmp = op_value - 1;
  memory_write_byte(op_address, tmp, cpu);
  cpu_checknz(tmp, cpu);
}

void cpu_dex(CPU* cpu) {
  cpu->x--;
  cpu_checknz(cpu->x, cpu);
}

void cpu_dey(CPU* cpu) {
  cpu->y--;
  cpu_checknz(cpu->y, cpu);
}

void cpu_inc(CPU* cpu) {
  uint8_t tmp = op_value + 1;
  memory_write_byte(op_address, tmp, cpu);
  cpu_checknz(tmp, cpu);
}

void cpu_inx(CPU* cpu) {
  cpu->x++;
  cpu_checknz(cpu->x, cpu);
}

void cpu_iny(CPU* cpu) {
  cpu->y++;
  cpu_checknz(cpu->y, cpu);
}

/* Load & Store ******/

void cpu_lda(CPU* cpu) {
  cpu->a = op_value;
  cpu_checknz(cpu->a, cpu);
}
void cpu_ldx(CPU* cpu) {
  cpu->x = op_value;
  cpu_checknz(cpu->x, cpu);
}
void cpu_ldy(CPU* cpu) {
  cpu->y = op_value;
  cpu_checknz(cpu->y, cpu);
}
void cpu_sta(CPU* cpu) {
  memory_write_byte(op_address, cpu->a, cpu);
}
void cpu_stx(CPU* cpu) {
  memory_write_byte(op_address, cpu->x, cpu);
}
void cpu_sty(CPU* cpu) {
  memory_write_byte(op_address, cpu->y, cpu);
}

/* Misc ******/

void cpu_nop() {}

/* Stack & Jump ******/

void cpu_pha(CPU* cpu) {
  cpu_stack_push_byte(cpu->a, cpu);
}
void cpu_php(CPU* cpu) {
  cpu_stack_push_byte(cpu->status | 0x30, cpu);
}
void cpu_pla(CPU* cpu) {
  cpu->a = cpu_stack_pop_byte(cpu);
  cpu_checknz(cpu->a, cpu);
}
void cpu_plp(CPU* cpu) {
  cpu->status = (cpu_stack_pop_byte(cpu) & 0xef) | 0x20;
}
void cpu_rts(CPU* cpu) {
  cpu->pc = cpu_stack_pop_word(cpu) + 1;
}
void cpu_rti(CPU* cpu) {
  cpu->status = cpu_stack_pop_byte(cpu) | FLAG_UNUSED;
  cpu->pc = cpu_stack_pop_word(cpu);
}
void cpu_jmp(CPU* cpu) {
  cpu->pc = op_address;
}
void cpu_jsr(CPU* cpu) {
  cpu_stack_push_word(cpu->pc - 1, cpu);
  cpu->pc = op_address;
}
void cpu_brk(CPU* cpu) {
  cpu_stack_push_word(cpu->pc - 1, cpu);
  cpu_stack_push_byte(cpu->status, cpu);
  cpu->status |= FLAG_UNUSED | FLAG_BREAK;
  cpu->pc = memory_read_word(0xfffa, cpu);  // NMI 中断
}
