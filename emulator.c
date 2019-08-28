#include "environment.h"
#include "cartridge.h"
#include "memory.h"
#include "cpu.h"

int main(void){

    cartridge *cartridge_p = initialize_cartridge("Tetris.gb");
    memory_map *memory_p = initialize_memory(cartridge_p);
    cpu *cpu_p = initialize_cpu(memory_p);

    initialize_emulator_state(cpu_p, memory_p);

    free(cartridge_p);
    cartridge_p = NULL;

    free(memory_p);
    memory_p = NULL;
    
    free(cpu_p);
    cpu_p = NULL;

    return 0;
}
