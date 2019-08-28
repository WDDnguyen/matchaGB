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