#include "minunit.h"
#include "cartridge.h"
#include "memory.h"
#include "environment.h"
#include "cpu.h"

#define ZERO_FLAG 7
#define SUBTRACT_FLAG 6
#define HALF_CARRY_FLAG 5
#define CARRY_FLAG 4

static cartridge *cartridge_p = NULL;
static memory_map *memory_p = NULL;
static cpu *cpu_p = NULL;
static byte opcode;

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
    mu_check(cpu_p->AF.hi == 0x01);
    mu_check(cpu_p->AF.lo == 0xB0);

    mu_check(cpu_p->BC.hi == 0x00);
    mu_check(cpu_p->BC.lo == 0x13);
    
    mu_check(cpu_p->DE.hi == 0x00);
    mu_check(cpu_p->DE.lo == 0xD8);
    
    mu_check(cpu_p->HL.hi == 0x01);
    mu_check(cpu_p->HL.lo == 0x4D);

    mu_check(cpu_p->SP.hi == 0xFF);
    mu_check(cpu_p->SP.lo == 0xFE);

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

// LD r, n when n == immediate 8 bit
MU_TEST(test_load_immediate_8_bit){
    
    // LD B, n
    opcode = 0x06;    
    mu_check(cpu_p->BC.hi == 0x00);

    cpu_p->PC = 0x9001;
    write_memory(cpu_p->memory_p, 0x9001, 0x05);
    execute_opcode(cpu_p, cpu_p->PC);

    mu_check(cpu_p->BC.hi == 0x05);
}

// LD r1, r2 when r1 == A
MU_TEST(test_load_register_A){
    
    // LD A, B
    opcode = 0x78;
    mu_check(cpu_p->AF.hi == 0x01);
    mu_check(cpu_p->BC.hi == 0x00);

    execute_opcode(cpu_p, 0x78);
    mu_check(cpu_p->AF.hi == 0x00);

    // LD A, (BC)
    opcode = 0x0A;
    write_memory(cpu_p->memory_p, 0x9000, 0xFF);
    cpu_p->BC.hi = 0x90;
    cpu_p->BC.lo = 0x00;

    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->AF.hi == 0xFF);

    // LD A, (nn)
    opcode = 0xFA;
    cpu_p->PC = 0x9000;
    write_memory(cpu_p->memory_p, 0x9000, 0x90);
    write_memory(cpu_p->memory_p, 0x9001, 0x00);
    
    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->AF.hi = 0x90);

    // LD A, (C)
    opcode = 0xF2;
    cpu_p->BC.lo = 0xFF;
    write_memory(cpu_p->memory_p, 0xFFFF, 0xFF);

    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->AF.hi == 0xFF);

}

// LD r1, r2 when r1 == B
MU_TEST(test_load_register_B){
    
    // LD B, A
    opcode = 0x47;
    mu_check(cpu_p->AF.hi == 0x01);
    mu_check(cpu_p->BC.hi == 0x00);

    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->BC.hi == 0x01);
}

// LD r1, r2 when r1 == C
MU_TEST(test_load_register_C){
    
    // LD C, A
    opcode = 0x4F;
    mu_check(cpu_p->AF.hi == 0x01);
    mu_check(cpu_p->BC.lo == 0x13);

    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->BC.lo == 0x01);
}

// LD r1, r2 when r1 == D
MU_TEST(test_load_register_D){
    
    // LD D, A
    opcode = 0x57;
    mu_check(cpu_p->AF.hi == 0x01);
    mu_check(cpu_p->DE.hi == 0x00);

    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->DE.hi == 0x01);
}

// LD r1, r2 when r1 == E
MU_TEST(test_load_register_E){
    
    // LD E, A
    opcode = 0x5F;
    mu_check(cpu_p->AF.hi == 0x01);
    mu_check(cpu_p->DE.lo == 0xD8);

    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->DE.lo == 0x01);
}


