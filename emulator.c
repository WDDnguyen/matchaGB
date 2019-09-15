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
static void render_sprites(memory_map *memory_p, byte lcdc);
static void render_tiles(memory_map *memory_p, byte lcdc);
static int bit_get_value(byte data, int position);
byte get_color(memory_map *memory_p, byte column_number, word address);
static void draw_scanline(memory_map *memory_p);

static byte screen_data[160][143][3];

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
            draw_scanline(cpu_p->memory_p);
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

static void draw_scanline(memory_map *memory_p){
    byte lcdc = read_memory(memory_p, LCDC_INDEX);
    
    // background
    if (TEST_BIT(lcdc, 0)){
        render_tiles(memory_p, lcdc);
    }

    // sprite
    if (TEST_BIT(lcdc, 1)){
        render_sprites(memory_p, lcdc);
    }
}

// if the background layout give an unsigned tile identifier as 0 then the tile would be between 0x800-0x800F

// algorithm to write region 0x800 - 0x97FF - if identifier is 0 then memory region is 0x9000- 0x900F   beside of 0x8800 - 0x880F
//word tile_address = VRAM_INDEX + (tile_identifier + offset * TILE_SIZE);

// tile in memory needs 16 bytes of data
// if two bytes of data form 1 line then, need to combine 2 bytes to form a break down of each pixel in the 8 pixel line 

/*
pixel# = 1 2 3 4 5 6 7 8
data 2 = 1 0 1 0 1 1 1 0
data 1 = 0 0 1 1 0 1 0 1

Pixel 1 colour id: 10
Pixel 2 colour id: 00
Pixel 3 colour id: 11
Pixel 4 colour id: 01
Pixel 5 colour id: 10
Pixel 6 colour id: 11
Pixel 7 colour id: 10
Pixel 8 colour id: 01

only 4 possible color is  00, 01, 10, 11
- need to map to correct color
  - This is what palettes are used, (palette are not fixed), the programmer can change the mapping
  - This means you can change the colour of tiles and sprites without the tile data.
    - Can amke cool special effects 

    - Background has monochrome color palette located in 0xFF47
    - Sprite can have 2 palettes 0xFF48 0xFF49
      - Can use white as transparret 


    - Every 2 bits in the palette data represent a colour
        - Bit 7-6 colour id 11 : black
        - Bit 5-4 color id 10 : dark gray
        - Bit 3-2 color id 01 : light gray
        - Bit 1-0 color id 00 : white

 */

