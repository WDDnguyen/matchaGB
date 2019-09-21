#ifndef __CARTRIDGE_H__
#define __CARTRIDGE_H__

#include "environment.h"

#define CARTRIDGE_MAX_SIZE 0x200000
#define GAME_TITLE_SIZE 0xE
#define NINTENDO_LOGO_SIZE 0x2A

// only supporting MBC1 and MBC2 since most games uses one of these
#define MBC1 1
#define MBC2 2
#define GAMEBOY 0

typedef struct cartridge{
    byte cartridge_memory[CARTRIDGE_MAX_SIZE];
    byte nintendo_logo[NINTENDO_LOGO_SIZE];
    byte gameboy_type;
    byte gameboy_indicator;
    byte rom_banks;
    byte ram_banks;
    byte game_title[GAME_TITLE_SIZE + 1];
    byte cartridge_type;
} cartridge;

cartridge *initialize_cartridge(char *file_name);
void set_nintendo_logo_data(cartridge *cartridge_p);
#endif