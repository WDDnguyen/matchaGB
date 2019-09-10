#include "environment.h"
#include "cartridge.h"
#include "memory.h"
#include "cpu.h"

#define FREQUENCY 60
#define MAX_CYCLES 4194304
#define MAX_CYCLES_PER_SECOND MAX_CYCLES / FREQUENCY

// need to check how master interrupt is enabled
static byte master_interrupt_enabled;
static void emulate(cpu *cpu_p);
static void update_timers(int cycles);
static void run_interrupts(cpu *cpu_p);
static void request_interrupt(cpu *cpu_p, int id);
static void service_interrupt(cpu *cpu_p, int interrupt_id);

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
        // To do : timer updates
        run_interrupts(cpu_p);
    }
}

static void run_interrupts(cpu *cpu_p){

    if (master_interrupt_enabled){
        byte ir_content = read_memory(cpu_p->memory_p, INTERRUPT_REQUEST_INDEX);
        byte ie_content = read_memory(cpu_p->memory_p, INTERRUPT_ENABLE_INDEX);

        // run all interrupts that are both in interrupt request and interrupt enable based on priority
        if (ir_content > 0){
            for (int interrupt_index = 0; interrupt_index < MAX_INTERRUPTS + 1; interrupt_index++){
                if (TEST_BIT(ie_content, interrupt_index)){
                    if (TEST_BIT(ir_content, interrupt_index)){
                        service_interrupt(cpu_p, interrupt_index);
                    }
                }
            }
        }
    }
}

static void request_interrupt(cpu *cpu_p, int id){
    byte request_value = read_memory(cpu_p->memory_p, INTERRUPT_REQUEST_INDEX);
    request_value = SET_BIT(request_value, id);
    write_memory(cpu_p->memory_p, INTERRUPT_REQUEST_INDEX, request_value);
}

static void service_interrupt(cpu *cpu_p, int interrupt_id){

    // when servicing interrupt, master interrupt is turned off and interrupt request memory cleared of interrupt serviced.
    master_interrupt_enabled = FALSE;
    byte ir_content = read_memory(cpu_p->memory_p, INTERRUPT_REQUEST_INDEX);
    ir_content = CLEAR_BIT(ir_content, interrupt_id);
    write_memory(cpu_p->memory_p, INTERRUPT_REQUEST_INDEX, ir_content);

    push_word_to_stack(cpu_p->memory_p, &cpu_p->SP, cpu_p->PC);
    
    switch(interrupt_id){
        case 0: cpu_p->PC = 0x40; break;
        case 1: cpu_p->PC = 0x48; break;
        case 2: cpu_p->PC = 0x50; break;
        case 4: cpu_p->PC = 0x60; break;
    }
}