static void render_tiles(memory_map *memory_p, byte lcdc){
    word tile_data = 0;
    word background_memory = 0;
    bool unsig = TRUE;

    // where to draw the visial area and the window
    byte scroll_y = read_memory(memory_p, SCROLL_Y_INDEX);
    byte scroll_x = read_memory(memory_p, SCROLL_X_INDEX);
    byte window_y = read_memory(memory_p, WINDOW_Y_INDEX);
    byte window_x = read_memory(memory_p, WINDOW_X_INDEX);
    
    bool windowed = FALSE;

    if (TEST_BIT(lcdc, 5)){
        // check is current scanline is within the window Y
        if (window_y <= read_memory(memory_p, LY_INDEX)){
            windowed = TRUE;
        }
    }

    // background and window tile data selection
    if (TEST_BIT(lcdc, 4)){
        tile_data = 0x8000;
    } else {
        // Selecting 0x8800 - 0x97FF tile identifier are SIGNED 
        tile_data = 0x8800;
        unsig = FALSE;
    }

    if (!windowed){
        // which background memory
        if (TEST_BIT(lcdc, 3)){
            background_memory = 0x9C00;
        } else {
            background_memory = 0x9800;
        }
    } else {
        // which window memory
        if (TEST_BIT(lcdc, 6)){
            background_memory = 0x9C00;
        } else {
            background_memory = 0x9800;
        }
    }

    byte y_position = 0;
    
    // y position used to  calce which 32 vertical tiles the current scanline is drawing
    if (!windowed){
        y_position = scroll_y + read_memory(memory_p, LY_INDEX);
    } else {
        y_position = read_memory(memory_p, LY_INDEX) - window_y;
    }

    // which 8 vertical pixel are currently tile is the scanline on - recheck
    word tile_row = (((byte) (y_position / 8)) * 32);

    // draw the 160 horizontal pixels for the scanline
    for (int pixel = 0; pixel < 160; pixel++){
        byte x_position = pixel + scroll_x;

        // translate the current x position to window space if necessary
        if (windowed){
            if (pixel >= window_x){
                x_position = pixel - window_x;
            }
        }
        // which of the 32 horizontal tile does this x_position fall within
        word tile_column = (x_position / 8);
        signed_word tile_number;

        // get tile identity number and check if signed or unsigned

        word tile_address = background_memory + tile_row + tile_column;
        if (unsig){
            tile_number = (byte) read_memory(memory_p, tile_address);
        } else {
            tile_number = (signed_byte) read_memory(memory_p, tile_address);
        }

        // find where the tile identifier is in memory
        word tile_location = tile_data;

        if (unsig){
            tile_location += (tile_number * TILE_SIZE);
        } else {
            tile_location += ((tile_number + 128) * TILE_SIZE);
        }

        // find correct vertical line of the tile to get the tile data from memory
        byte line = y_position % 8;
        line *= 2; // each vertical line takes 2 bytes of memory
        byte data1 = read_memory(memory_p, tile_location + line);
        byte data2 = read_memory(memory_p, tile_location + line + 1);

        // pixel 0 in the tile is 7 of data 1 and data 2
        int color_bit = x_position & 8;
        color_bit -= 7;
        color_bit *= -1;

        // combine data 2 and data 1 to get color id for this pixel of the tile
        int color_number = bit_get_value(data2, color_bit);
        color_number <<= 1;
        color_number |= bit_get_value(data1, color_bit);

        // now we have the color id - get the actual color from palette 0xFF47


        // get color 
        byte col = get_color(memory_p, color_number, BACKGROUND_PALETTE);
        int red = 0;
        int green = 0;
        int blue = 0;

        // setup RGB values
        
        switch(col){
            case 0 : red = 255; green = 255;; blue = 255; break; // WHITE
            case 1: red = 0xCC; green = 0xCC ; blue = 0xCC; break ; // LIGHT GRAY
            case 2:	red = 0x77; green = 0x77 ; blue = 0x77; break ; // DARK GRAY
        }
        

        int final_y = read_memory(memory_p, LY_INDEX);

        // safety check that in bound 
        if ((final_y < 0 ) || (final_y > 143) || (pixel < 0) || (pixel > 159)){
            continue;
        }

        screen_data[pixel][final_y][0] = red;
        screen_data[pixel][final_y][1] = green;
        screen_data[pixel][final_y][2] = blue;

    }
}


byte get_color(memory_map *memory_p, byte column_number, word address){
    byte result = 0;
    byte palette = read_memory(memory_p, address);
    int hi = 0;
    int lo = 0;

    // which bit of the color palette does the color id map to
    switch(column_number){
        case 0: hi = 1; lo = 0; break;
        case 1: hi = 3; lo = 2; break;
        case 2: hi = 5; lo = 4; break;
        case 3: hi = 7; lo = 6; break;
    }

    // use the palette to get the color
    int color = 0;
    color = bit_get_value(palette, hi) << 1;
    color |= bit_get_value(palette, lo);

    //convert the game color to emulator color
    switch(color){
        case 0: result = 0; break; // WHITE
        case 1: result = 1; break; // LIGHT_GRAY
        case 2: result = 2; break; // DARK_GRAY 
        case 3: result = 3; break; // BLACK 
    }

    return result;
}

static int bit_get_value(byte data, int position){
    byte mask = 1 << position ;
	return ( data & mask ) ? 1 : 0 ;
}

