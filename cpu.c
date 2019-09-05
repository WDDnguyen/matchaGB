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
static void increment(cpu_register *register_p);
static void decrement(cpu_register *register_p);

static word address;
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
        printf("\nCARRY OVERFLOW\n");
        AF_p->lo = SET_BIT(AF_p->lo, CARRY_FLAG);
    } else {
        printf("\n NO CARRY OVERFLOW\n");
        AF_p->lo = CLEAR_BIT(AF_p->lo, CARRY_FLAG);
    }

    overflow = (get_registers_word(&cpu_p->SP) & 0xF) + (n & 0xF);
    
    if (overflow > 0xF){
        printf("\n AF BEFORE %x\n", AF_p->lo);
        AF_p->lo = SET_BIT(AF_p->lo, HALF_CARRY_FLAG);
        printf("\n AF AFTER %x\n", AF_p->lo);
    } else {
        printf("\n NO HALF OVERFLOW\n");
        AF_p->lo = CLEAR_BIT(AF_p->lo, HALF_CARRY_FLAG);
    }
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