#include "cpu.h"

#define ZERO_FLAG 7
#define SUBTRACT_FLAG 6
#define HALF_CARRY_FLAG 5
#define CARRY_FLAG 4

static void load_immediate_8_bit(cpu *cpu_p, byte *register_p);
static void load_8_bit(byte *p, byte data);
static byte get_immediate_8_bit(cpu *cpu_p);
static void load_immediate_16_bit(cpu *cpu_p, cpu_register *register_p);
static word get_immediate_16_bit(cpu *cpu_p);
static void set_registers_word(cpu_register *register_p, word data);
static void load_hl(cpu *cpu_p, cpu_register *AF_p, cpu_register *HL_p);
static void add_8_bit(cpu_register *register_p, byte data);
static void add_carry_8_bit(cpu_register *AF_p, byte data);
static void sub_8_bit(cpu_register *AF_p, byte data);
static void sub_carry_8_bit(cpu_register *AF_p, byte data);
static void and_8_bit(cpu_register *AF_p, byte data);
static void or_8_bit(cpu_register *AF_p, byte data);
static void xor_8_bit(cpu_register *AF_p, byte data);
static void cp_8_bit(cpu_register *AF_p, byte data);
static void inc_8_bit(byte *register_p, cpu_register *AF_p);
static void inc_memory_8_bit(cpu *register_p, cpu_register *AF_p);
static void dec_memory_8_bit(cpu *register_p, cpu_register *AF_p);
static void dec_8_bit(byte *register_p, cpu_register *AF_p);
static void add_16_bit_hl(cpu_register *HL_p, cpu_register *AF_p, word data);
static void add_16_bit_sp(cpu *cpu_p, cpu_register *SP_p, cpu_register *AF_p);
static void increment(cpu_register *register_p);
static void decrement(cpu_register *register_p);
static void inc_16_bit(cpu_register *register_p);
static void dec_16_bit(cpu_register *register_p);
static void swap_nibble(byte *register_p, byte *F_p);
static void swap_nibble_memory(memory_map *memory_p, byte *F_p, word address);
static int execute_extended_opcode(cpu *cpu_p);
static void cpl(cpu_register *AF_p);
static void ccf(byte *F_p);
static void scf(byte *F_p);
static void daa(cpu_register *AF_p);
static void nop();
static void halt(cpu *cpu_p);
static void stop(cpu *cpu_p);
static void di(cpu *cpu_p);
static void ei(cpu *cpu_p);
static void rrc(byte *R_p, cpu_register *AF_p);
static void rrc_memory(memory_map *memory_p, word address, cpu_register *AF_p);
static void rr(byte *R_p, byte *F_p);
static void rr_memory(memory_map *memory_p, word address, byte *F_p);
static void rlc(byte *R_p, cpu_register *AF_p);
static void rlc_memory(memory_map *memory_p, word address, cpu_register *AF_p);
static void rl(byte *R_p, byte *F_p);
static void rl_memory(memory_map *memory_p, word address, byte *F_p);
static void sla(byte *R_p, byte *F_p);
static void sla_memory(memory_map *memory_p, word address, byte *F_p);
static void sra(byte *R_p, byte *F_p);
static void sra_memory(memory_map *memory_p, word address, byte *F_p);
static void srl(byte *R_p, byte *F_p);
static void srl_memory(memory_map *memory_p, word address, byte *F_p);
static void bit(byte position, byte *R_p, byte *F_p);
static void bit_memory(byte position, memory_map *memory_p, word address, byte *F_p);
static void set(byte position, byte *R_p);
static void set_memory(byte position, memory_map *memory_p, word address);
static void res(byte position, byte *R_p);
static void res_memory(byte position, memory_map *memory_p, word address);
static void jp(cpu *cpu_p, byte *F_p, byte has_condition, byte condition, byte flag);
static void jp_hl(cpu *cpu_p);
static void jr(cpu *cpu_p, byte *F_p, byte has_condition, byte condition, byte flag);
static void push_word_to_stack(memory_map *memory_p, cpu_register *SP_p, word address);
static void call(cpu *cpu_p, byte* F_p, byte has_condition, byte condition, byte flag);
static word address;
static byte *register_p;
static byte data;


cpu *initialize_cpu(memory_map *memory_p){
    
    cpu *cpu_p = calloc(sizeof(cpu), 1);
    cpu_p->memory_p = memory_p;
    return cpu_p;
}

int execute_next_opcode(cpu *cpu_p){

    int cycles = 0;
    byte opcode = read_memory(cpu_p->memory_p, cpu_p->PC);
    cpu_p->PC++;
    cycles = execute_opcode(cpu_p, opcode);
    return cycles;

}

