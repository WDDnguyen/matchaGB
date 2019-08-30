#include "cpu.h"

#define ZERO_FLAG 7
#define SUBTRACT_FLAG 6
#define HALF_CARRY_FLAG 5
#define CARRY_FLAG 4

static int execute_opcode(cpu *cpu_p, byte opcode);
static void load_8_bit_immediate(cpu *cpu_p, byte *p);
static void load_8_bit(byte *p, byte data);
//static void set_value(cpu_register *p);

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

static int execute_opcode(cpu *cpu_p, byte opcode){
    
    switch(opcode){
        
        // load 8 bit immediate data
        case 0x06: load_8_bit_immediate(cpu_p, &cpu_p->BC.high); return 8;
        case 0x0E: load_8_bit_immediate(cpu_p, &cpu_p->BC.low); return 8;
        case 0x16: load_8_bit_immediate(cpu_p, &cpu_p->DE.high); return 8;
        case 0x1E: load_8_bit_immediate(cpu_p, &cpu_p->DE.low); return 8;
        case 0x26: load_8_bit_immediate(cpu_p, &cpu_p->HL.high); return 8;
        case 0x2E: load_8_bit_immediate(cpu_p, &cpu_p->HL.low); return 8;

        // load 8 bit register data
        case 0x7F: load_8_bit(&cpu_p->AF.high, cpu_p->AF.high); return 4;
        case 0x78: load_8_bit(&cpu_p->AF.high, cpu_p->BC.high); return 4;
        case 0x79: load_8_bit(&cpu_p->AF.high, cpu_p->BC.low); return 4;
        case 0x7A: load_8_bit(&cpu_p->AF.high, cpu_p->DE.high); return 4;
        case 0x7B: load_8_bit(&cpu_p->AF.high, cpu_p->DE.low); return 4;
        case 0x7C: load_8_bit(&cpu_p->AF.high, cpu_p->HL.high); return 4;
        case 0x7D: load_8_bit(&cpu_p->AF.high, cpu_p->HL.low); return 4;
        case 0x40: load_8_bit(&cpu_p->BC.high, cpu_p->BC.high); return 4;
        case 0x41: load_8_bit(&cpu_p->BC.high, cpu_p->BC.low); return 4;
        case 0x42: load_8_bit(&cpu_p->BC.high, cpu_p->DE.high); return 4;
        case 0x43: load_8_bit(&cpu_p->BC.high, cpu_p->DE.low); return 4;
        case 0x44: load_8_bit(&cpu_p->BC.high, cpu_p->HL.high); return 4;
        case 0x45: load_8_bit(&cpu_p->BC.high, cpu_p->HL.low); return 4;
        case 0x48: load_8_bit(&cpu_p->BC.low, cpu_p->BC.high); return 4;
        case 0x49: load_8_bit(&cpu_p->BC.low, cpu_p->BC.low); return 4;
        case 0x4A: load_8_bit(&cpu_p->BC.low, cpu_p->DE.high); return 4;
        case 0x4B: load_8_bit(&cpu_p->BC.low, cpu_p->DE.low); return 4;
        case 0x4C: load_8_bit(&cpu_p->BC.low, cpu_p->HL.high); return 4;
        case 0x4D: load_8_bit(&cpu_p->BC.low, cpu_p->HL.low); return 4;
        case 0x50: load_8_bit(&cpu_p->DE.high, cpu_p->BC.high); return 4;
        case 0x51: load_8_bit(&cpu_p->DE.high, cpu_p->BC.low); return 4;
        case 0x52: load_8_bit(&cpu_p->DE.high, cpu_p->DE.high); return 4;
        case 0x53: load_8_bit(&cpu_p->DE.high, cpu_p->DE.low); return 4;
        case 0x54: load_8_bit(&cpu_p->DE.high, cpu_p->HL.high); return 4;
        case 0x55: load_8_bit(&cpu_p->DE.high, cpu_p->HL.low); return 4;
        case 0x58: load_8_bit(&cpu_p->DE.low, cpu_p->BC.high); return 4;
        case 0x59: load_8_bit(&cpu_p->DE.low, cpu_p->BC.low); return 4;
        case 0x5A: load_8_bit(&cpu_p->DE.low, cpu_p->DE.high); return 4;
        case 0x5B: load_8_bit(&cpu_p->DE.low, cpu_p->DE.low); return 4;
        case 0x5C: load_8_bit(&cpu_p->DE.low, cpu_p->HL.high); return 4;
        case 0x5D: load_8_bit(&cpu_p->DE.low, cpu_p->HL.low); return 4;
        case 0x60: load_8_bit(&cpu_p->HL.high, cpu_p->BC.high); return 4;
        case 0x61: load_8_bit(&cpu_p->HL.high, cpu_p->BC.low); return 4;
        case 0x62: load_8_bit(&cpu_p->HL.high, cpu_p->DE.high); return 4;
        case 0x63: load_8_bit(&cpu_p->HL.high, cpu_p->DE.low); return 4;
        case 0x64: load_8_bit(&cpu_p->HL.high, cpu_p->HL.high); return 4;
        case 0x65: load_8_bit(&cpu_p->HL.high, cpu_p->HL.low); return 4;
        case 0x68: load_8_bit(&cpu_p->HL.low, cpu_p->BC.high); return 4;
        case 0x69: load_8_bit(&cpu_p->HL.low, cpu_p->BC.low); return 4;
        case 0x6A: load_8_bit(&cpu_p->HL.low, cpu_p->DE.high); return 4;
        case 0x6B: load_8_bit(&cpu_p->HL.low, cpu_p->DE.low); return 4;
        case 0x6C: load_8_bit(&cpu_p->HL.low, cpu_p->HL.high); return 4;
        case 0x6D: load_8_bit(&cpu_p->HL.low, cpu_p->HL.low); return 4;
        
        // write register with memory at (HL)
        case 0x7E: load_8_bit(&cpu_p->AF.high, read_memory(cpu_p->memory_p, cpu_p->HL.value)); return 8;
        case 0x46: load_8_bit(&cpu_p->BC.high, read_memory(cpu_p->memory_p, cpu_p->HL.value)); return 8;
        case 0x4E: load_8_bit(&cpu_p->BC.low, read_memory(cpu_p->memory_p, cpu_p->HL.value)); return 8;
        case 0x56: load_8_bit(&cpu_p->DE.high, read_memory(cpu_p->memory_p, cpu_p->HL.value)); return 8;
        case 0x5E: load_8_bit(&cpu_p->DE.low, read_memory(cpu_p->memory_p, cpu_p->HL.value)); return 8;
        case 0x66: load_8_bit(&cpu_p->HL.high, read_memory(cpu_p->memory_p, cpu_p->HL.value)); return 8;
        case 0x6E: load_8_bit(&cpu_p->HL.low, read_memory(cpu_p->memory_p, cpu_p->HL.value)); return 8; 
        
        // write into memory at (HL)
        case 0x70: write_memory(cpu_p->memory_p, cpu_p->HL.value, cpu_p->BC.high); return 8;
        case 0x71: write_memory(cpu_p->memory_p, cpu_p->HL.value, cpu_p->BC.low); return 8;
        case 0x72: write_memory(cpu_p->memory_p, cpu_p->HL.value, cpu_p->DE.high); return 8;
        case 0x73: write_memory(cpu_p->memory_p, cpu_p->HL.value, cpu_p->DE.low); return 8;
        case 0x74: write_memory(cpu_p->memory_p, cpu_p->HL.value, cpu_p->HL.high); return 8;
        case 0x75: write_memory(cpu_p->memory_p, cpu_p->HL.value, cpu_p->HL.low); return 8;
        case 0x36:write_memory(cpu_p->memory_p, cpu_p->HL.value, read_memory(cpu_p->memory_p, cpu_p->PC));
            cpu_p->PC++;
            return 8;
    
        // load 8 bit data
        
        
        
    }

    return 0;
}

// load n (immediate value) into 8 bit registers
static void load_8_bit_immediate(cpu *cpu_p, byte *register_p){
    byte n = read_memory(cpu_p->memory_p, cpu_p->PC);
    cpu_p->PC++;
    *register_p = n;
}

static void load_8_bit(byte *register_p, byte data){
    *register_p = data;
}

void initialize_emulator_state(cpu *cpu_p, memory_map *memory_p){
    
    cpu_p->PC = 0x100;
    cpu_p->AF.value = 0x01B0;
    cpu_p->AF.high = 0x01;
    cpu_p->AF.low = 0xB0;

    cpu_p->BC.value = 0x0013;
    cpu_p->BC.high = 0x00;
    cpu_p->BC.low = 0x13;
    
    cpu_p->DE.value = 0x00D8;
    cpu_p->DE.high = 0x00;
    cpu_p->DE.low = 0xD8;
    
    cpu_p->HL.value = 0x014D;
    cpu_p->HL.high = 0x01;
    cpu_p->HL.low = 0x4D;
    cpu_p->SP.value = 0xFFFE;
    cpu_p->SP.high = 0xFF;
    cpu_p->SP.low = 0xFE;

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