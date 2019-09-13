#include "environment.h"
#include "cartridge.h"
#include "memory.h"
#include "cpu.h"

// set by EI and ACK the interrupt setting by the IE register
static byte interrupt_master_enable;
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

static int scanline_counter = 456;
static void set_lcd_status(cpu *cpu_p);
static bool lcd_enabled(memory_map *memory_p);
static void update_graphics(cpu *cpu_p, int cycles);

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

/*
    Interrupt Procedure
        IME (interrupt_master_enable) reset by DI (disable interrupt) and prohibits all interrupts
        IME is set by EI and acknowledges the interrupt setting by the IE register 

        1- When the interrupt is generated, the IF flag will be set
        2- if the  IME flag is set and the corresponding IE flag is set then step 3-5 are executed
        3-  Reset the IME flag and prevent all interrupts
        4- PC is pushed onto the stack
        5- Jump to the starting address of the interrupt

    return from an interrupt routine can be performed by either RETI or RET instruction
      if RET is used as final operation, interrupt are disabled until a EI was used in interrupt routine.
 */
static void run_interrupts(cpu *cpu_p){

    if (interrupt_master_enable){
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
    interrupt_master_enable = FALSE;
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

/*
Screen resolution is 160 x 144 
    - only 144 visible lines, 8 invisible
    - Vertical blank between 144-153 
        - 

Scanline takes 456 cycles to complete before switching to next line 
  -  increment memory from LY_INDEX when enough cycles passed.
*/

static void update_graphics(cpu *cpu_p, int cycles){
    set_lcd_status(cpu_p);
    
    // scanline_counter decrement starting from 456 when lcd is enabled
    if(lcd_enabled(cpu_p->memory_p)){
        scanline_counter -= cycles;
    } else {
        return;
    }

    if (scanline_counter <= 0){
        // move to next scanline
        cpu_p->memory_p->memory[LY_INDEX]++;
        byte current_line = read_memory(cpu_p->memory_p, LY_INDEX);

        // in vertical blank
        if (current_line == 144){
            request_interrupt(cpu_p, 0);
        }
        
        // wrap around back to 0
        else if (current_line > 153){
            cpu_p->memory_p->memory[LY_INDEX] = 0;
        }

        else if (current_line < 144){
            //draw_scanline();
        }
    }
}

static void set_lcd_status(cpu *cpu_p){
    byte status = read_memory(cpu_p->memory_p, LCDC_STATUS_INDEX);

    if (lcd_enabled(cpu_p->memory_p)){
        // set mode to 1 when lcd is disabled and reset scanline
        scanline_counter = 456;
        cpu_p->memory_p->memory[LY_INDEX] = 0;
        // keep all the bits but modify the last 2 bits to become V-BLANK mode 1
        status &= 252;
        status = SET_BIT(status, 0);
        write_memory(cpu_p->memory_p, LCDC_STATUS_INDEX, status);
        return;
    }

    byte current_line = read_memory(cpu_p->memory_p, LY_INDEX);
    byte current_mode = status & 0x3;

    /*
        4 modes
        00 - H-BLANK
        01 - V-BLANK
        10 - OAM search
        11 - Transfering Data to LCD driver

        mode 2 when first 80 cycles
        mode 3 when 172 cycles after mode 2 = 172 + 80 = 202
        mode 0 when the rest of the cycles 
     */
    byte mode = 0;
    bool interrupted = FALSE;

    if (current_line > 144){
        mode = 1;
        status = SET_BIT(status, 0);
        status = CLEAR_BIT(status, 1);
        interrupted = TEST_BIT(status, 4);
    } else {
        int mode2_bounds = 456-80;
        int mode3_bounds = mode2_bounds - 172;

        // mode 2 
        if (scanline_counter >= mode2_bounds){
            mode = 2;
            status = SET_BIT(status, 1);
            status = SET_BIT(status, 0);
            interrupted = TEST_BIT(status, 5);
        } else if (scanline_counter >= mode3_bounds){
            mode = 3;
            status = SET_BIT(status, 1);
            status = SET_BIT(status, 0);
        } else {
            mode = 0;
            status = CLEAR_BIT(status, 1);
            status = CLEAR_BIT(status, 0);
            interrupted = TEST_BIT(status, 3);
        }
    }

    if (interrupted && (mode != current_mode)){
        service_interrupt(cpu_p, 1);
    }

    // when 0xFF44 == 0xFF45
    if (current_line == read_memory(cpu_p->memory_p, LYC_INDEX)){
        // bit 2 (coincidence flag) is set when condition is true 
        status = SET_BIT(status, 2);
        // if bit 6 and bit 2 are enabled then request interrupt
        if (TEST_BIT(status, 6)){
            request_interrupt(cpu_p, 1);
        }
    } else {
        status = CLEAR_BIT(status, 2);
    }

    write_memory(cpu_p->memory_p, LCDC_STATUS_INDEX, status);
}

static bool lcd_enabled(memory_map *memory_p){
    bool enabled = TEST_BIT(read_memory(memory_p, LCDC_INDEX), 7);
    return enabled;
}