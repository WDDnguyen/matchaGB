#ifndef __CPU_H__
#define __CPU_H__

#include "environment.h"
#include "memory.h"

typedef union cpu_register {
    word value;
    struct {
        byte low;
        byte high;
    };
} cpu_register;

typedef struct cpu {
    memory_map *memory_p;
    cpu_register AF;
    cpu_register BC;
    cpu_register DE;
    cpu_register HL;
    cpu_register SP;
    word PC;
    
} cpu;

cpu *initialize_cpu(memory_map *memory_p);
int execute_next_opcode(cpu *cpu_p);
void initialize_emulator_state(cpu *cpu_p, memory_map *memory_p);

#endif