// LD r1, r2 when r1 == H
MU_TEST(test_load_register_H){
    
    // LD H, A
    opcode = 0x67;
    mu_check(cpu_p->AF.hi == 0x01);
    mu_check(cpu_p->HL.hi == 0x01);

    cpu_p->HL.hi = 0x05;

    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->HL.hi == 0x01);
}

// LD r1, r2 when r1 == L
MU_TEST(test_load_register_L){
    
    // LD L, A
    opcode = 0x6F;
    mu_check(cpu_p->AF.hi == 0x01);
    mu_check(cpu_p->HL.lo == 0x4D);

    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->HL.lo == 0x01);
}

// LD r1, r2 when r2 == (HL)
MU_TEST(test_load_address_HL){
    
    // LD A, (HL)
    opcode = 0x7E;
    mu_check(cpu_p->AF.hi == 0x01);
    cpu_p->HL.hi = 0x90;
    cpu_p->HL.lo = 0x00;
    write_memory(cpu_p->memory_p, 0x9000, 0xFF);

    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->AF.hi = 0xFF);
}

// LD r1, r2 when r1 == (HL)
MU_TEST(test_write_address_HL){
    
    // LD (HL), A
    opcode = 0x77;
    mu_check(cpu_p->AF.hi == 0x01);
    cpu_p->HL.lo = 0x00;
    cpu_p->HL.hi = 0x90;

    execute_opcode(cpu_p, opcode);

    mu_check(cpu_p->memory_p->memory[get_registers_word(&cpu_p->HL)] == 0x01);
}

// LD r1, r2 when r1 == (HLD)
MU_TEST(test_write_address_HLD){

    // LD (HLD), A
    opcode = 0x32;
    mu_check(cpu_p->AF.hi == 0x01);
    cpu_p->HL.hi = 0x90;
    cpu_p->HL.lo = 0x00;
    

    execute_opcode(cpu_p, opcode);
    word address = get_registers_word(&cpu_p->HL);
    mu_check(cpu_p->memory_p->memory[address + 1] == 0x01);
    mu_check(get_registers_word(&cpu_p->HL) == 0x8FFF);
}

// LD r1, r2 when r1 == (HLI)
MU_TEST(test_write_address_HLI){

    // LD (HLi), A
    opcode = 0x22;
    mu_check(cpu_p->AF.hi == 0x01);
    cpu_p->HL.hi = 0x90;
    cpu_p->HL.lo = 0x00;
    

    execute_opcode(cpu_p, opcode);
    word address = get_registers_word(&cpu_p->HL);
    mu_check(cpu_p->memory_p->memory[address - 1] == 0x01);
    mu_check(get_registers_word(&cpu_p->HL) == 0x9001);
}

// LD r1, r2 when r1 == (n)
MU_TEST(test_write_address_n){

    // LD (n), A
    opcode = 0xE0;
    mu_check(cpu_p->AF.hi == 0x01);
    cpu_p->PC = 0x9000;
    write_memory(cpu_p->memory_p, cpu_p->PC, 0xFF);

    execute_opcode(cpu_p, opcode);
    mu_check(cpu_p->memory_p->memory[0xFFFF] == 0x01);
    mu_check(cpu_p->PC == 0x9001);
}

// LD n, nn when nn == immediate 16 bit
MU_TEST(test_load_immediate_16_bit){
    
    //LD BC, nn
    opcode = 0x01;
    mu_check(get_registers_word(&cpu_p->BC) == 0x0013);
    
    cpu_p->PC = 0x9000;
    write_memory(cpu_p->memory_p, cpu_p->PC, 0xFF);
    write_memory(cpu_p->memory_p, cpu_p->PC + 1, 0xFF);
    
    execute_opcode(cpu_p, opcode);
    mu_check(get_registers_word(&cpu_p->BC) == 0xFFFF);
}

// LD r1, r2 when r1 == SP
MU_TEST(test_load_register_SP){

    // LD SP, HL
    opcode = 0xF9;
    mu_check(get_registers_word(&cpu_p->SP) == 0xFFFE);
    mu_check(get_registers_word(&cpu_p->HL) == 0x014D);
    
    execute_opcode(cpu_p, opcode);
    mu_check(get_registers_word(&cpu_p->SP) == 0x014D);
}

