#ifndef __CPU_H__
#define __CPU_H__

#include "environment.h"
#include "memory.h"

#define CPU_FREQUENCY 60
#define CPU_MAX_CYCLES 4194304
#define CPU_MAX_CYCLES_PER_SECOND CPU_MAX_CYCLES / CPU_FREQUENCY

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
    byte interrupt_enable;
    byte interrupt_request;

} cpu;

cpu *initialize_cpu(memory_map *memory_p);
int execute_opcode(cpu *cpu_p, byte opcode);
int execute_next_opcode(cpu *cpu_p);
void initialize_game_state(cpu *cpu_p, memory_map *memory_p);
word get_registers_word(cpu_register *register_p);
void push_word_to_stack(memory_map *memory_p, cpu_register *SP_p, word address);
word pop_word_from_stack(memory_map *memory_p, cpu_register *SP_p);

#endif
