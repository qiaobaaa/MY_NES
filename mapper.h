#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "cartridge.h"
#include "ppu.h"

#define MAPPER_COUNT 7 // 项目中有 7 种 Mapper (0,1,2,3,4,66)

#define PRG_8K 0x2000
#define PRG_16K 0x4000
#define PRG_32K 0x8000

#define CHR_1K 0x400
#define CHR_2K 0x800
#define CHR_4K 0x1000
#define CHR_8K 0x2000

extern bool chr_rom_flag;
// ========== 具体 Mapper 头文件包含 ==========
//#include "mapper_000.h"

// 前向声明
typedef struct Cartridge Cartridge;

// 镜像模式
typedef enum {
	MIRROR_HORIZONTAL,
	MIRROR_VERTICAL,
	MIRROR_ONESCREEN_LO,
	MIRROR_ONESCREEN_HI,
	MIRROR_HARDWARE,
} MirrorMode;

// Mapper 函数指针类型
typedef uint8_t (mapper_cpu_read_fn)(void* mapper, uint16_t addr);
typedef bool (mapper_cpu_write_fn)(void* mapper, uint16_t addr, uint32_t* mapped_addr, uint8_t data);
typedef uint8_t (mapper_ppu_read_fn)(void* mapper, uint16_t addr);
typedef bool (mapper_ppu_write_fn)(void* mapper, uint16_t addr, uint32_t* mapped_addr);
typedef void (mapper_reset_fn)(void* mapper);
typedef MirrorMode(mapper_mirror_fn)(void* mapper);
typedef bool (mapper_irq_state_fn)(void* mapper);
typedef void (mapper_irq_clear_fn)(void* mapper);
typedef void (mapper_scanline_fn)(void* mapper);
typedef void (mapper_register_write_fn)(void* mapper, uint16_t addr, uint8_t data);
typedef void (mapper_irq_trigger_fn)(void* mapper);

// 虚函数表
typedef struct MapperVTable {
	mapper_cpu_read_fn* cpu_read;
	mapper_cpu_write_fn* cpu_write;
	mapper_ppu_read_fn* ppu_read;
	mapper_ppu_write_fn* ppu_write;
	mapper_reset_fn* reset;
	mapper_mirror_fn* mirror;
	mapper_irq_state_fn* irq_state;
	mapper_irq_clear_fn* irq_clear;
	mapper_scanline_fn* scanline;
	mapper_register_write_fn* register_write;
	mapper_irq_trigger_fn* irq_trigger;
} MapperVTable;

// 基础 Mapper 结构体
typedef struct Mapper {
	const MapperVTable* vtable; // 虚函数表指
	Cartridge* cart;             // 持有 Cartridge 引用
	uint8_t prg_banks;           // PRG ROM bank 数量
	uint8_t chr_banks;           // CHR ROM bank 数量
	uint8_t prg_ram_protect;
} Mapper;

// 通用函数（通过 vtable 调用）
/*
static inline bool mapper_cpu_read(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr, uint8_t* data) {
	return mapper->vtable->cpu_read(mapper, addr, mapped_addr, data);
}

static inline bool mapper_cpu_write(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr, uint8_t data) {
	return mapper->vtable->cpu_write(mapper, addr, mapped_addr, data);
}

static inline bool mapper_ppu_read(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr) {
	return mapper->vtable->ppu_read(mapper, addr, mapped_addr);
}

static inline bool mapper_ppu_write(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr) {
	return mapper->vtable->ppu_write(mapper, addr, mapped_addr);
}

static inline void mapper_reset(Mapper* mapper) {
	mapper->vtable->reset(mapper);
}

static inline MirrorMode mapper_mirror(Mapper* mapper) {
	return mapper->vtable->mirror(mapper);
}

static inline bool mapper_irq_state(Mapper* mapper) {
	return mapper->vtable->irq_state(mapper);
}

static inline void mapper_irq_clear(Mapper* mapper) {
	mapper->vtable->irq_clear(mapper);
}

static inline void mapper_scanline(Mapper* mapper) {
	mapper->vtable->scanline(mapper);
}

static inline void mapper_register_write(Mapper* mapper, uint16_t addr, uint8_t data) {
	mapper->vtable->register_write(mapper, addr, data);
}
*/