// LDHL HL
MU_TEST(test_load_ldhl){
    opcode = 0xF8;
    mu_check(get_registers_word(&cpu_p->SP) == 0xFFFE);
    
    // no overflow test
    cpu_p->PC = 0x9000;
    write_memory(cpu_p->memory_p, cpu_p->PC, 0x01);

    execute_opcode(cpu_p, opcode);
    mu_check(get_registers_word(&cpu_p->HL) == 0xFFFF);
    mu_check(TEST_BIT(cpu_p->AF.lo, ZERO_FLAG) == 0);
    mu_check(TEST_BIT(cpu_p->AF.lo, SUBTRACT_FLAG) == 0);
    mu_check(TEST_BIT(cpu_p->AF.lo, HALF_CARRY_FLAG) == 0);
    mu_check(TEST_BIT(cpu_p->AF.lo, CARRY_FLAG) == 0);

    //overflow test
    cpu_p->PC = 0x9000;
    write_memory(cpu_p->memory_p, cpu_p->PC, 0x0F);

    execute_opcode(cpu_p, opcode);
    mu_check(get_registers_word(&cpu_p->HL) == 0x000D);
    mu_check(TEST_BIT(cpu_p->AF.lo, ZERO_FLAG) == 0);
    mu_check(TEST_BIT(cpu_p->AF.lo, SUBTRACT_FLAG) == 0);
    mu_check(TEST_BIT(cpu_p->AF.lo, HALF_CARRY_FLAG) > 0);
    mu_check(TEST_BIT(cpu_p->AF.lo, CARRY_FLAG) > 0);

}

MU_TEST(test_write_SP){
    
    opcode = 0x08;
    cpu_p->PC = 0x9000;
    cpu_p->SP.lo = 0xFF;
    cpu_p->SP.hi = 0xFF;
    write_memory(cpu_p->memory_p, cpu_p->PC, 0x90);
    write_memory(cpu_p->memory_p, cpu_p->PC + 1, 0x00);

    execute_opcode(cpu_p, opcode);
    mu_check(read_memory(cpu_p->memory_p, 0x9000) == 0xFF);
    mu_check(read_memory(cpu_p->memory_p, 0x9000) == 0xFF);
    
}


// MU_TEST(test_write_memory_no_bank_switch){}
//MU_TEST(test_write_memory_external_ram){}

MU_TEST_SUITE(test_suite){
    printf("\n");
    // setup
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);
    MU_RUN_TEST(test_initialize_cartridge_tetris);
    MU_RUN_TEST(test_initialize_emulate_state);
    
    // memory tests
    MU_RUN_TEST(test_read_memory_normal);
    MU_RUN_TEST(test_read_memory_rom_no_switch);
    MU_RUN_TEST(test_read_memory_external_ram_no_switch);
    MU_RUN_TEST(test_write_memory_internal_ram);
    MU_RUN_TEST(test_write_normal);  
    
    // instructions tests
    MU_RUN_TEST(test_load_immediate_8_bit);
    MU_RUN_TEST(test_load_register_A);
    MU_RUN_TEST(test_load_register_B);
    MU_RUN_TEST(test_load_register_C);
    MU_RUN_TEST(test_load_register_D);
    MU_RUN_TEST(test_load_register_E);
    MU_RUN_TEST(test_load_register_H);
    MU_RUN_TEST(test_load_register_L);
    MU_RUN_TEST(test_load_address_HL);
    MU_RUN_TEST(test_write_address_HL);
    MU_RUN_TEST(test_write_address_HLD);
    MU_RUN_TEST(test_write_address_HLI);
    MU_RUN_TEST(test_write_address_n);
    MU_RUN_TEST(test_load_immediate_16_bit);
    MU_RUN_TEST(test_load_register_SP);
    MU_RUN_TEST(test_load_ldhl);
    MU_RUN_TEST(test_write_SP);
}

int main (int argc, char *argv[]){
    printf("\n----------------START TEST----------------\n");
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return 0;
}