#include "environment.h"
#include "cartridge.h"
#include "memory.h"
#include "cpu.h"

#define FREQUENCY 60
#define MAX_CYCLES 4194304
#define MAX_CYCLES_PER_SECOND MAX_CYCLES / FREQUENCY

static void emulate();

int main(void){

    cartridge *cartridge_p = initialize_cartridge("Tetris.gb");
    memory_map *memory_p = initialize_memory(cartridge_p);
    cpu *cpu_p = initialize_cpu(memory_p);

    initialize_emulator_state(cpu_p, memory_p);

    emulate(cpu_p);

    free(cartridge_p);
    cartridge_p = NULL;

    free(memory_p);
    memory_p = NULL;
    
    free(cpu_p);
    cpu_p = NULL;

    return 0;
}

static void emulate(cpu *cpu_p){

    int cycles_used = 0;
    while (cycles_used < MAX_CYCLES_PER_SECOND){
        int opcode_cycles = execute_next_opcode(cpu_p);
        cycles_used += opcode_cycles;
    }
}
