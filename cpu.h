#ifndef __CPU_H__
#define __CPU_H__

#include "environment.h"
#include "memory.h"

typedef union cpu_register {
    struct {
        byte lo;
        byte hi;
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
    byte halted;
    byte pending_interrupt_enable;

} cpu;

cpu *initialize_cpu(memory_map *memory_p);
int execute_opcode(cpu *cpu_p, byte opcode);
int execute_next_opcode(cpu *cpu_p);
void initialize_emulator_state(cpu *cpu_p, memory_map *memory_p);
word get_registers_word(cpu_register *register_p);

#endif
