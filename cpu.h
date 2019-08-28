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
    cpu_register register_AF;
    cpu_register register_BC;
    cpu_register register_DE;
    cpu_register register_HL;
    cpu_register register_SP;
    word PC;
    
} cpu;

cpu *initialize_cpu(memory_map *memory_p);
int execute_next_opcode(cpu *cpu_p);

#endif
