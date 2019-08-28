#include "cpu.h"

#define ZERO_FLAG 7
#define SUBTRACT_FLAG 6
#define HALF_CARRY_FLAG 5
#define CARRY_FLAG 4

static int execute_opcode(cpu *cpu_p, byte opcode);
static void load_8_bit(cpu *cpu_p, byte *register_p);

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
        case 0x0006:
        load_8_bit(cpu_p, &cpu_p->register_BC.high);
    }

    return 0;
}

// load n (immediate value) into 8 bit registers
static void load_8_bit(cpu *cpu_p, byte *register_p){
    byte n = read_memory(cpu_p->memory_p, cpu_p->PC);
    cpu_p->PC++;
    *register_p = n;
    return ;
}

void initialize_emulator_state(cpu *cpu_p, memory_map *memory_p){
    
    cpu_p->PC = 0x100;
    cpu_p->register_AF.value = 0x01B0;
    cpu_p->register_AF.high = 0x01;
    cpu_p->register_AF.low = 0xB0;

    cpu_p->register_BC.value = 0x0013;
    cpu_p->register_BC.high = 0x00;
    cpu_p->register_BC.low = 0x13;
    
    cpu_p->register_DE.value = 0x00D8;
    cpu_p->register_DE.high = 0x00;
    cpu_p->register_DE.low = 0xD8;
    
    cpu_p->register_HL.value = 0x014D;
    cpu_p->register_HL.high = 0x01;
    cpu_p->register_HL.low = 0x4D;
    cpu_p->register_SP.value = 0xFFFE;
    cpu_p->register_SP.high = 0xFF;
    cpu_p->register_SP.low = 0xFE;

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