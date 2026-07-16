#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "cartridge.h"

#define MAPPER_COUNT 7

/* ==================== 前向声明 ==================== */
//typedef struct Mapper Mapper;
//typedef struct MapperVTable MapperVTable;

/* ==================== 枚举类型 ==================== */
typedef enum {
	MIRROR_HORIZONTAL,
	MIRROR_VERTICAL,
	MIRROR_ONESCREEN_LO,
	MIRROR_ONESCREEN_HI,
	MIRROR_HARDWARE,
} MirrorMode;

typedef struct Mapper Mapper;
typedef struct MapperVTable MapperVTable;


/* ==================== 虚函数类型（统一用 Mapper*）==================== */
typedef bool(mapper_cpu_read_fn)(Mapper* self, uint16_t addr, uint32_t* mapped_addr, uint8_t* data);
typedef bool(mapper_cpu_write_fn)(Mapper* self, uint16_t addr, uint32_t* mapped_addr, uint8_t data);
typedef bool(mapper_ppu_read_fn)(Mapper* self, uint16_t addr, uint32_t* mapped_addr);
typedef bool(mapper_ppu_write_fn)(Mapper* self, uint16_t addr, uint32_t* mapped_addr);
typedef void(mapper_reset_fn)(Mapper* self);
typedef MirrorMode(mapper_mirror_fn)(Mapper* self);
typedef bool(mapper_irq_state_fn)(Mapper* self);
typedef void(mapper_irq_clear_fn)(Mapper* self);
typedef void(mapper_scanline_fn)(Mapper* self);

/* ==================== 虚函数表 ==================== */
struct MapperVTable {
	mapper_cpu_read_fn* cpu_read;
	mapper_cpu_write_fn* cpu_write;
	mapper_ppu_read_fn* ppu_read;
	mapper_ppu_write_fn* ppu_write;
	mapper_reset_fn* reset;
	mapper_mirror_fn* mirror;
	mapper_irq_state_fn* irq_state;
	mapper_irq_clear_fn* irq_clear;
	mapper_scanline_fn* scanline;
};

/* ==================== 基础结构体（不透明）==================== */
struct Mapper {
	const MapperVTable* vtable;
	Cartridge* cart;
	uint8_t prg_banks;
	uint8_t chr_banks;
};

/* ==================== 公共 API ==================== */
//Mapper mapper_create(uint8_t mapper_id, Cartridge* cart, uint8_t prg_banks, uint8_t chr_banks);
//void mapper_destroy(Mapper* mapper);

bool mapper_cpu_read(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr, uint8_t* data);
bool mapper_cpu_write(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr, uint8_t data);
bool mapper_ppu_read(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr);
bool mapper_ppu_write(Mapper* mapper, uint16_t addr, uint32_t* mapped_addr);
void mapper_reset(Mapper* mapper);
MirrorMode mapper_mirror(Mapper* mapper);
bool mapper_irq_state(Mapper* mapper);
void mapper_irq_clear(Mapper* mapper);
void mapper_scanline(Mapper* mapper);