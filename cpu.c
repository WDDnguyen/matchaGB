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