int execute_opcode(cpu *cpu_p, byte opcode){
    
    switch(opcode){
        
        // LD r, n when n == immediate 8 bit
        case 0x3E: load_immediate_8_bit(cpu_p, &cpu_p->AF.hi); return 8;
        case 0x06: load_immediate_8_bit(cpu_p, &cpu_p->BC.hi); return 8;
        case 0x0E: load_immediate_8_bit(cpu_p, &cpu_p->BC.lo); return 8;
        case 0x16: load_immediate_8_bit(cpu_p, &cpu_p->DE.hi); return 8;
        case 0x1E: load_immediate_8_bit(cpu_p, &cpu_p->DE.lo); return 8;
        case 0x26: load_immediate_8_bit(cpu_p, &cpu_p->HL.hi); return 8;
        case 0x2E: load_immediate_8_bit(cpu_p, &cpu_p->HL.lo); return 8;

        // LD r1, r2 when r1 == A
        case 0x7F: load_8_bit(&cpu_p->AF.hi, cpu_p->AF.hi); return 4;
        case 0x78: load_8_bit(&cpu_p->AF.hi, cpu_p->BC.hi); return 4;
        case 0x79: load_8_bit(&cpu_p->AF.hi, cpu_p->BC.lo); return 4;
        case 0x7A: load_8_bit(&cpu_p->AF.hi, cpu_p->DE.hi); return 4;
        case 0x7B: load_8_bit(&cpu_p->AF.hi, cpu_p->DE.lo); return 4;
        case 0x7C: load_8_bit(&cpu_p->AF.hi, cpu_p->HL.hi); return 4;
        case 0x7D: load_8_bit(&cpu_p->AF.hi, cpu_p->HL.lo); return 4;
        // LD A, n  when n == {(BC), (DE), (nn), #} 
        case 0x0A: load_8_bit(&cpu_p->AF.hi, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->BC))); return 8; 
        case 0x1A: load_8_bit(&cpu_p->AF.hi, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->DE))); return 8;
        case 0xFA: load_8_bit(&cpu_p->AF.hi, read_memory(cpu_p->memory_p, get_immediate_16_bit(cpu_p))); return 16;
        
        // LD r1, r2 when r1 == B
        case 0x47: load_8_bit(&cpu_p->BC.hi, cpu_p->AF.hi); return 4;
        case 0x40: load_8_bit(&cpu_p->BC.hi, cpu_p->BC.hi); return 4;
        case 0x41: load_8_bit(&cpu_p->BC.hi, cpu_p->BC.lo); return 4;
        case 0x42: load_8_bit(&cpu_p->BC.hi, cpu_p->DE.hi); return 4;
        case 0x43: load_8_bit(&cpu_p->BC.hi, cpu_p->DE.lo); return 4;
        case 0x44: load_8_bit(&cpu_p->BC.hi, cpu_p->HL.hi); return 4;
        case 0x45: load_8_bit(&cpu_p->BC.hi, cpu_p->HL.lo); return 4;
        
        // LD r1, r2 when r1 == C
        case 0x4F: load_8_bit(&cpu_p->BC.lo, cpu_p->AF.hi); return 4;
        case 0x48: load_8_bit(&cpu_p->BC.lo, cpu_p->BC.hi); return 4;
        case 0x49: load_8_bit(&cpu_p->BC.lo, cpu_p->BC.lo); return 4;
        case 0x4A: load_8_bit(&cpu_p->BC.lo, cpu_p->DE.hi); return 4;
        case 0x4B: load_8_bit(&cpu_p->BC.lo, cpu_p->DE.lo); return 4;
        case 0x4C: load_8_bit(&cpu_p->BC.lo, cpu_p->HL.hi); return 4;
        case 0x4D: load_8_bit(&cpu_p->BC.lo, cpu_p->HL.lo); return 4;
        
        // LD r1, r2 when r1 == D
        case 0x57: load_8_bit(&cpu_p->DE.hi, cpu_p->AF.hi); return 4;
        case 0x50: load_8_bit(&cpu_p->DE.hi, cpu_p->BC.hi); return 4;
        case 0x51: load_8_bit(&cpu_p->DE.hi, cpu_p->BC.lo); return 4;
        case 0x52: load_8_bit(&cpu_p->DE.hi, cpu_p->DE.hi); return 4;
        case 0x53: load_8_bit(&cpu_p->DE.hi, cpu_p->DE.lo); return 4;
        case 0x54: load_8_bit(&cpu_p->DE.hi, cpu_p->HL.hi); return 4;
        case 0x55: load_8_bit(&cpu_p->DE.hi, cpu_p->HL.lo); return 4;
        
        // LD r1, r2 when r1 == E
        case 0x5F: load_8_bit(&cpu_p->DE.lo, cpu_p->AF.hi); return 4;
        case 0x58: load_8_bit(&cpu_p->DE.lo, cpu_p->BC.hi); return 4;
        case 0x59: load_8_bit(&cpu_p->DE.lo, cpu_p->BC.lo); return 4;
        case 0x5A: load_8_bit(&cpu_p->DE.lo, cpu_p->DE.hi); return 4;
        case 0x5B: load_8_bit(&cpu_p->DE.lo, cpu_p->DE.lo); return 4;
        case 0x5C: load_8_bit(&cpu_p->DE.lo, cpu_p->HL.hi); return 4;
        case 0x5D: load_8_bit(&cpu_p->DE.lo, cpu_p->HL.lo); return 4;
        
        // LD r1, r2 when r1 == H
        case 0x67: load_8_bit(&cpu_p->HL.hi, cpu_p->AF.hi); return 4;
        case 0x60: load_8_bit(&cpu_p->HL.hi, cpu_p->BC.hi); return 4;
        case 0x61: load_8_bit(&cpu_p->HL.hi, cpu_p->BC.lo); return 4;
        case 0x62: load_8_bit(&cpu_p->HL.hi, cpu_p->DE.hi); return 4;
        case 0x63: load_8_bit(&cpu_p->HL.hi, cpu_p->DE.lo); return 4;
        case 0x64: load_8_bit(&cpu_p->HL.hi, cpu_p->HL.hi); return 4;
        case 0x65: load_8_bit(&cpu_p->HL.hi, cpu_p->HL.lo); return 4;
       
        // LD r1, r2 when r1 == L
        case 0x6F: load_8_bit(&cpu_p->HL.lo, cpu_p->AF.hi); return 4;
        case 0x68: load_8_bit(&cpu_p->HL.lo, cpu_p->BC.hi); return 4;
        case 0x69: load_8_bit(&cpu_p->HL.lo, cpu_p->BC.lo); return 4;
        case 0x6A: load_8_bit(&cpu_p->HL.lo, cpu_p->DE.hi); return 4;
        case 0x6B: load_8_bit(&cpu_p->HL.lo, cpu_p->DE.lo); return 4;
        case 0x6C: load_8_bit(&cpu_p->HL.lo, cpu_p->HL.hi); return 4;
        case 0x6D: load_8_bit(&cpu_p->HL.lo, cpu_p->HL.lo); return 4;
        
        // LD r1, r2 when r2 == (HL)
        case 0x7E: load_8_bit(&cpu_p->AF.hi, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8;
        case 0x46: load_8_bit(&cpu_p->BC.hi, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8;
        case 0x4E: load_8_bit(&cpu_p->BC.lo, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8;
        case 0x56: load_8_bit(&cpu_p->DE.hi, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8;
        case 0x5E: load_8_bit(&cpu_p->DE.lo, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8;
        case 0x66: load_8_bit(&cpu_p->HL.hi, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8;
        case 0x6E: load_8_bit(&cpu_p->HL.lo, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8; 
        
        // LD r1, r2 when r1 == (HL) 
        case 0x77: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), cpu_p->AF.hi); return 8;
        case 0x70: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), cpu_p->BC.hi); return 8;
        case 0x71: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), cpu_p->BC.lo); return 8;
        case 0x72: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), cpu_p->DE.hi); return 8;
        case 0x73: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), cpu_p->DE.lo); return 8;
        case 0x74: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), cpu_p->HL.hi); return 8;
        case 0x75: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), cpu_p->HL.lo); return 8;
        case 0x36: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), get_immediate_8_bit(cpu_p)); return 12;
          
        // LD n, A when n == {(BC), (DE), (HL), (nn)}
        case 0x02: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->BC), cpu_p->AF.hi); return 8;
        case 0x12: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->DE), cpu_p->AF.hi); return 8;
        case 0xEA: write_memory(cpu_p->memory_p, get_immediate_16_bit(cpu_p), cpu_p->AF.hi); return 16;
        
        // LD A, (C)
        case 0xF2: load_8_bit(&cpu_p->AF.hi, read_memory(cpu_p->memory_p, 0xFF00 + cpu_p->BC.lo)); return 8;
        // LD (C), A
        case 0xE2: write_memory(cpu_p->memory_p, 0xFF00 + cpu_p->BC.lo, cpu_p->AF.hi); return 8;
        
        // LD A, (HLD)
        case 0x3A: load_8_bit(&cpu_p->AF.hi, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); decrement(&cpu_p->HL); return 8;
        // LD (HLD), A
        case 0x32: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), cpu_p->AF.hi); decrement(&cpu_p->HL); return 8; 
        
        // LD A, (HLI)
        case 0x2A: load_8_bit(&cpu_p->AF.hi, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); increment(&cpu_p->HL); return 8;
        // LD, (HLI), A
        case 0x22: write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), cpu_p->AF.hi); increment(&cpu_p->HL); return 8;

        // LDH (n), A
        case 0xE0: write_memory(cpu_p->memory_p, 0xFF00 + get_immediate_8_bit(cpu_p), cpu_p->AF.hi); return 12;
        // LDH A, (n)
        case 0xF0: load_8_bit(&cpu_p->AF.hi, read_memory(cpu_p->memory_p, 0xFF00 + get_immediate_8_bit(cpu_p))); return 12;

        // LD n,nn when nn == immediate 16 bit
        case 0x01: load_immediate_16_bit(cpu_p, &cpu_p->BC); return 12;
        case 0x11: load_immediate_16_bit(cpu_p, &cpu_p->DE); return 12;
        case 0x21: load_immediate_16_bit(cpu_p, &cpu_p->HL); return 12;
        case 0x31: load_immediate_16_bit(cpu_p, &cpu_p->SP); return 12;

        // LD SP, HL
        case 0xF9: set_registers_word(&cpu_p->SP, get_registers_word(&cpu_p->HL)); return 8;
        case 0xF8: load_hl(cpu_p, &cpu_p->AF, &cpu_p->HL); return 12;
        // LD (nn), SP
        case 0x08: 
            address = get_immediate_16_bit(cpu_p);
            write_memory(cpu_p->memory_p, address, cpu_p->SP.lo);
            address++;
            write_memory(cpu_p->memory_p, address, cpu_p->SP.hi);
            return 20; 

        // 8-BIT ALU

         // ADC A,n 
        case 0x87: add_8_bit(&cpu_p->AF, cpu_p->AF.lo); return 4;
        case 0x80: add_8_bit(&cpu_p->AF, cpu_p->BC.hi); return 4;
        case 0x81: add_8_bit(&cpu_p->AF, cpu_p->BC.lo); return 4;
        case 0x82: add_8_bit(&cpu_p->AF, cpu_p->DE.hi); return 4;
        case 0x83: add_8_bit(&cpu_p->AF, cpu_p->DE.lo); return 4;
        case 0x84: add_8_bit(&cpu_p->AF, cpu_p->HL.hi); return 4;
        case 0x85: add_8_bit(&cpu_p->AF, cpu_p->HL.lo); return 4;
        case 0x86: add_8_bit(&cpu_p->AF, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->AF))); return 8;
        case 0xC6: add_8_bit(&cpu_p->AF, get_immediate_8_bit(cpu_p)); return 8;

        // ADC A, n
        case 0x8F: add_carry_8_bit(&cpu_p->AF, cpu_p->AF.hi); return 4;
        case 0x88: add_carry_8_bit(&cpu_p->AF, cpu_p->BC.hi); return 4;
        case 0x89: add_carry_8_bit(&cpu_p->AF, cpu_p->BC.lo); return 4;
        case 0x8A: add_carry_8_bit(&cpu_p->AF, cpu_p->DE.hi); return 4;
        case 0x8B: add_carry_8_bit(&cpu_p->AF, cpu_p->DE.lo); return 4;
        case 0x8C: add_carry_8_bit(&cpu_p->AF, cpu_p->HL.hi); return 4;
        case 0x8D: add_carry_8_bit(&cpu_p->AF, cpu_p->HL.lo); return 4;
        case 0x8E: add_carry_8_bit(&cpu_p->AF, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->AF))); return 8;
        case 0xCE: add_carry_8_bit(&cpu_p->AF, get_immediate_8_bit(cpu_p)); return 8;
        
        // SUB n
        case 0x97: sub_8_bit(&cpu_p->AF, cpu_p->AF.hi); return 4;
        case 0x90: sub_8_bit(&cpu_p->AF, cpu_p->BC.hi); return 4;
        case 0x91: sub_8_bit(&cpu_p->AF, cpu_p->BC.lo); return 4;
        case 0x92: sub_8_bit(&cpu_p->AF, cpu_p->DE.hi); return 4;
        case 0x93: sub_8_bit(&cpu_p->AF, cpu_p->DE.lo); return 4;
        case 0x94: sub_8_bit(&cpu_p->AF, cpu_p->HL.hi); return 4;
        case 0x95: sub_8_bit(&cpu_p->AF, cpu_p->HL.lo); return 4;
        case 0x96: sub_8_bit(&cpu_p->AF, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8;
        case 0xD6: sub_8_bit(&cpu_p->AF, get_immediate_8_bit(cpu_p)); return 8;
       
        // SBC A, n 
        case 0x9F: sub_carry_8_bit(&cpu_p->AF, cpu_p->AF.hi); return 4;
        case 0x98: sub_carry_8_bit(&cpu_p->AF, cpu_p->BC.hi); return 4;
        case 0x99: sub_carry_8_bit(&cpu_p->AF, cpu_p->BC.lo); return 4;
        case 0x9A: sub_carry_8_bit(&cpu_p->AF, cpu_p->DE.hi); return 4;
        case 0x9B: sub_carry_8_bit(&cpu_p->AF, cpu_p->DE.lo); return 4;
        case 0x9C: sub_carry_8_bit(&cpu_p->AF, cpu_p->HL.hi); return 4;
        case 0x9D: sub_carry_8_bit(&cpu_p->AF, cpu_p->HL.lo); return 4;
        case 0x9E: sub_carry_8_bit(&cpu_p->AF, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL)));
        // need to very 0xDE is SBC, not in the gameboy manual
        case 0xDE: sub_8_bit(&cpu_p->AF, get_immediate_8_bit(cpu_p)); return 8;

        // AND n 
        case 0xA7: and_8_bit(&cpu_p->AF, cpu_p->AF.hi); return 4;
        case 0xA0: and_8_bit(&cpu_p->AF, cpu_p->BC.hi); return 4;
        case 0xA1: and_8_bit(&cpu_p->AF, cpu_p->BC.lo); return 4;
        case 0xA2: and_8_bit(&cpu_p->AF, cpu_p->DE.hi); return 4;
        case 0xA3: and_8_bit(&cpu_p->AF, cpu_p->DE.lo); return 4;
        case 0xA4: and_8_bit(&cpu_p->AF, cpu_p->HL.hi); return 4;
        case 0xA5: and_8_bit(&cpu_p->AF, cpu_p->HL.lo); return 4;
        case 0xA6: and_8_bit(&cpu_p->AF, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8; 
        case 0xE6: and_8_bit(&cpu_p->AF, get_immediate_8_bit(cpu_p)); return 8;

        // OR n
        case 0xB7: or_8_bit(&cpu_p->AF, cpu_p->AF.hi); return 4;
        case 0xB0: or_8_bit(&cpu_p->AF, cpu_p->BC.hi); return 4;
        case 0xB1: or_8_bit(&cpu_p->AF, cpu_p->BC.lo); return 4;
        case 0xB2: or_8_bit(&cpu_p->AF, cpu_p->DE.hi); return 4;
        case 0xB3: or_8_bit(&cpu_p->AF, cpu_p->DE.lo); return 4;
        case 0xB4: or_8_bit(&cpu_p->AF, cpu_p->HL.hi); return 4;
        case 0xB5: or_8_bit(&cpu_p->AF, cpu_p->HL.lo); return 4;
        case 0xB6: or_8_bit(&cpu_p->AF, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8; 
        case 0xF6: or_8_bit(&cpu_p->AF, get_immediate_8_bit(cpu_p)); return 8;

        // XOR n
        case 0xAF: xor_8_bit(&cpu_p->AF, cpu_p->AF.hi); return 4;
        case 0xA8: xor_8_bit(&cpu_p->AF, cpu_p->BC.hi); return 4;
        case 0xA9: xor_8_bit(&cpu_p->AF, cpu_p->BC.lo); return 4;
        case 0xAA: xor_8_bit(&cpu_p->AF, cpu_p->DE.hi); return 4;
        case 0xAB: xor_8_bit(&cpu_p->AF, cpu_p->DE.lo); return 4;
        case 0xAC: xor_8_bit(&cpu_p->AF, cpu_p->HL.hi); return 4;
        case 0xAD: xor_8_bit(&cpu_p->AF, cpu_p->HL.lo); return 4;
        case 0xAE: xor_8_bit(&cpu_p->AF, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8; 
        case 0xEE: xor_8_bit(&cpu_p->AF, get_immediate_8_bit(cpu_p)); return 8;

        // CP n
        case 0xBF: cp_8_bit(&cpu_p->AF, cpu_p->AF.hi); return 4;
        case 0xB8: cp_8_bit(&cpu_p->AF, cpu_p->BC.hi); return 4;
        case 0xB9: cp_8_bit(&cpu_p->AF, cpu_p->BC.lo); return 4;
        case 0xBA: cp_8_bit(&cpu_p->AF, cpu_p->DE.hi); return 4;
        case 0xBB: cp_8_bit(&cpu_p->AF, cpu_p->DE.lo); return 4;
        case 0xBC: cp_8_bit(&cpu_p->AF, cpu_p->HL.hi); return 4;
        case 0xBD: cp_8_bit(&cpu_p->AF, cpu_p->HL.lo); return 4;
        case 0xBE: cp_8_bit(&cpu_p->AF, read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL))); return 8; 
        case 0xFE: cp_8_bit(&cpu_p->AF, get_immediate_8_bit(cpu_p)); return 8;
        
        // INC n
        case 0x3C: inc_8_bit(&cpu_p->AF.hi, &cpu_p->AF); return 4;
        case 0x04: inc_8_bit(&cpu_p->BC.hi, &cpu_p->AF); return 4;
        case 0x0C: inc_8_bit(&cpu_p->BC.lo, &cpu_p->AF); return 4;
        case 0x14: inc_8_bit(&cpu_p->DE.hi, &cpu_p->AF); return 4;
        case 0x1C: inc_8_bit(&cpu_p->DE.lo, &cpu_p->AF); return 4;
        case 0x24: inc_8_bit(&cpu_p->HL.hi, &cpu_p->AF); return 4;
        case 0x2C: inc_8_bit(&cpu_p->AF.lo, &cpu_p->AF); return 4;
        case 0x34: inc_memory_8_bit(cpu_p, &cpu_p->AF); return 12;

        // DEC n
        case 0x3D: dec_8_bit(&cpu_p->AF.hi, &cpu_p->AF); return 4;
        case 0x05: dec_8_bit(&cpu_p->BC.hi, &cpu_p->AF); return 4;
        case 0x0D: dec_8_bit(&cpu_p->BC.lo, &cpu_p->AF); return 4;
        case 0x15: dec_8_bit(&cpu_p->DE.hi, &cpu_p->AF); return 4;
        case 0x1D: dec_8_bit(&cpu_p->DE.lo, &cpu_p->AF); return 4;
        case 0x25: dec_8_bit(&cpu_p->HL.hi, &cpu_p->AF); return 4;
        case 0x2D: dec_8_bit(&cpu_p->AF.lo, &cpu_p->AF); return 4;
        case 0x35: dec_memory_8_bit(cpu_p, &cpu_p->AF); return 12;

        // 16-bit ALU

        // ADD HL, n
        case 0x09: add_16_bit_hl(&cpu_p->HL, &cpu_p->AF, get_registers_word(&cpu_p->BC)); return 8;
        case 0x19: add_16_bit_hl(&cpu_p->HL, &cpu_p->AF, get_registers_word(&cpu_p->DE)); return 8;
        case 0x29: add_16_bit_hl(&cpu_p->HL, &cpu_p->AF, get_registers_word(&cpu_p->HL)); return 8;
        case 0x39: add_16_bit_hl(&cpu_p->HL, &cpu_p->AF, get_registers_word(&cpu_p->SP)); return 8;

        // ADD SP, n
        case 0xE8: add_16_bit_sp(cpu_p, &cpu_p->SP, &cpu_p->AF); return 16;

        // INC, nn
        case 0x03: inc_16_bit(&cpu_p->BC); return 8;
        case 0x13: inc_16_bit(&cpu_p->DE); return 8;
        case 0x23: inc_16_bit(&cpu_p->HL); return 8;
        case 0x33: inc_16_bit(&cpu_p->SP); return 8;

        // DEC, nn
        case 0x0B: dec_16_bit(&cpu_p->BC); return 8;
        case 0x1B: dec_16_bit(&cpu_p->DE); return 8;
        case 0x2B: dec_16_bit(&cpu_p->HL); return 8;
        case 0x3B: dec_16_bit(&cpu_p->SP); return 8;

        // CB
        case 0xCB: return execute_extended_opcode(cpu_p);

        // Miscellaneous
        
        // DAA
        case 0x27: daa(&cpu_p->AF); return 4; 

        // CPL
        case 0x2F: cpl(&cpu_p->AF); return 4;

        // CCF 
        case 0x3F: ccf(&cpu_p->AF.lo); return 4;

        // SCF
        case 0x37: scf(&cpu_p->AF.lo); return 4;

        // NOP
        case 0x00: nop(); return 4;

        // HALT
        case 0x76: halt(cpu_p); return 4;

        // STOP 10 00
        case 0x10: stop(cpu_p); return 4;

        // DI
        case 0xF3: di(cpu_p); return 4;

        // EI
        case 0xFB: ei(cpu_p); return 4;

        // left rotation
        case 0x07: rlc(&cpu_p->AF.hi, &cpu_p->AF); return 4;
        case 0x17: rl(&cpu_p->AF.hi, &cpu_p->AF.lo); return 4;

        // right rotation
        case 0x0F: rrc(&cpu_p->AF.hi, &cpu_p->AF); return 4;
        case 0x1F: rr(&cpu_p->AF.hi, &cpu_p->AF.lo); return 4;

        // JUMPs

        // JP nn
        case 0xC3: jp(cpu_p, &cpu_p->AF.lo, FALSE, FALSE, 0); return 12;
        // JP cc, nn
        case 0xC2: jp(cpu_p, &cpu_p->AF.lo, TRUE, FALSE, ZERO_FLAG); return 12;
        case 0xCA: jp(cpu_p, &cpu_p->AF.lo, TRUE, TRUE, ZERO_FLAG); return 12; 
        case 0xD2: jp(cpu_p, &cpu_p->AF.lo, TRUE, FALSE, CARRY_FLAG); return 12;
        case 0xDA: jp(cpu_p, &cpu_p->AF.lo, TRUE, TRUE, CARRY_FLAG); return 12;

        // JP (HL)
        case 0xE9: jp_hl(cpu_p); return 4;
        
        // JR n
        case 0x18: jr(cpu_p, &cpu_p->AF.lo, FALSE, FALSE, 0); return 8;
        case 0x20: jr(cpu_p, &cpu_p->AF.lo, TRUE, FALSE, ZERO_FLAG); return 8;
        case 0x28: jr(cpu_p, &cpu_p->AF.lo, TRUE, TRUE, ZERO_FLAG); return 8;
        case 0x30: jr(cpu_p, &cpu_p->AF.lo, TRUE, FALSE, CARRY_FLAG); return 8;
        case 0x38: jr(cpu_p, &cpu_p->AF.lo, TRUE, TRUE, CARRY_FLAG); return 8;
        
        // CALL
        case 0xCD: call(cpu_p, &cpu_p->AF.lo, FALSE, FALSE, 0); return 12;
        case 0xC4: call(cpu_p, &cpu_p->AF.lo, TRUE, FALSE, ZERO_FLAG); return 12;
        case 0xCC: call(cpu_p, &cpu_p->AF.lo, TRUE, TRUE, ZERO_FLAG); return 12; 
        case 0xD4: call(cpu_p, &cpu_p->AF.lo, TRUE, FALSE, CARRY_FLAG); return 12;
        case 0xDC: call(cpu_p, &cpu_p->AF.lo, TRUE, TRUE, CARRY_FLAG); return 12;
        
    }

    return 0;
}

