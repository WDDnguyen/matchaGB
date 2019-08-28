#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "environment.h"
#include "cartridge.h"

#define MEMORY_SIZE 0x10000 
#define RAM_BANK_SIZE 0x8000

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

#endif