/* all sprites located in 0x8000-0x8FFF
all sprites identifiers are unsigned value so easy to find
40 tiles located in 0x8000-0x8FFF
scan through them all and check their attribites to find where they rendered
sprite attribute found in attribute table in 0xFE00-0xFE9F
Each sprite has 4 bytes associated to it 

0: Sprite Y Position: Position of the sprite on the Y axis of the viewing display minus 16
1: Sprite X Position: Position of the sprite on the X axis of the viewing display minus 8
2: Pattern number: This is the sprite identifier used for looking up the sprite data in memory region 0x8000-0x8FFF
3: Attributes:
    Bit7: Sprite to Background Priority
        - 0 then sprite rendered above the background and the window.
        - 1 hide behind the background
    Bit6: Y flip
      - sprite becomes upside down
    Bit5: X flip
      - change direction of character
    Bit4: Palette number
      - Sprite can either be in palette 0xFF48 or 0xFF49
    Bit3: Not used in standard gameboy
    Bit2-0: Not used in standard gameboy

A sprite can be 8x8 pixel or 8x16 pixels

*/

static void render_sprites(memory_map *memory_p, byte lcdc){

    // spirte size configuration
    bool use8x16 = FALSE;
    if (TEST_BIT(lcdc, 2)){
        use8x16 = TRUE;
    }

    for (int sprite = 0; sprite < 40; sprite++){
        // sprite has 4 bytes in OAM table
        byte index = sprite * 4;
        byte y_position = read_memory(memory_p, OAM_INDEX + index) - 16;
        byte x_position = read_memory(memory_p, OAM_INDEX + index + 1) - 8;
        byte tile_location = read_memory(memory_p, OAM_INDEX + index + 2);
        byte attributes = read_memory(memory_p, OAM_INDEX + index + 3);

        bool y_flip = TEST_BIT(attributes, 6);
        bool x_flip = TEST_BIT(attributes, 5);

        int scanline = read_memory(memory_p, LY_INDEX);

        int y_size = 8;
        if (use8x16){
            y_size = 16;
        }

        // does this sprite intercept with the scanline
        if ((scanline >= y_position) && (scanline < (y_position + y_size))){
            int line = scanline - y_position;


            // read the sprite in backwards in the y axis
            if (y_flip){
                line -= y_size;
                line *= -1;
            }

            line *=2; // same as tiles
            word data_address = (0x8000 + (tile_location * 16)) + line;
            byte data1 = read_memory(memory_p, data_address);
            byte data2 = read_memory(memory_p, data_address + 1);

            // its easier to read in from right to left as pixel 0 is bit 7 in the color data

            for (int tile_pixel = 7; tile_pixel >= 0; tile_pixel--){
                int color_bit = tile_pixel;
                // read the sprite in backwards for the x acis
                if (x_flip){
                    color_bit -= 7;
                    color_bit *= -1;
                }

                // the rest is the same as for tile 
                int color_number = bit_get_value(data2, color_bit);
                color_number <<= 1;
                color_number |= bit_get_value(data1, color_bit);

                word color_address = TEST_BIT(attributes, 4) ? 0xFF49 : 0xFF48;
                byte col = get_color(memory_p, color_number, color_address);

                // white is transparent for sprites
                if (col == 0){
                    continue;
                }

                int red = 0;
                int green = 0;
                int blue = 0;

                switch(col){
                    case 0: red =255; green=255; blue=255; break; // WHITE
                    case 1:red =0xCC; green=0xCC; blue=0xCC; break; // LIGHT_GRAY
                    case 2:red=0x77; green=0x77; blue=0x77; break; // DARK GRAY
                }

                int x_pixel = 0 - tile_pixel;
                x_pixel += 7;

                int pixel = x_position + x_pixel;

                // sanity check

                screen_data[pixel][scanline][0] = red;
                screen_data[pixel][scanline][1] = green;
                screen_data[pixel][scanline][2] = blue;
                
            }
        }
    }
}