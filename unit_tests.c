#include "minunit.h"
#include "cartridge.h"
#include "memory.h"
#include "environment.h"
#include "cpu.h"

static cartridge *cartridge_p = NULL;
static memory_map *memory_p = NULL;
static cpu *cpu_p = NULL;

static char *file_name = "Tetris.gb";

void test_setup(void){
    cartridge_p = initialize_cartridge(file_name); 
    memory_p = initialize_memory(cartridge_p);
    cpu_p = initialize_cpu(memory_p);
    initialize_emulator_state(cpu_p, memory_p);
}

void test_teardown(void){
    free(cartridge_p);
    cartridge_p = NULL;

    free(memory_p);
    memory_p = NULL;
    
    free(cpu_p);
    cpu_p = NULL;
}

MU_TEST(test_initialize_cartridge_tetris){

    mu_assert_string_eq((char *) cartridge_p->game_title, "TETRIS");
    mu_check(cartridge_p->cartridge_type == GAMEBOY);
    mu_check(cartridge_p->gameboy_indicator == 0);
    mu_check(cartridge_p->rom_banks * 16 == 32);
    mu_check(cartridge_p->ram_banks == 0);
    mu_check(cartridge_p->gameboy_type == 0);

}

MU_TEST(test_initialize_emulate_state){

    mu_check(cpu_p->PC == 0x100);
    mu_check(cpu_p->register_AF.value == 0x01B0);
    mu_check(cpu_p->register_AF.high == 0x01);
    mu_check(cpu_p->register_AF.low == 0xB0);

    mu_check(cpu_p->register_BC.value == 0x0013);
    mu_check(cpu_p->register_BC.high == 0x00);
    mu_check(cpu_p->register_BC.low == 0x13);
    
    mu_check(cpu_p->register_DE.value == 0x00D8);
    mu_check(cpu_p->register_DE.high == 0x00);
    mu_check(cpu_p->register_DE.low == 0xD8);
    
    mu_check(cpu_p->register_HL.value == 0x014D);
    mu_check(cpu_p->register_HL.high == 0x01);
    mu_check(cpu_p->register_HL.low == 0x4D);

    mu_check(cpu_p->register_SP.value == 0xFFFE);
    mu_check(cpu_p->register_SP.high == 0xFF);
    mu_check(cpu_p->register_SP.low == 0xFE);

    mu_check(memory_p->memory[0xFF05] == 0x00); 
    mu_check(memory_p->memory[0xFF06] == 0x00); 
    mu_check(memory_p->memory[0xFF07] == 0x00); 
    mu_check(memory_p->memory[0xFF10] == 0x80); 
    mu_check(memory_p->memory[0xFF11] == 0xBF); 
    mu_check(memory_p->memory[0xFF12] == 0xF3); 
    mu_check(memory_p->memory[0xFF14] == 0xBF); 
    mu_check(memory_p->memory[0xFF16] == 0x3F); 
    mu_check(memory_p->memory[0xFF17] == 0x00); 
    mu_check(memory_p->memory[0xFF19] == 0xBF); 
    mu_check(memory_p->memory[0xFF1A] == 0x7F); 
    mu_check(memory_p->memory[0xFF1B] == 0xFF); 
    mu_check(memory_p->memory[0xFF1C] == 0x9F); 
    mu_check(memory_p->memory[0xFF1E] == 0xBF); 
    mu_check(memory_p->memory[0xFF20] == 0xFF); 
    mu_check(memory_p->memory[0xFF21] == 0x00); 
    mu_check(memory_p->memory[0xFF22] == 0x00); 
    mu_check(memory_p->memory[0xFF23] == 0xBF); 
    mu_check(memory_p->memory[0xFF24] == 0x77); 
    mu_check(memory_p->memory[0xFF25] == 0xF3);
    mu_check(memory_p->memory[0xFF26] == 0xF1); 
    mu_check(memory_p->memory[0xFF40] == 0x91); 
    mu_check(memory_p->memory[0xFF42] == 0x00); 
    mu_check(memory_p->memory[0xFF43] == 0x00); 
    mu_check(memory_p->memory[0xFF45] == 0x00); 
    mu_check(memory_p->memory[0xFF47] == 0xFC); 
    mu_check(memory_p->memory[0xFF48] == 0xFF); 
    mu_check(memory_p->memory[0xFF49] == 0xFF); 
    mu_check(memory_p->memory[0xFF4A] == 0x00); 
    mu_check(memory_p->memory[0xFF4B] == 0x00); 
    mu_check(memory_p->memory[0xFFFF] == 0x00); 

}


MU_TEST(test_read_memory_normal){
    mu_check(read_memory(memory_p, 0xFF47) == 0xFC);
}

MU_TEST(test_read_memory_rom_no_switch){
    mu_check(read_memory(memory_p, 0x4000) == memory_p->memory[0x4000]);
}

MU_TEST(test_read_memory_external_ram_no_switch){
    mu_check(read_memory(memory_p, 0xA000) == memory_p->memory[0xA000]);
}

MU_TEST(test_write_memory_internal_ram){
    write_memory(memory_p, 0xE000, 0xFF);
    mu_check(memory_p->memory[0xE000] == 0xFF);
    mu_check(memory_p->memory[0xE000 - 0x2000] == 0xFF);
}

MU_TEST(test_write_normal){
    write_memory(memory_p, 0x9000, 0xFF);
    mu_check(memory_p->memory[0x9000] == 0xFF);
}

MU_TEST(test_load_8_bit_immediate){
    
    mu_check(cpu_p->register_BC.high == 0x00);
    cpu_p->PC = 0x9000;
    write_memory(cpu_p->memory_p, 0x9000, 0x06);
    write_memory(cpu_p->memory_p, 0x9001, 0x05);
    
    execute_next_opcode(cpu_p);

    mu_check(cpu_p->register_BC.high == 0x05);

}

MU_TEST(test_load_8_bit){

    mu_check(cpu_p->register_BC.value == 0x0013);    
    mu_check(cpu_p->register_BC.high == 0x00);

    mu_check(cpu_p->register_DE.value == 0x00D8);
    mu_check(cpu_p->register_DE.low == 0xD8);
    cpu_p->PC = 0x9000;
    write_memory(cpu_p->memory_p, 0x9000, 0x43);
    
    execute_next_opcode(cpu_p);

    mu_check(cpu_p->register_BC.high == 0xD8);
    //mu_check(cpu_p->register_BC.value == 0x00D8);

}

// MU_TEST(test_write_memory_no_bank_switch){}
//MU_TEST(test_write_memory_external_ram){}

MU_TEST_SUITE(test_suite){
    printf("\n");
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);
    MU_RUN_TEST(test_initialize_cartridge_tetris);
    MU_RUN_TEST(test_initialize_emulate_state);
    MU_RUN_TEST(test_read_memory_normal);
    MU_RUN_TEST(test_read_memory_rom_no_switch);
    MU_RUN_TEST(test_read_memory_external_ram_no_switch);
    MU_RUN_TEST(test_write_memory_internal_ram);
    MU_RUN_TEST(test_write_normal);  
    MU_RUN_TEST(test_load_8_bit_immediate);
    MU_RUN_TEST(test_load_8_bit);
}

int main (int argc, char *argv[]){
    printf("\n----------------START TEST----------------\n");
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return 0;
}