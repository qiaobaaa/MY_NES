/* NES 内存操作
*/

/* NES Memory Map

http://wiki.nesdev.com/w/index.php/CPU_memory_map
地址范围 | Size | 说明
------------+------+----------------------------------
0000 ~ 07FF | 0800 | 2KB 内部 RAM
0800 ~ 0FFF | 0800 | 0000 ~ 07FF 的镜像
1000 ~ 17FF | 0800 | 0000 ~ 07FF 的镜像
1800 ~ 1FFF | 0800 | 0000 ~ 07FF 的镜像
2000 ~ 2007 | 0008 | PPU 寄存器
2008 ~ 3FFF | 1ff8 | 2000 ~ 2007 的镜像 (每 8 字节重复一次)
4000 ~ 401F | 0020 | APU 与 IO 寄存器
4020 ~ FFFF | BFE0 | 卡带所使用的内存区域
------------+------+----------------------------------
卡带使用的内存区域:
4020 ~ 5FFF | 1FE0 | Expansion ROM
6000 ~ 7FFF | 2000 | SRAM (Save RAM)
8000 ~ FFFF | 8000 | PRG ROM
*/
/* 中断向量:

FFFA ~ FFFB: NMI vector
FFFC ~ FFFD: Reset vector
FFFE ~ FFFF: IRQ/BRK vector
*/
/* PPU 寄存器:
* 2000: PPU Control Register 1
* 2001: PPU Control Register 2
* 2002: PPU Status Register
* 2003: SPR-RAM Address Register
* 2004: SPR-RAM I/O Register
* 2005: VRAM Address Register 1
* 2006: VRAM Address Register 2
* 2007: VRAM I/O Register
*
* APU 寄存器:
* 4000: pAPU Pulse 1 Control Register
* 4001: pAPU Pulse 1 Ramp Control Register
* 4002: pAPU Pulse 1 Fine Tune (FT) Register
* 4003: pAPU Pulse 1 Coarse Tune (CT) Register
* 4004: pAPU Pulse 2 Control Register
* 4005: pAPU Pulse 2 Ramp Control Register
* 4006: pAPU Pulse 2 Fine Tune Register
* 4007: pAPU Pulse 2 Coarse Tune Register
* 4008: pAPU Triangle Control Register 1
* 4009: pAPU Triangle Control Register 2
* 400A: pAPU Triangle Frequency Register 1
* 400B: pAPU Triangle Frequency Register 2
* 400C: pAPU Noise Control Register 1
* 400E: pAPU Noise Frequency Register 1
* 400F: pAPU Noise Frequency Register 2
* 4010: pAPU Delta Modulation Control Register
* 4011: pAPU Delta Modulation D/A Register
* 4012: pAPU Delta Modulation Address Register
* 4013: pAPU Delta Modulation Data Length Register
* 4014: Sprite DMA Register
* 4015: pAPU Sound/Vertical Clock Signal Register
* 4016: Controller 1
* 4017: Controller 2
/
/

CPU内部工作内存（RAM）
地址范围：$0000-$07FF（2KB，但实际硬件上是2KB镜像扩展到$0800-$1FFF，模拟器通常只模拟2KB物理内存）
读写性质：完全可读写
用途：
存储程序运行时数据、堆栈（$0100-$01FF）、零页变量（$0000-$00FF，因寻址方式更快）
模拟器需处理镜像地址（如$0800对应$0000）。
扩展：部分mapper（如MMC5）可映射额外RAM到$6000-$7FFF，但这是卡带扩展区。
PPU（图像处理单元）寄存器
地址范围：$2000-$2007（PPU寄存器核心区）
镜像区：$2008-$3FFF（每8字节镜像重复，如$2008→$2000）
读写性质：
只写寄存器：$2000（PPU控制寄存器）、$2001（PPU屏蔽寄存器）、$2003（OAM地址）、$2005（滚动位置）、$2006（PPU地址）、$2007（数据端口写）
只读寄存器：$2002（PPU状态寄存器，用于清除写标志和检测VBlank）
读写寄存器：无（所有PPU寄存器写操作有副作用，读操作大多返回内部状态）
间接访问区域（通过PPU寄存器操作）：
VRAM（视频RAM）：$3F00-$3FFF（调色板数据）
需通过$2006（地址）和$2007（数据）写入/读取。
OAM（精灵属性内存）：$0000-$03FF（PPU内部256字节）
通过$2003和$2004访问（直接OAM）或通过DMA（$4014）批量传输。
APulse/音频处理单元（APU）及扩展硬件
地址范围：$4000-$401F
关键寄存器：
$4000-$4013：音频通道控制寄存器（脉冲、锯齿、三角波、噪声、DMC）
性质：只写（控制音频参数）
$4014：OAM DMA传输寄存器
性质：只写（触发将CPU内存页数据传输到PPU OAM）
$4015：音频通道状态寄存器
性质：读写
读：获取当前音频通道启用状态
写：启用/禁用音频通道
$4016：手柄输入端口1
读写：
写入$4016的bit0控制读取游戏手柄方式（0=读取手柄1，1=读取手柄2）
读取$4016获取手柄1状态（连续读取两次获取完整8位数据）
$4017：手柄输入端口2/帧计数器控制
读写：
读取获取手柄2状态（手柄1由$4016控制）
写入控制APU帧计时器模式（IRQ产生方式）
卡带映射区域（Cartridge / Mapper）
地址范围：$6000-$7FFF（部分卡带使用）
部分mapper允许卡带自带RAM（SRAM）用于存档，需电池维护（$6000-$7FFF可读写）
部分mapper将扩展内存映射到这里（如MMC5的扩展RAM）
性质：可读写（取决于mapper硬件）
$8000-$FFFF：程序只读存储器（ROM）
通常只读，但部分mapper（如Action 53）支持写入（如bankswitching切换可写页面，但实际是写寄存器控制bankswitching，而非直接写PRG ROM）
特殊只读区域
$FFFA-$FFFF：中断向量表（不可写入，但模拟器初始化时需设置）
$FFFA-$FFFB：NMI向量
$FFFC-$FFFD：复位向量
$FFFE-$FFFF：IRQ/BRK向量
常见误解与补充说明
CPU不能直接读取VRAM：需通过PPU寄存器间接访问（$2006/$2007）。
OAM访问限制：直接OAM读写（$2003/$2004）较慢，通常用DMA（$4014）快速传输。
手柄读取时序：需连续读取两次$4016/$4017，第一次为移位寄存器准备，第二次获取完整数据。
Mapper寄存器：部分mapper使用非标准地址（如$5000-$5FFF），但CPU对这些区域的访问由卡带硬件决定，模拟器需实现对应mapper逻辑。
APU I/O端口：$4016-$4017不仅是手柄输入，也涉及音频帧计数器，模拟器需处理时序。
总结：CPU可读写区域分类
地址范围
区域类型
读写性质
$0000-$07FF
CPU工作RAM
完全可读写
$0800-$1FFF
RAM镜像
镜像至$0000-$07FF
$2000-$2007
PPU核心寄存器
主要只写（$2002只读）
$2008-$3FFF
PPU寄存器镜像
同$2000-$2007
$4000-$401F
APU/输入寄存器
混合读写（音频只写，$4015/$4016-$4017可读写）
$6000-$7FFF
卡带扩展RAM/ROM
可读写（取决于mapper）
$8000-$FFFF
程序ROM
通常只读（部分mapper除外）
开发建议
实现镜像地址：确保$0800-$1FFF正确映射到$0000-$07FF。
PPU寄存器时序：写入$2006后，下一次写$2007才会访问VRAM。
手柄输入：模拟器需模拟“时钟移位”机制，连续读取两次返回完整数据。
Mapper支持：根据游戏选择对应mapper，模拟bankswitching逻辑。
*/

