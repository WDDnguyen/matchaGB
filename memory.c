#include "memory.h"

#define BANK0_INDEX 0x0000
#define SWITCHING_BANK_INDEX 0x4000
#define BANK_SIZE 0x4000
#define ROM_SIZE 0x8000
#define VRAM_INDEX 0x8000
#define VRAM_SIZE 0x2000
#define EXTERNAL_RAM_INDEX 0xA000
#define EXTERNAL_RAM_SIZE 0x2000
#define WRAM_INDEX 0xC000
#define WRAM_SIZE 0x2000
#define WRAM_ECHO_INDEX 0xE000
#define OAM_INDEX 0xFE00
#define IO_PORTS_INDEX 0xFF00
#define HRAM_INDEX 0xFF80
#define INTERRUPT_ENABLE_REGISTER 0xFFFF

static void load_rom_to_memory_map(memory_map *memory_p);
static void print_memory(memory_map *memory_p, word address, word printSize);
static void handle_bank_switching(memory_map *memory_p, word address, byte byte);
static void enable_ram_bank(memory_map *memory_p, word address, byte data, byte mbc_type);
static void switch_rom_bank_low(memory_map *memory_p, byte data, byte mbc_type);
static void switch_rom_bank_high(memory_map *memory_p, byte data);
static void switch_ram_bank(memory_map *memory_p, byte data);
static void select_rom_ram_mode(memory_map *memory_p, byte data);

memory_map *initialize_memory(cartridge *cartride_p){
    
    memory_map * memory_p = calloc(sizeof(memory_map), 1); 
    memory_p->cartridge_p = cartride_p;
    memory_p->current_rom_bank = 1;
    memory_p->current_ram_bank = 0;
    load_rom_to_memory_map(memory_p);
    //print_memory(memory_p, BANK0_INDEX, 50);

    return memory_p;
}

byte read_memory(memory_map *memory_p, word address){
    // reading from ROM switching memory bank
    if ((address >= 0x4000) && (address <= 0x7FFF)){
        word new_address = address - 0x4000;
        //printf("\n READ MEMORY --- CURRENT ROM BANK : %u\n", memory_p->current_rom_bank);
        return memory_p->cartridge_p->cartridge_memory[new_address + (memory_p->current_rom_bank * 0x4000)];
    }
    // reading from External RAM memory bank
    else if ((address >= 0xA000) && (address <= 0xBFFF)){
        word new_address = address - 0xA000;
        //printf("\n READ MEMORY --- CURRENT RAM BANK : %u\n", memory_p->current_ram_bank);
        return memory_p->ram_banks[new_address + (memory_p->current_ram_bank * 0x2000)];
    }
    return memory_p->memory[address];
}

void write_memory(memory_map *memory_p, word address, byte data){
    // addresses 0x0000 - 0x8000 {BANK0, switching BANK N} are read-only memory
    if (address < 0x8000){
        //printf("\n WRITE MEMORY --- BANK SWITCHING\n");
        handle_bank_switching(memory_p, address, data);
    }
    // writing in external RAM bank
    else if ((address >= 0xA000) && (address < 0xC000)){
        if (memory_p->enable_ram){
            word new_address = address - 0xA000;
            memory_p->ram_banks[new_address + (memory_p->current_ram_bank * 0x2000)] = data;
        }
    }
    // addresses 0xE000 - 0xFE00 are echoed with addresses 0xC000-0xE000 (Internal RAM)
    else if ((address >= 0xE000) && (address < 0xFE00)){
        memory_p->memory[address] = data;
        //printf("\n WRITE MEMORY --- ECHO\n");
        write_memory(memory_p, (address - 0x2000), data);
    }

    else if (address == 0xFF04){
        memory_p->memory[address] = 0;
    }
    
    else if (address == 0xFF44){
        memory_p->memory[address] = 0;
    }
    // addresses 0xFEA0 - 0xFEFF {OAM, I/O, HRAM} are restricted
    else if ((address >= 0xFEA0) && (address < 0xFEFF)){
        return;
    } 
    else {
        memory_p->memory[address] = data;
    }
}

