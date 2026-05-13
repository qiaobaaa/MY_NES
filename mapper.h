#pragma once
#include <stdint.h>
#include <stdbool.h>

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
typedef bool (mapper_cpu_read_fn)(void* mapper, uint16_t addr, uint32_t* mapped_addr, uint8_t* data);
typedef bool (mapper_cpu_write_fn)(void* mapper, uint16_t addr, uint32_t* mapped_addr, uint8_t data);
typedef bool (mapper_ppu_read_fn)(void* mapper, uint16_t addr, uint32_t* mapped_addr);
typedef bool (mapper_ppu_write_fn)(void* mapper, uint16_t addr, uint32_t* mapped_addr);
typedef void (mapper_reset_fn)(void* mapper);
typedef MirrorMode(mapper_mirror_fn)(void* mapper);
typedef bool (mapper_irq_state_fn)(void* mapper);
typedef void (mapper_irq_clear_fn)(void* mapper);
typedef void (mapper_scanline_fn)(void* mapper);

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
} MapperVTable;

// 基础 Mapper 结构体
typedef struct Mapper {
	const MapperVTable* vtable; // 虚函数表指针

	Cartridge* cart;             // 持有 Cartridge 引用
	uint8_t prg_banks;           // PRG ROM bank 数量
	uint8_t chr_banks;           // CHR ROM bank 数量
} Mapper;

// 通用函数（通过 vtable 调用）
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