#include "memory.h"
//#include "ppu.h"
//#include "io.h"
//#include "cpu.h"

uint8_t* prg_rom_ptr;
uint8_t* chr_rom_ptr;
int prg_rom_size;

uint8_t interal_ram[0x0800];  // 0000 ~ 07FF
uint8_t save_ram[0x2000];     // 6000 ~ 7FFF

void memory_init(uint8_t* prg_rom, int prg_rom_length) {
  prg_rom_ptr = prg_rom;
  prg_rom_size = prg_rom_length;
}

uint8_t memory_read_byte(uint16_t address, CPU* cpu) {
  switch (address >> 13) {
    case 0:  // 0000 ~ 1FFF, 内部 RAM
      //return interal_ram[address % 0x0800];
      return cpu->ram[address % 0x0800];
    case 1:  // 2000 ~ 3FFF, PPU 寄存器
    //return ppu_io_read(address);
    case 2:  // APU 与 IO 寄存器
    //return io_read(address);
    case 3:  // Save RAM
      //return save_ram[address - 0x6000];
      return cpu->sram[address - 0x6000];
    default:  // PRG ROM
      //return prg_rom_ptr[(address - 0x8000) % prg_rom_size];
      //return cpu->cart.mapper->vtable->cpu_read();
      return 0;
  }
}

uint16_t memory_read_word(uint16_t address, CPU* cpu) {
  return memory_read_byte(address, cpu) + (memory_read_byte(address + 1, cpu) << 8);
}

void memory_write_byte(uint16_t address, uint8_t data, CPU* cpu) {
  /*
int i; uint16_t tmp;
// DMA 传输
if (address == 0x4014) {
for (i = 0; i < 256; i++) {
tmp = (0x100 * data) + i;
if ((tmp >> 13) == 0) {
ppu_sprram_write(interal_ram[tmp % 0x0800]);
}
else {
ppu_sprram_write(prg_rom_ptr[(address - 0x8000) % prg_rom_size]);
}
}
return;
}
*/
  switch (address >> 13) {
    case 0:  // 0000 ~ 1FFF, 内部 RAM
      //interal_ram[address % 0x0800] = data;
      cpu->ram[address % 0x0800] = data;
      break;
    case 1:  // 2000 ~ 3FFF, PPU 寄存器
      //ppu_io_write(address, data);
      break;
    case 2:  // APU 与 IO 寄存器
      //io_write(address, data);
      break;
    case 3:  // Save RAM
      //save_ram[address - 0x6000] = data;
      cpu->sram[address - 0x6000] = data;
      break;
    default:  // PRG ROM
      //prg_rom_ptr[(address - 0x8000) % prg_rom_size] = data;
      break;
  }
}

void memory_write_word(uint16_t address, uint16_t data, CPU* cpu) {
  memory_write_byte(address, data & 0xFF, cpu);
  memory_write_byte(address + 1, data >> 8, cpu);
}