static void load_rom_to_memory_map(memory_map *memory_p){
      // load BANK0 in 0x0000 - 0x3FFF and BANK1 in 0x4000 - 0x7FFFF
    if (memory_p->cartridge_p->cartridge_type == 0){
        memcpy(&memory_p->memory, &memory_p->cartridge_p->cartridge_memory, BANK_SIZE * 2);
    }
    // only load BANK0 in 0x000 - 0x3FFF
    else if (memory_p->cartridge_p->cartridge_type == 1 || memory_p->cartridge_p->cartridge_type == 2){
        memcpy(&memory_p->memory, &memory_p->cartridge_p->cartridge_memory, BANK_SIZE);
    }
}

static void handle_bank_switching(memory_map *memory_p, word address, byte data){
    
    byte mbc_type = memory_p->cartridge_p->cartridge_type;
    // RAM bank switching Enabling
    if (address < 0x2000){
        if (mbc_type == MBC1 || mbc_type == MBC2){
            enable_ram_bank(memory_p, address, data, mbc_type);
        }
    }

    // ROM bank switching
    else if ((address >= 0x2000) && (address < 0x4000)){
        if (mbc_type == MBC1 || mbc_type == MBC2){
            switch_rom_bank_low(memory_p, data, mbc_type);
        }
    }

    // ROM or RAM bank switching for MBC1
    else if ((address >= 0x4000) && (address < 0x6000)){
        if (mbc_type == MBC1){
            if (memory_p->rom_banking){
                switch_rom_bank_high(memory_p, data);
            }
            else {
                switch_ram_bank(memory_p, data);
            }
        }
    }

    // this will change if ROM or RAM banking is done on the above if statement
    // this changes the MBC1 mode from either 16/8 or 4/32
    else if ((address >= 0x6000) && (address < 0x8000)){
        if (mbc_type == MBC1){
            select_rom_ram_mode(memory_p, data);
        }
    }
}

static void enable_ram_bank(memory_map *memory_p, word address, byte data, byte mbc_type){
    
    if (mbc_type == MBC2){
        // additional clause that bit 4 of address must be 0 for MBC2
        if (TEST_BIT(address, 4) == 1){
            return;
        }
    }

    byte enable_ram_select = data & 0xF;
    if (enable_ram_select == 0xA){
        memory_p->enable_ram = TRUE;
    }
    else if (enable_ram_select == 0){
        memory_p->enable_ram = FALSE;
    }
}

static void switch_rom_bank_low(memory_map *memory_p, byte data, byte mbc_type){
    // data (XXXXBBBB) need to grab bits 0-3 for MBC2
    if (mbc_type == MBC2){
        memory_p->current_rom_bank = data & 0xF;
        return;
    }

    // data (XXXBBBBB) need to grab bits 0-4 for MBC1
    byte mbc1_bank = data & 31;
    // need to keep bit 5-7 bits while switching bank
    memory_p->current_rom_bank &= 224;
    memory_p->current_rom_bank |= mbc1_bank;
    // ROM select 0 and 1 will pick first bank
    if (memory_p->current_rom_bank == 0){
        memory_p->current_rom_bank++;
    }
}

static void switch_rom_bank_high(memory_map *memory_p, byte data){
    // select ROM bank based on the bit 4 and 5
    // turn off upper 3 bits of current ROM bank
    memory_p->current_rom_bank &= 31;

    // turn off the lower 5 bits of the data
    data &= 224;
    memory_p->current_rom_bank |= data;
    if (memory_p->current_rom_bank == 0){
        memory_p->current_rom_bank++;
    }
}

static void switch_ram_bank(memory_map *memory_p, byte data){
    memory_p->current_ram_bank = data & 0x03;
}

static void select_rom_ram_mode(memory_map *memory_p, byte data){
    memory_p->rom_banking = ((data & 0x1) == 0) ? TRUE : FALSE;
    if (memory_p->rom_banking){
        memory_p->current_ram_bank = 0;
    }
}

static void print_memory(memory_map *memory_p, word address, word print_size){
    
    printf("PRINTING MEMORY ADDRESS : %u, SIZE : %u\n", address, print_size);
    // print in columns
    int column = 0;
    printf("%04x --- ", address);
    for(int i = 0; i < print_size; i++){
        if (column > 15){
            column = 0;
            printf("\n");
            printf("%04x --- ", address + i);
        }
        printf("%02x ", memory_p->memory[address + i]);
        column++;
    }
}