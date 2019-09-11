#include "environment.h"
#include "cartridge.h"
#include "memory.h"
#include "cpu.h"

// need to check how master interrupt is enabled
static byte master_interrupt_enabled;
static void emulate(cpu *cpu_p);
static void update_timers(cpu *cpu_p, int cycles);
static void run_interrupts(cpu *cpu_p);
static void request_interrupt(cpu *cpu_p, int id);
static void service_interrupt(cpu *cpu_p, int interrupt_id);
static int timer_counter;
static int divider_register;
static byte get_clock_frequency(memory_map *memory_p);
static void set_clock_frequency(memory_map *memory_p);
static byte clock_enabled(memory_map *memory_p);
static void update_timers(cpu *cpu_p, int cycles);
static void update_divider_register(memory_map *memory_p, int cycles);

int main(void){

    cartridge *cartridge_p = initialize_cartridge("Tetris.gb");
    memory_map *memory_p = initialize_memory(cartridge_p);
    cpu *cpu_p = initialize_cpu(memory_p);

    initialize_emulator_state(cpu_p, memory_p);

    // initialize timer_counter
    set_clock_frequency(memory_p);

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
    while (cycles_used < CPU_MAX_CYCLES_PER_SECOND){
        int opcode_cycles = execute_next_opcode(cpu_p);
        cycles_used += opcode_cycles;
        update_timers(cpu_p, cycles_used);
        run_interrupts(cpu_p);
    }
}

static byte get_clock_frequency(memory_map *memory_p){
    return read_memory(memory_p, TMA_INDEX);
}

static void set_clock_frequency(memory_map *memory_p){
    byte tma = get_clock_frequency(memory_p);

    switch(tma){
        case 0: timer_counter = 1024; break; // frequency 4096
        case 1: timer_counter = 16; break; // frequency 262144
        case 2: timer_counter = 64; break; // frequency 65536
        case 3: timer_counter = 256; break; // frequency 16382
    }
}

static byte clock_enabled(memory_map *memory_p){
    // bit 2 enable or disable the clock
    byte enabled = TEST_BIT(read_memory(memory_p, TMC_INDEX), 2) ? TRUE : FALSE;
    return enabled;
}

static void update_timers(cpu *cpu_p, int cycles){

    update_divider_register(cpu_p->memory_p, cycles);

    if (clock_enabled(cpu_p->memory_p)){
        timer_counter -= cycles;
        
        // reset timer counter value
        if (timer_counter <= 0){
            set_clock_frequency(cpu_p->memory_p);
        }

        // interrupt requested when TIMA overflows
        if (read_memory(cpu_p->memory_p, TIMA_INDEX) == 255){
            write_memory(cpu_p->memory_p, TIMA_INDEX, 0);
            service_interrupt(cpu_p, 0);
        } 
        else {
            write_memory(cpu_p->memory_p, TIMA_INDEX, (read_memory(cpu_p->memory_p, TIMA_INDEX) + 1));
        }
    }
}

static void update_divider_register(memory_map *memory_p, int cycles){
    divider_register += cycles;
    // divider register overflow
    if (divider_register >= 255){
        divider_register = 0;
        // directly increment memory value since writing to this memory address resets value to 0
        memory_p->memory[0xFF04]++; 
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
