#include "cartridge.h"

#define NINTENDO_LOGO_INDEX 0x104
#define GAME_TITLE_INDEX 0x134
#define GAMEBOY_TYPE_INDEX 0x143
#define GAMEBOY_INDICATOR_INDEX 0x146
#define CARTRIDGE_TYPE_INDEX 0x147
#define ROM_SIZE_INDEX 0x148
#define RAM_SIZE_INDEX 0x149

static void load_cartridge_rom(cartridge *cartridge_p, char *file_name);
static void print_cartridge_values(cartridge *cartridge_p);
static byte get_cartridge_type(byte data);
static byte get_rom_banks(byte data);
static byte get_ram_banks(byte data);

cartridge *initialize_cartridge(char *file_name){

    cartridge *cartridge_p = calloc(sizeof(cartridge), 1);

    load_cartridge_rom(cartridge_p, file_name);

    memcpy(&cartridge_p->nintendo_logo, &cartridge_p->cartridge_memory[NINTENDO_LOGO_INDEX], NINTENDO_LOGO_SIZE);
    memcpy(&cartridge_p->game_title, &cartridge_p->cartridge_memory[GAME_TITLE_INDEX], GAME_TITLE_SIZE);
    cartridge_p->game_title[GAME_TITLE_SIZE] = '\0';

    cartridge_p->gameboy_indicator = cartridge_p->cartridge_memory[GAMEBOY_INDICATOR_INDEX];
    cartridge_p->gameboy_type = cartridge_p->cartridge_memory[GAMEBOY_TYPE_INDEX];
    cartridge_p->rom_banks = get_rom_banks(cartridge_p->cartridge_memory[ROM_SIZE_INDEX]);
    cartridge_p->ram_banks = get_ram_banks(cartridge_p->cartridge_memory[RAM_SIZE_INDEX]);  
    cartridge_p->cartridge_type = get_cartridge_type(cartridge_p->cartridge_memory[CARTRIDGE_TYPE_INDEX]); 

    //print_cartridge_values(cartridge_p); 

    return cartridge_p;
}

static void load_cartridge_rom(cartridge *cartridge_p, char* file_name){

    FILE *rom_file = fopen(file_name, "rb");

    if (rom_file == NULL){
        printf("ERROR : Couldn't open %s \n", file_name);
        exit(1);
    }

    fread(cartridge_p->cartridge_memory, CARTRIDGE_MAX_SIZE, 1, rom_file);
    fclose(rom_file);
}

static byte get_rom_banks(byte data){
    
    byte rom_banks;

    switch(data){
        case 0 : rom_banks = 2; break;
        case 1 : rom_banks = 4; break;
        case 2 : rom_banks = 8; break;
        default : break;
    }

    return rom_banks;
}

static byte get_ram_banks(byte data){
    
    byte ram_banks;

    switch(data){
        case 0 : ram_banks = 0; break;
        case 1 : ram_banks = 1; break;
        case 2 : ram_banks = 1; break;
        default : break;
    }

    return ram_banks;
}

static byte get_cartridge_type(byte data){
    
    byte result;
    
    switch(data){
        case 0 : result = GAMEBOY; break;
        case 1 : result = MBC1; break;
        case 2 : result = MBC1; break;
        case 3 : result = MBC1; break;
        case 5 : result = MBC2; break;
        case 6 : result = MBC2; break;
        default : break;
    }
    return result;
}

static void print_cartridge_values(cartridge *cartridge_p){

    for(int i = 0; i < 100; i++){
        printf("%02x ", cartridge_p->cartridge_memory[i]);
    }
    printf("\n");

    printf("NINTENDO LOGO VALUES : ");
    for (int i = 0; i < NINTENDO_LOGO_SIZE; i++){
        printf("%02x ", cartridge_p->nintendo_logo[i]);
    }
    printf("\n");

    char *title = (char *) cartridge_p->game_title;

    printf("GAME TITLE : %s\n", title);
    printf("GAMEBOY TYPE : %u\n", cartridge_p->gameboy_type);
    printf("GAMEBOY INDICATOR : %u\n", cartridge_p->gameboy_indicator);
    printf("ROM SIZE : %u KB\n", cartridge_p->rom_banks * 16);
    printf("RAM SIZE : %u KB\n", cartridge_p->ram_banks * 16);
    printf("CARTRIDGE TYPE : %u\n", cartridge_p->cartridge_type);
}
