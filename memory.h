#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "environment.h"
#include "cartridge.h"

#define MEMORY_SIZE 0x10000 
#define RAM_BANK_SIZE 0x8000

#define INTERRUPT_ENABLE_INDEX 0xFFFF
#define INTERRUPT_REQUEST_INDEX 0xFF0F
#define MAX_INTERRUPTS 4

#define TIMA_INDEX 0xFF05
#define TMA_INDEX 0xFF06
#define TMC_INDEX 0xFF07 

#define LCDC_INDEX 0xFF40
#define LCDC_STATUS_INDEX 0xFF41
#define LY_INDEX 0xFF44 // indicate vertical line between 0 - 153 
#define LYC_INDEX 0xFF45

#define SCROLL_Y_INDEX 0xFF42
#define SCROLL_X_INDEX 0xFF43
#define WINDOW_Y_INDEX 0xFF4A
#define WINDOW_X_INDEX 0xFF48

#define TILE_SIZE 16
#define VRAM_INDEX 0x8000

#define BACKGROUND_PALETTE 0xFF47
#define SPRITE_PALETTE_1 0xFF48
#define SPRITE_PALETTE_2 0xFF49

#define OAM_INDEX 0xFE00

typedef struct memory_map{
    cartridge *cartridge_p;
    byte memory[MEMORY_SIZE];
    byte ram_banks[RAM_BANK_SIZE];
    byte current_rom_bank;
    byte current_ram_bank; // ram banking not used in MBC2
    byte enable_ram;
    byte rom_banking;
} memory_map;

memory_map *initialize_memory(cartridge *cartride_p);
byte read_memory(memory_map *memory_p, word address);
void write_memory(memory_map *memory_p, word address, byte byte);
void print_vram_memory(memory_map *memory_p);
void print_tile_map_0(memory_map *memory_p);
void test_nintendo_logo(memory_map *memory_p);
#endif