static int execute_extended_opcode(cpu *cpu_p){
    
    byte extended_opcode = get_immediate_8_bit(cpu_p);
    
    switch(extended_opcode){

        // SWAP n
        case 0x37: swap_nibble(&cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x30: swap_nibble(&cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x31: swap_nibble(&cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x32: swap_nibble(&cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x33: swap_nibble(&cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x34: swap_nibble(&cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x35: swap_nibble(&cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0X36: swap_nibble_memory(cpu_p->memory_p, &cpu_p->AF.lo, get_registers_word(&cpu_p->HL)); return 16;

        // RLC n
        case 0x07: rlc(&cpu_p->AF.hi, &cpu_p->AF); return 8;
        case 0x00: rlc(&cpu_p->BC.hi, &cpu_p->AF); return 8;
        case 0x01: rlc(&cpu_p->BC.lo, &cpu_p->AF); return 8;
        case 0x02: rlc(&cpu_p->DE.hi, &cpu_p->AF); return 8;
        case 0x03: rlc(&cpu_p->DE.lo, &cpu_p->AF); return 8;
        case 0x04: rlc(&cpu_p->HL.hi, &cpu_p->AF); return 8;
        case 0x05: rlc(&cpu_p->HL.lo, &cpu_p->AF); return 8;
        case 0x06: rlc_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF); return 16;

        // RL n
        case 0x17: rl(&cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x10: rl(&cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x11: rl(&cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x12: rl(&cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x13: rl(&cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x14: rl(&cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x15: rl(&cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x16: rl_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;

        // RRC n
        case 0x0F: rrc(&cpu_p->AF.hi, &cpu_p->AF); return 8;
        case 0x08: rrc(&cpu_p->BC.hi, &cpu_p->AF); return 8;
        case 0x09: rrc(&cpu_p->BC.lo, &cpu_p->AF); return 8;
        case 0x0A: rrc(&cpu_p->DE.hi, &cpu_p->AF); return 8;
        case 0x0B: rrc(&cpu_p->DE.lo, &cpu_p->AF); return 8;
        case 0x0C: rrc(&cpu_p->HL.hi, &cpu_p->AF); return 8;
        case 0x0D: rrc(&cpu_p->HL.lo, &cpu_p->AF); return 8;
        case 0x0E: rrc_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF); return 16;

        // RR n
        case 0x1F: rr(&cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x18: rr(&cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x19: rr(&cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x1A: rr(&cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x1B: rr(&cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x1C: rr(&cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x1D: rr(&cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x1E: rr_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;

        // SLA n
        case 0x27: sla(&cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x20: sla(&cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x21: sla(&cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x22: sla(&cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x23: sla(&cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x24: sla(&cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x25: sla(&cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x26: sla_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;

        // SRA n
        case 0x2F: sra(&cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x28: sra(&cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x29: sra(&cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x2A: sra(&cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x2B: sra(&cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x2C: sra(&cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x2D: sra(&cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x2E: sra_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;
        
        // SRL n
        case 0x3F: srl(&cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x38: srl(&cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x39: srl(&cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x3A: srl(&cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x3B: srl(&cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x3C: srl(&cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x3D: srl(&cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x3E: srl_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;
        
        // BIT Opcodes

        // BIT b,r
        case 0x40: bit(0, &cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x41: bit(0, &cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x42: bit(0, &cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x43: bit(0, &cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x44: bit(0, &cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x45: bit(0, &cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x46: bit_memory(0, cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;
        case 0x47: bit(0, &cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x48: bit(1, &cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x49: bit(1, &cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x4A: bit(1, &cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x4B: bit(1, &cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x4C: bit(1, &cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x4D: bit(1, &cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x4E: bit_memory(1, cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;
        case 0x4F: bit(1, &cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x50: bit(2, &cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x51: bit(2, &cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x52: bit(2, &cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x53: bit(2, &cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x54: bit(2, &cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x55: bit(2, &cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x56: bit_memory(2, cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;
        case 0x57: bit(2, &cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x58: bit(3, &cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x59: bit(3, &cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x5A: bit(3, &cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x5B: bit(3, &cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x5C: bit(3, &cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x5D: bit(3, &cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x5E: bit_memory(3, cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;
        case 0x5F: bit(3, &cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x60: bit(4, &cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x61: bit(4, &cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x62: bit(4, &cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x63: bit(4, &cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x64: bit(4, &cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x65: bit(4, &cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x66: bit_memory(4, cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;
        case 0x67: bit(4, &cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x68: bit(5, &cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x69: bit(5, &cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x6A: bit(5, &cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x6B: bit(5, &cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x6C: bit(5, &cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x6D: bit(5, &cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x6E: bit_memory(5, cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;
        case 0x6F: bit(5, &cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x70: bit(6, &cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x71: bit(6, &cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x72: bit(6, &cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x73: bit(6, &cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x74: bit(6, &cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x75: bit(6, &cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x76: bit_memory(6, cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;
        case 0x77: bit(6, &cpu_p->AF.hi, &cpu_p->AF.lo); return 8;
        case 0x78: bit(7, &cpu_p->BC.hi, &cpu_p->AF.lo); return 8;
        case 0x79: bit(7, &cpu_p->BC.lo, &cpu_p->AF.lo); return 8;
        case 0x7A: bit(7, &cpu_p->DE.hi, &cpu_p->AF.lo); return 8;
        case 0x7B: bit(7, &cpu_p->DE.lo, &cpu_p->AF.lo); return 8;
        case 0x7C: bit(7, &cpu_p->HL.hi, &cpu_p->AF.lo); return 8;
        case 0x7D: bit(7, &cpu_p->HL.lo, &cpu_p->AF.lo); return 8;
        case 0x7E: bit_memory(7, cpu_p->memory_p, get_registers_word(&cpu_p->HL), &cpu_p->AF.lo); return 16;
        case 0x7F: bit(7, &cpu_p->AF.hi, &cpu_p->AF.lo); return 8;

        // RST b, n
        case 0x80: res(0, &cpu_p->BC.hi); return 8;
        case 0x81: res(0, &cpu_p->BC.lo); return 8;
        case 0x82: res(0, &cpu_p->DE.hi); return 8;
        case 0x83: res(0, &cpu_p->DE.lo); return 8;
        case 0x84: res(0, &cpu_p->HL.hi); return 8;
        case 0x85: res(0, &cpu_p->HL.lo); return 8;
        case 0x86: res_memory(0, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0x87: res(0, &cpu_p->AF.hi); return 8;
        case 0x88: res(1, &cpu_p->BC.hi); return 8;
        case 0x89: res(1, &cpu_p->BC.lo); return 8;
        case 0x8A: res(1, &cpu_p->DE.hi); return 8;
        case 0x8B: res(1, &cpu_p->DE.lo); return 8;
        case 0x8C: res(1, &cpu_p->HL.hi); return 8;
        case 0x8D: res(1, &cpu_p->HL.lo); return 8;
        case 0x8E: res_memory(1, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0x8F: res(1, &cpu_p->AF.hi); return 8;
        case 0x90: res(2, &cpu_p->BC.hi); return 8;
        case 0x91: res(2, &cpu_p->BC.lo); return 8;
        case 0x92: res(2, &cpu_p->DE.hi); return 8;
        case 0x93: res(2, &cpu_p->DE.lo); return 8;
        case 0x94: res(2, &cpu_p->HL.hi); return 8;
        case 0x95: res(2, &cpu_p->HL.lo); return 8;
        case 0x96: res_memory(2, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0x97: res(2, &cpu_p->AF.hi); return 8;
        case 0x98: res(3, &cpu_p->BC.hi); return 8;
        case 0x99: res(3, &cpu_p->BC.lo); return 8;
        case 0x9A: res(3, &cpu_p->DE.hi); return 8;
        case 0x9B: res(3, &cpu_p->DE.lo); return 8;
        case 0x9C: res(3, &cpu_p->HL.hi); return 8;
        case 0x9D: res(3, &cpu_p->HL.lo); return 8;
        case 0x9E: res_memory(3, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0x9F: res(3, &cpu_p->AF.hi); return 8;
        case 0xA0: res(4, &cpu_p->BC.hi); return 8;
        case 0xA1: res(4, &cpu_p->BC.lo); return 8;
        case 0xA2: res(4, &cpu_p->DE.hi); return 8;
        case 0xA3: res(4, &cpu_p->DE.lo); return 8;
        case 0xA4: res(4, &cpu_p->HL.hi); return 8;
        case 0xA5: res(4, &cpu_p->HL.lo); return 8;
        case 0xA6: res_memory(4, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xA7: res(4, &cpu_p->AF.hi); return 8;
        case 0xA8: res(5, &cpu_p->BC.hi); return 8;
        case 0xA9: res(5, &cpu_p->BC.lo); return 8;
        case 0xAA: res(5, &cpu_p->DE.hi); return 8;
        case 0xAB: res(5, &cpu_p->DE.lo); return 8;
        case 0xAC: res(5, &cpu_p->HL.hi); return 8;
        case 0xAD: res(5, &cpu_p->HL.lo); return 8;
        case 0xAE: res_memory(5, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xAF: res(5, &cpu_p->AF.hi); return 8;
        case 0xB0: res(6, &cpu_p->BC.hi); return 8;
        case 0xB1: res(6, &cpu_p->BC.lo); return 8;
        case 0xB2: res(6, &cpu_p->DE.hi); return 8;
        case 0xB3: res(6, &cpu_p->DE.lo); return 8;
        case 0xB4: res(6, &cpu_p->HL.hi); return 8;
        case 0xB5: res(6, &cpu_p->HL.lo); return 8;
        case 0xB6: res_memory(6, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xB7: res(6, &cpu_p->AF.hi); return 8;
        case 0xB8: res(7, &cpu_p->BC.hi); return 8;
        case 0xB9: res(7, &cpu_p->BC.lo); return 8;
        case 0xBA: res(7, &cpu_p->DE.hi); return 8;
        case 0xBB: res(7, &cpu_p->DE.lo); return 8;
        case 0xBC: res(7, &cpu_p->HL.hi); return 8;
        case 0xBD: res(7, &cpu_p->HL.lo); return 8;
        case 0xBE: res_memory(7, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xBF: res(7, &cpu_p->AF.hi); return 8;

        //SET b, n
        case 0xC0: set(0, &cpu_p->BC.hi); return 8;
        case 0xC1: set(0, &cpu_p->BC.lo); return 8;
        case 0xC2: set(0, &cpu_p->DE.hi); return 8;
        case 0xC3: set(0, &cpu_p->DE.lo); return 8;
        case 0xC4: set(0, &cpu_p->HL.hi); return 8;
        case 0xC5: set(0, &cpu_p->HL.lo); return 8;
        case 0xC6: set_memory(0, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xC7: set(0, &cpu_p->AF.hi); return 8;
        case 0xC8: set(1, &cpu_p->BC.hi); return 8;
        case 0xC9: set(1, &cpu_p->BC.lo); return 8;
        case 0xCA: set(1, &cpu_p->DE.hi); return 8;
        case 0xCB: set(1, &cpu_p->DE.lo); return 8;
        case 0xCC: set(1, &cpu_p->HL.hi); return 8;
        case 0xCD: set(1, &cpu_p->HL.lo); return 8;
        case 0xCE: set_memory(1, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xCF: set(1, &cpu_p->AF.hi); return 8;
        case 0xD0: set(2, &cpu_p->BC.hi); return 8;
        case 0xD1: set(2, &cpu_p->BC.lo); return 8;
        case 0xD2: set(2, &cpu_p->DE.hi); return 8;
        case 0xD3: set(2, &cpu_p->DE.lo); return 8;
        case 0xD4: set(2, &cpu_p->HL.hi); return 8;
        case 0xD5: set(2, &cpu_p->HL.lo); return 8;
        case 0xD6: set_memory(2, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xD7: set(2, &cpu_p->AF.hi); return 8;
        case 0xD8: set(3, &cpu_p->BC.hi); return 8;
        case 0xD9: set(3, &cpu_p->BC.lo); return 8;
        case 0xDA: set(3, &cpu_p->DE.hi); return 8;
        case 0xDB: set(3, &cpu_p->DE.lo); return 8;
        case 0xDC: set(3, &cpu_p->HL.hi); return 8;
        case 0xDD: set(3, &cpu_p->HL.lo); return 8;
        case 0xDE: set_memory(3, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xDF: set(3, &cpu_p->AF.hi); return 8;
        case 0xE0: set(4, &cpu_p->BC.hi); return 8;
        case 0xE1: set(4, &cpu_p->BC.lo); return 8;
        case 0xE2: set(4, &cpu_p->DE.hi); return 8;
        case 0xE3: set(4, &cpu_p->DE.lo); return 8;
        case 0xE4: set(4, &cpu_p->HL.hi); return 8;
        case 0xE5: set(4, &cpu_p->HL.lo); return 8;
        case 0xE6: set_memory(4, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xE7: set(4, &cpu_p->AF.hi); return 8;
        case 0xE8: set(5, &cpu_p->BC.hi); return 8;
        case 0xE9: set(5, &cpu_p->BC.lo); return 8;
        case 0xEA: set(5, &cpu_p->DE.hi); return 8;
        case 0xEB: set(5, &cpu_p->DE.lo); return 8;
        case 0xEC: set(5, &cpu_p->HL.hi); return 8;
        case 0xED: set(5, &cpu_p->HL.lo); return 8;
        case 0xEE: set_memory(5, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xEF: set(5, &cpu_p->AF.hi); return 8;
        case 0xF0: set(6, &cpu_p->BC.hi); return 8;
        case 0xF1: set(6, &cpu_p->BC.lo); return 8;
        case 0xF2: set(6, &cpu_p->DE.hi); return 8;
        case 0xF3: set(6, &cpu_p->DE.lo); return 8;
        case 0xF4: set(6, &cpu_p->HL.hi); return 8;
        case 0xF5: set(6, &cpu_p->HL.lo); return 8;
        case 0xF6: set_memory(6, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xF7: set(6, &cpu_p->AF.hi); return 8;
        case 0xF8: set(7, &cpu_p->BC.hi); return 8;
        case 0xF9: set(7, &cpu_p->BC.lo); return 8;
        case 0xFA: set(7, &cpu_p->DE.hi); return 8;
        case 0xFB: set(7, &cpu_p->DE.lo); return 8;
        case 0xFC: set(7, &cpu_p->HL.hi); return 8;
        case 0xFD: set(7, &cpu_p->HL.lo); return 8;
        case 0xFE: set_memory(6, cpu_p->memory_p, get_registers_word(&cpu_p->HL)); return 16;
        case 0xFF: set(7, &cpu_p->AF.hi); return 8;
    }
    return 0;
}

static void load_immediate_8_bit(cpu *cpu_p, byte *register_p){
    byte n = get_immediate_8_bit(cpu_p);
    *register_p = n;
}

static byte get_immediate_8_bit(cpu *cpu_p){
    byte data = read_memory(cpu_p->memory_p, cpu_p->PC);
    cpu_p->PC++;
    return data;
}

static void load_8_bit(byte *register_p, byte data){
    *register_p = data;
}

static word get_immediate_16_bit(cpu *cpu_p){
    word address = read_memory(cpu_p->memory_p, cpu_p->PC) << 8;
    cpu_p->PC++;
    address |= read_memory(cpu_p->memory_p, cpu_p->PC);
    cpu_p->PC++;
    return address;
}

static void load_immediate_16_bit(cpu *cpu_p, cpu_register *register_p){
    word nn = get_immediate_16_bit(cpu_p);
    set_registers_word(register_p, nn);
}

word get_registers_word(cpu_register *register_p){
    return ((register_p->hi << 8) | (register_p->lo));
}

static void set_registers_word(cpu_register *register_p, word data){
    register_p->hi = (data & 0xFF00) >> 8;
    register_p->lo = (data & 0x00FF);
}

static void load_hl(cpu *cpu_p, cpu_register *AF_p, cpu_register *HL_p){
    
    unsigned int overflow;
    byte n = get_immediate_8_bit(cpu_p);

    AF_p->lo = CLEAR_BIT(AF_p->lo, ZERO_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);

    word value = (get_registers_word(&cpu_p->SP) + n) & 0xFFFF;
    set_registers_word(HL_p, value);

    overflow = get_registers_word(&cpu_p->SP) + n;
    if (overflow > 0xFFFF){
        AF_p->lo = SET_BIT(AF_p->lo, CARRY_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, CARRY_FLAG);
    }

    overflow = (get_registers_word(&cpu_p->SP) & 0xF) + (n & 0xF);
    
    if (overflow > 0xF){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }
} 


static void add_carry_8_bit(cpu_register *AF_p, byte data){
    byte result = AF_p->hi;
    byte carry = 0;
    
    // add carry
    if(TEST_BIT(AF_p->lo, CARRY_FLAG)){
        carry = 1;
    }

    result += (data + carry);


    // FLAG configurations
    AF_p->lo = 0;
    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);

    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    }

    word overflow = AF_p->hi & 0xF;
    overflow += ((data + carry) & 0xF);
    
    if (overflow > 0xF){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

    overflow = AF_p->hi + data + carry;
    if (overflow > 0xFF){
        AF_p->lo = SET_BIT(AF_p->lo, CARRY_FLAG);
    }

    AF_p->hi = result;

}

static void add_8_bit(cpu_register *AF_p, byte data){
    
    byte result = AF_p->hi;

    result += data;

    // FLAG configurations
    AF_p->lo = 0;
    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);

    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    }

    word overflow = AF_p->hi & 0xF;
    overflow += ((data) & 0xF);
    
    if (overflow > 0xF){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

    overflow = AF_p->hi + data;
    if (overflow > 0xFF){
        AF_p->lo = SET_BIT(AF_p->lo, CARRY_FLAG);
    }

    AF_p->hi = result;
}


static void sub_8_bit(cpu_register *AF_p, byte data){
    
    byte result = AF_p->hi;
    result -= data;

    AF_p->lo = 0;

    AF_p->lo = SET_BIT(AF_p->lo, SUBTRACT_FLAG);
    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    }

    if (AF_p->lo < data){
        AF_p->lo = SET_BIT(AF_p->lo, CARRY_FLAG);
    }

    // should this be signed ? 
    word borrow = AF_p->hi & 0xF;
    borrow -= (result & 0xF);

    if (borrow < 0){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

    AF_p->hi = result;
}

static void sub_carry_8_bit(cpu_register *AF_p, byte data){
    
    byte result = AF_p->hi;
    byte carry = 0;
    
    // add carry
    if(TEST_BIT(AF_p->lo, CARRY_FLAG)){
        carry = 1;
    }

    result += carry;
    result -= data;

    AF_p->lo = 0;
    AF_p->lo = SET_BIT(AF_p->lo, SUBTRACT_FLAG);
    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    }

    if ((AF_p->lo + carry) < data){
        AF_p->lo = SET_BIT(AF_p->lo, CARRY_FLAG);
    }

    // should this be signed ? 
    word borrow = AF_p->hi & 0xF;
    borrow += carry;
    borrow -= (result & 0xF);

    if (borrow < 0){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

    AF_p->hi = result;
}

static void and_8_bit(cpu_register *AF_p, byte data){
    
    byte result = AF_p->hi & data;

    AF_p->lo = 0;
    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    }

    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);
    AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, CARRY_FLAG);

    AF_p->hi = result;
}
static void or_8_bit(cpu_register *AF_p, byte data){
    
    byte result = AF_p->hi | data;

    AF_p->lo = 0;
    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    }

    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, CARRY_FLAG);

    AF_p->hi = result;
}

static void xor_8_bit(cpu_register *AF_p, byte data){

    byte result = AF_p->hi ^ data;

    AF_p->lo = 0;
    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    }

    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, CARRY_FLAG);
    
    AF_p->hi = result;
}

static void cp_8_bit(cpu_register *AF_p, byte data){

    AF_p->lo = 0;
    
    if (AF_p->hi == data){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    }

    AF_p->lo = SET_BIT(AF_p->lo, SUBTRACT_FLAG);
    
    signed_word borrow = AF_p->hi & 0xF;
    borrow -= data & 0xF;

    if (borrow < 0){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

    if (AF_p->hi < data){
        AF_p->lo = SET_BIT(AF_p->lo, CARRY_FLAG);
    }
}

static void inc_8_bit(byte *register_p, cpu_register *AF_p){
    
    byte result = *register_p;
    result++;

    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, ZERO_FLAG);
    }
    
    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);
    
    if ((*register_p & 0xF) == 0xF){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

    *register_p = result;
    
}

static void inc_memory_8_bit(cpu *cpu_p, cpu_register *AF_p){

    byte data = read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL));
    byte result = data;
    result++;

    write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), result);

    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, ZERO_FLAG);
    }
    
    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);
    
    if ((data & 0xF) == 0xF){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

}

static void dec_memory_8_bit(cpu *cpu_p, cpu_register *AF_p){

    byte data = read_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL));
    byte result = data;
    result--;

    write_memory(cpu_p->memory_p, get_registers_word(&cpu_p->HL), result);
    
    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, ZERO_FLAG);
    }
    
    AF_p->lo = SET_BIT(AF_p->lo, SUBTRACT_FLAG);
    
    if ((data & 0xF) == 0){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

    *register_p = result;
}

static void dec_8_bit(byte *register_p, cpu_register *AF_p){
    
    byte result = *register_p;
    result--;

    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, ZERO_FLAG);
    }
    
    AF_p->lo = SET_BIT(AF_p->lo, SUBTRACT_FLAG);
    
    if ((*register_p & 0xF) == 0){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

    *register_p = result;
}

static void increment(cpu_register *register_p){
    word value = get_registers_word(register_p);
    value++;
    register_p->hi = (value & 0xFF00) >> 8;
    register_p->lo = (value & 0x00FF);
}

static void decrement(cpu_register *register_p){
    word value = get_registers_word(register_p);
    value--;
    register_p->hi = (value & 0xFF00) >> 8;
    register_p->lo = (value & 0x00FF);
}

static void inc_16_bit(cpu_register *register_p){
    increment(register_p);
}

static void dec_16_bit(cpu_register *register_p){
    decrement(register_p);
}

static void add_16_bit_hl(cpu_register *HL_p, cpu_register *AF_p, word data){
    
    word result = get_registers_word(HL_p) + data;
    
    // flags configuration
    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);
    
    int carry_test = get_registers_word(HL_p) + data;
    if (carry_test > 0xFFFF){
        AF_p->lo = SET_BIT(AF_p->lo, CARRY_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, CARRY_FLAG);
    }
    
    // need to test if carry from bit 11
    carry_test = get_registers_word(HL_p) & 0x0FFF;
    carry_test += data & 0x0FFF;

    if (carry_test > 0x0FFF){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

    set_registers_word(HL_p, result);

}
static void add_16_bit_sp(cpu *cpu_p, cpu_register *SP_p, cpu_register *AF_p){
    
    signed_byte data = get_immediate_8_bit(cpu_p);
    word result = get_registers_word(SP_p) + data;

    // flag configurations
    AF_p->lo = CLEAR_BIT(AF_p->lo, ZERO_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);

    // not sure how H and C are set, need to recheck
    int carry_test = get_registers_word(SP_p) + data;
    if (carry_test > 0xFFFF){
        AF_p->lo = SET_BIT(AF_p->lo, CARRY_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, CARRY_FLAG);
    }

    carry_test = get_registers_word(SP_p) & 0xF;
    carry_test += data & 0xF;
    
    if (carry_test > 0xF){
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }

}

static void swap_nibble(byte *register_p, byte *F_p){

    byte upper_nibble = *register_p & 0xF0;
    byte lower_nibble = *register_p & 0x0F;
    byte swapped_nibble = (lower_nibble << 4) | upper_nibble;
    
    // flag configuration
    *F_p = 0;
    if (swapped_nibble == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }

    *register_p = swapped_nibble;
}
static void swap_nibble_memory(memory_map *memory_p, byte *F_p, word address){

    byte data = read_memory(memory_p, address);
    byte upper_nibble = data & 0xF0;
    byte lower_nibble = data & 0x0F;
    byte swapped_nibble = (lower_nibble << 4) | upper_nibble;

    // flag configuration
    *F_p = 0;
    if (swapped_nibble == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }

    write_memory(memory_p, address, swapped_nibble);
}


// took BCD implementation from external source
// have to verify implementation and test it.
static void daa(cpu_register *AF_p) {

    if (TEST_BIT(AF_p->lo, SUBTRACT_FLAG)){
		if ((AF_p->hi &0x0F ) >0x09 || AF_p->lo &0x20 )
		{
			AF_p->hi -=0x06; //Half borrow: (0-1) = (0xF-0x6) = 9
			if ((AF_p->hi&0xF0)==0xF0) AF_p->lo|=0x10; else AF_p->lo&=~0x10;
		}

		if ((AF_p->hi&0xF0)>0x90 || AF_p->lo&0x10) AF_p->hi-=0x60;
	}
	else
	{
		if ((AF_p->hi&0x0F)>9 || AF_p->lo&0x20)
		{
			AF_p->hi+=0x06; //Half carry: (9+1) = (0xA+0x6) = 10
			if((AF_p->hi&0xF0)==0) AF_p->lo|=0x10; else AF_p->lo&=~0x10;
		}

		if((AF_p->hi&0xF0)>0x90 || AF_p->lo&0x10) AF_p->hi+=0x60;
	}

	if(AF_p->hi==0) AF_p->lo|=0x80; else AF_p->lo&=~0x80;
}

static void cpl(cpu_register *AF_p){
    
    byte complement = ~(AF_p->hi);

    // flag configuration
    AF_p->lo = SET_BIT(AF_p->lo, SUBTRACT_FLAG);
    AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
    
    AF_p->hi = complement;
}

static void ccf(byte *F_p){
 
    // flag configuration

    *F_p = CLEAR_BIT(*F_p, SUBTRACT_FLAG);
    *F_p = CLEAR_BIT(*F_p, HALF_CARRY_FLAG);

    if (TEST_BIT(*F_p, CARRY_FLAG)){
        *F_p = CLEAR_BIT(*F_p, CARRY_FLAG);
    } else {
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }
}

static void scf(byte *F_p){

    *F_p = CLEAR_BIT(*F_p, SUBTRACT_FLAG);
    *F_p = CLEAR_BIT(*F_p, HALF_CARRY_FLAG);
    *F_p = SET_BIT(*F_p, CARRY_FLAG);
}

static void nop(){
    return;
}

static void halt(cpu *cpu_p){
    cpu_p->halted = TRUE;
}

// halt cpu and LCD when button is pressed, need to check how to halt lcd display
static void stop(cpu *cpu_p){
    cpu_p->halted = TRUE;
    // LCD ?
}

static void di(cpu *cpu_p){
    cpu_p->pending_interrupt_enable = FALSE;
}

static void ei(cpu *cpu_p){
    // does pending interrupt disable be disabled ?
    cpu_p->pending_interrupt_enable = TRUE;
}

static void rlc(byte *R_p, cpu_register *AF_p){

    byte result = *R_p << 1;
    // flag configuration

    AF_p->lo = 0;
    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, ZERO_FLAG);
    }

    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);

    byte msb_bit = *R_p & 0x80;
    if (msb_bit){
        AF_p->lo = TEST_BIT(AF_p->lo, CARRY_FLAG);
    }

    *R_p = result;    
}

static void rlc_memory(memory_map *memory_p, word address, cpu_register *AF_p){
    
    byte data = read_memory(memory_p, address);
    byte result = data << 1;

    // flag configuration

    AF_p->lo = 0;
    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, ZERO_FLAG);
    }

    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);

    byte msb_bit = result & 0x80;
    if (msb_bit){
        AF_p->lo = TEST_BIT(AF_p->lo, CARRY_FLAG);
    }

    write_memory(memory_p, address, result);

}

static void rl(byte *R_p, byte *F_p){

    byte carry_set = TEST_BIT(*F_p, CARRY_FLAG);
    byte msb_set = TEST_BIT(*R_p, 7);

    *R_p <<= 1;

    // flag configuration
    if (msb_set){
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }

    if (carry_set){
        *R_p = SET_BIT(*R_p, 0);
    }
    
    if (*R_p == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }
}

static void rl_memory(memory_map *memory_p, word address, byte *F_p){

    byte data = read_memory(memory_p, address);
    byte carry_set = TEST_BIT(*F_p, CARRY_FLAG);
    byte msb_set = TEST_BIT(data, 7);

    data <<= 1;

    // flag configuration
    if (msb_set){
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }

    if (carry_set){
        data = SET_BIT(data, 0);
    }
    
    if (data == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }

    write_memory(memory_p, address, data);
}

static void rrc(byte *R_p, cpu_register *AF_p){
    
    byte result = *R_p >> 1;
    
    // flag configuration
    AF_p->lo = 0;
    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, ZERO_FLAG);
    }

    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);

    byte lsb_bit = *R_p & 0x01;
    if (lsb_bit){
        AF_p->lo = TEST_BIT(AF_p->lo, CARRY_FLAG);
    }

    *R_p = result;     
}

static void rrc_memory(memory_map *memory_p, word address, cpu_register *AF_p){
    
    byte data = read_memory(memory_p, address);
    byte result = data >> 1;

    // flag configuration

    AF_p->lo = 0;
    if (result == 0){
        AF_p->lo = SET_BIT(AF_p->lo, ZERO_FLAG);
    } else {
        AF_p->lo = CLEAR_BIT(AF_p->lo, ZERO_FLAG);
    }

    AF_p->lo = CLEAR_BIT(AF_p->lo, SUBTRACT_FLAG);
    AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);

    byte lsb_bit = result & 0x01;
    if (lsb_bit){
        AF_p->lo = TEST_BIT(AF_p->lo, CARRY_FLAG);
    }

    write_memory(memory_p, address, result);

}

static void rr(byte *R_p, byte *F_p){

    byte carry_set = TEST_BIT(*F_p, CARRY_FLAG);
    byte lsb_set = TEST_BIT(*R_p, 0);

    *R_p >>= 1;

    // flag configuration
    
    if (lsb_set){
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }

    if (carry_set){
        *R_p = SET_BIT(*R_p, 7);
    }
    
    if (*R_p == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }
}

static void rr_memory(memory_map *memory_p, word address, byte *F_p){

    byte data = read_memory(memory_p, address);
    byte carry_set = TEST_BIT(*F_p, CARRY_FLAG);
    byte lsb_set = TEST_BIT(data, 0);

    data >>= 1;

    // flag configuration
    if (lsb_set){
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }

    if (carry_set){
        data = SET_BIT(data, 7);
    }
    
    if (data == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }

    write_memory(memory_p, address, data);
}

static void sla(byte *R_p, byte *F_p){
    
    byte msb_set = TEST_BIT(*R_p, 7);

    byte result = *R_p << 1;
    result = CLEAR_BIT(result, 0);
    
    // flag configuration
    *F_p = 0;
    if (result == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }

    *F_p = CLEAR_BIT(*F_p, SUBTRACT_FLAG);
    *F_p = CLEAR_BIT(*F_p, HALF_CARRY_FLAG);

    if (msb_set){
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }

    *R_p = result;
}

static void sla_memory(memory_map *memory_p, word address, byte *F_p){
    
    byte data = read_memory(memory_p, address);
    byte msb_set = TEST_BIT(data, 7);

    byte result = data << 1;
    result = CLEAR_BIT(result, 0);
    
    // flag configuration
    *F_p = 0;
    if (result == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }

    *F_p = CLEAR_BIT(*F_p, SUBTRACT_FLAG);
    *F_p = CLEAR_BIT(*F_p, HALF_CARRY_FLAG);

    if (msb_set){
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }

    write_memory(memory_p, address, result);
}

static void sra(byte *R_p, byte *F_p){

    byte lsb_set = TEST_BIT(*R_p, 0);
    byte msb_set = TEST_BIT(*R_p, 7);
    byte result = *R_p >> 1;
    
    // flag configuration
    *F_p = 0;
    if (result == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }

    *F_p = CLEAR_BIT(*F_p, SUBTRACT_FLAG);
    *F_p = CLEAR_BIT(*F_p, HALF_CARRY_FLAG);

    if (lsb_set){
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }

    if (msb_set){
        *R_p = SET_BIT(*R_p, 7);
    }

    *R_p = result;
}

static void sra_memory(memory_map *memory_p, word address, byte *F_p){
    
    byte data = read_memory(memory_p, address);
    byte lsb_set = TEST_BIT(data, 0);
    byte msb_set = TEST_BIT(data, 7);

    byte result = data >> 1;
    
    // flag configuration
    *F_p = 0;
    if (result == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }

    *F_p = CLEAR_BIT(*F_p, SUBTRACT_FLAG);
    *F_p = CLEAR_BIT(*F_p, HALF_CARRY_FLAG);

    if (lsb_set){
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }

    if (msb_set){
        result = SET_BIT(result, 7);
    }

    write_memory(memory_p, address, result);
}

static void srl(byte *R_p, byte *F_p){

    byte lsb_set = TEST_BIT(*R_p, 0);

    byte result = *R_p >> 1;
    
    // flag configuration
    *F_p = 0;
    if (result == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }

    *F_p = CLEAR_BIT(*F_p, SUBTRACT_FLAG);
    *F_p = CLEAR_BIT(*F_p, HALF_CARRY_FLAG);

    if (lsb_set){
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }

    *R_p = result;
}

static void srl_memory(memory_map *memory_p, word address, byte *F_p){
    
    byte data = read_memory(memory_p, address);
    byte lsb_set = TEST_BIT(data, 0);

    byte result = data >> 1;
    
    // flag configuration
    *F_p = 0;
    if (result == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    }

    *F_p = CLEAR_BIT(*F_p, SUBTRACT_FLAG);
    *F_p = CLEAR_BIT(*F_p, HALF_CARRY_FLAG);

    if (lsb_set){
        *F_p = SET_BIT(*F_p, CARRY_FLAG);
    }

    write_memory(memory_p, address, result);
}

static void bit(byte position, byte *R_p, byte *F_p){
    
    byte test_bit = TEST_BIT(*R_p, position);

    // flag configuration
    if (test_bit == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    } else {
        *F_p = CLEAR_BIT(*F_p, ZERO_FLAG);
    }

    *F_p = CLEAR_BIT(*F_p, SUBTRACT_FLAG);
    *F_p = SET_BIT(*F_p, HALF_CARRY_FLAG);
}

static void bit_memory(byte position, memory_map *memory_p, word address, byte *F_p){

    byte data = read_memory(memory_p, address);
    byte test_bit = TEST_BIT(data, position);

    // flag configuration
    if (test_bit == 0){
        *F_p = SET_BIT(*F_p, ZERO_FLAG);
    } else {
        *F_p = CLEAR_BIT(*F_p, ZERO_FLAG);
    }

    *F_p = CLEAR_BIT(*F_p, SUBTRACT_FLAG);
    *F_p = SET_BIT(*F_p, HALF_CARRY_FLAG);

}

static void set(byte position, byte *R_p){
    *R_p = SET_BIT(*R_p, position);
}
static void set_memory(byte position, memory_map *memory_p, word address){
    byte data = read_memory(memory_p, address);
    byte result = SET_BIT(data, position);
    write_memory(memory_p, address, result);
}

static void res(byte position, byte *R_p){
    *R_p = CLEAR_BIT(*R_p, position);
}

static void res_memory(byte position, memory_map *memory_p, word address){
    byte data = read_memory(memory_p, address);
    byte result = CLEAR_BIT(data, position);
    write_memory(memory_p, address, result);
}

static void jp(cpu *cpu_p, byte *F_p, byte has_condition, byte condition, byte flag){
    word address = get_immediate_16_bit(cpu_p);

    if (!has_condition){
        cpu_p->PC = address;
        return;
    }

    if (TEST_BIT(*F_p, flag) == condition){
        cpu_p->PC = address;
    }
}

static void jp_hl(cpu *cpu_p){
    cpu_p->PC = get_registers_word(&cpu_p->HL);
}

static void jr(cpu *cpu_p, byte *F_p, byte has_condition, byte condition, byte flag){
    signed_byte value = (signed_byte) get_immediate_8_bit(cpu_p);

    if (!has_condition){
        cpu_p->PC += value;
        return;
    }

    if (TEST_BIT(*F_p, flag) == condition){
        cpu_p->PC += value;
    }
}

static void call(cpu *cpu_p, byte* F_p, byte has_condition, byte condition, byte flag){
    word address = get_immediate_16_bit(cpu_p);
    
    if (!has_condition){
        push_word_to_stack(cpu_p->memory_p, &cpu_p->SP, cpu_p->PC);
        cpu_p->PC = address;
        return;
    }

    if (TEST_BIT(*F_p, flag) == condition){
        push_word_to_stack(cpu_p->memory_p, &cpu_p->SP, cpu_p->PC);
        cpu_p->PC = address;
    }
}


static void push_word_to_stack(memory_map *memory_p, cpu_register *SP_p, word address){
    byte hi_byte = address >> 8;
    byte lo_byte = address & 0xFF;
    word sp_address = get_registers_word(SP_p);
    sp_address--;
    write_memory(memory_p, sp_address, hi_byte);
    sp_address--;
    write_memory(memory_p, sp_address, lo_byte);
    set_registers_word(SP_p, sp_address);
    
}


void initialize_emulator_state(cpu *cpu_p, memory_map *memory_p){
    
    cpu_p->PC = 0x100;   
    cpu_p->AF.hi = 0x01;
    cpu_p->AF.lo = 0xB0;

    cpu_p->BC.hi = 0x00;
    cpu_p->BC.lo = 0x13;
    
    cpu_p->DE.hi = 0x00;
    cpu_p->DE.lo = 0xD8;
    
    cpu_p->HL.hi = 0x01;
    cpu_p->HL.lo = 0x4D;
    cpu_p->SP.hi = 0xFF;
    cpu_p->SP.lo = 0xFE;

    memory_p->memory[0xFF05] = 0x00; 
    memory_p->memory[0xFF06] = 0x00; 
    memory_p->memory[0xFF07] = 0x00; 
    memory_p->memory[0xFF10] = 0x80; 
    memory_p->memory[0xFF11] = 0xBF; 
    memory_p->memory[0xFF12] = 0xF3; 
    memory_p->memory[0xFF14] = 0xBF; 
    memory_p->memory[0xFF16] = 0x3F; 
    memory_p->memory[0xFF17] = 0x00; 
    memory_p->memory[0xFF19] = 0xBF; 
    memory_p->memory[0xFF1A] = 0x7F; 
    memory_p->memory[0xFF1B] = 0xFF; 
    memory_p->memory[0xFF1C] = 0x9F; 
    memory_p->memory[0xFF1E] = 0xBF; 
    memory_p->memory[0xFF20] = 0xFF; 
    memory_p->memory[0xFF21] = 0x00; 
    memory_p->memory[0xFF22] = 0x00; 
    memory_p->memory[0xFF23] = 0xBF; 
    memory_p->memory[0xFF24] = 0x77; 
    memory_p->memory[0xFF25] = 0xF3;
    memory_p->memory[0xFF26] = 0xF1; 
    memory_p->memory[0xFF40] = 0x91; 
    memory_p->memory[0xFF42] = 0x00; 
    memory_p->memory[0xFF43] = 0x00; 
    memory_p->memory[0xFF45] = 0x00; 
    memory_p->memory[0xFF47] = 0xFC; 
    memory_p->memory[0xFF48] = 0xFF; 
    memory_p->memory[0xFF49] = 0xFF; 
    memory_p->memory[0xFF4A] = 0x00; 
    memory_p->memory[0xFF4B] = 0x00; 
    memory_p->memory[0xFFFF] = 0x00; 
}