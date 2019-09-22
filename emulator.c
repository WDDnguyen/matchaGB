#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "environment.h"
#include "cartridge.h"
#include "memory.h"
#include "cpu.h"

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

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
static void step(cpu *cpu_p, int iterations);

static byte screen_data[SCREEN_WIDTH][SCREEN_HEIGHT][3];
void print_screen_data();
static void step_graphics(cpu *cpu_p, int iterations);

void setup_gl_context();
void initialize_gl_context();
void initialize_sdl_window();
void initialize_screen_data();
void render_screen();
void print_cpu_content(cpu *cpu_p);

// window to render screen 
SDL_Window* sdl_window = NULL;

// creat opengl context associated with the window
SDL_GLContext gl_context = NULL;

int main(void){

    cartridge *cartridge_p = initialize_cartridge("DMG_ROM.bin");
    set_nintendo_logo_data(cartridge_p);
    //cartridge *cartridge_p = initialize_cartridge("Tetris.gb");
    memory_map *memory_p = initialize_memory(cartridge_p);
    cpu *cpu_p = initialize_cpu(memory_p);

    //initialize_game_state(cpu_p, memory_p);

    printf("PC value %04X\n", cpu_p->PC);

    // initialize timer_counter
    set_clock_frequency(memory_p);

    initialize_sdl_window();
    initialize_gl_context();

    /* bootstrap testing in steps
    
    Done :
        - Initialize SP: 1 iteration
        - clear VRAM from 0x9FFF to 0x8000: 24579 iterations
        - Audio setup : 10 iterations
        - Initial palette, write nintendo logo into VRAM + R into Vram : 4086 iterations

    In progress :
        - TILE MAP 132 to get to scrolling 
        - SCROLLING 
    */ 

    //initialize_screen_data();
    
    step(cpu_p, 1);
    step(cpu_p, 24579);
    //print_vram_memory(cpu_p->memory_p);
    step(cpu_p, 10);
    step(cpu_p, 2); // initialize color palette with 0xFC
    // render screen
    
    // print_screen_data();
    // render_screen();
    
    step(cpu_p, 4084);
    //print_vram_memory(cpu_p->memory_p);
    step(cpu_p, 132);
    // print_tile_map_0(memory_p);
    step(cpu_p, 6); // enable LCD
    //step_graphics(cpu_p, 1);
    step_graphics(cpu_p, 154);
    print_screen_data();
    //print_vram_memory(cpu_p->memory_p);
    
    SDL_Event event;
    bool exit_sdl = FALSE;
    int iteration = 0;

    while (iteration < 5000 && exit_sdl == FALSE) {
        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_QUIT){
                exit_sdl = TRUE;
                continue;
            }

            if (event.type == SDL_KEYDOWN) {
                iteration++;
                // emulating 1 frame
                //emulate(cpu_p);

                int cycles = execute_next_opcode(cpu_p);
                //cycles_used += cycles;
                update_timers(cpu_p, cycles);
                //run_interrupts(cpu_p);
                //update_graphics(cpu_p, cycles);
                print_cpu_content(cpu_p);
            }
        }
    }

    printf("NEW iterations %d\n", iteration);

    free(cartridge_p);
    cartridge_p = NULL;

    free(memory_p);
    memory_p = NULL;
    
    free(cpu_p);
    cpu_p = NULL;

    return 0;
}

static void step_graphics(cpu *cpu_p, int iterations){
    
    int cycles_used = 0;
    int cycles = 456;
    for (int i = 0; i < iterations; i++){
        cycles_used += cycles;
        update_graphics(cpu_p, cycles);
        if (cycles_used >= CPU_MAX_CYCLES_PER_SECOND){
            cycles_used = 0;
            render_screen();
        }
    }
    
}

static void step(cpu *cpu_p, int iterations){

    for(int i = 0; i < iterations; i++){
        execute_next_opcode(cpu_p);
    }
    print_cpu_content(cpu_p);
}

void print_screen_data(){
    for(byte width = 0; width < SCREEN_WIDTH; width++){
        printf("\n");
        for (byte height = 0; height < SCREEN_HEIGHT; height++){
            byte r = screen_data[width][height][0];
            byte b = screen_data[width][height][1];
            byte g = screen_data[width][height][2];
            if (r == 0){
                printf("B"); // black
            }
            else if (r == 255){
                printf("W"); // white
            }
            
        }
    }
}

static void emulate(cpu *cpu_p){

    int cycles_used = 0;
    while (cycles_used < CPU_MAX_CYCLES_PER_SECOND){
        int cycles = execute_next_opcode(cpu_p);
        cycles_used += cycles;
        //update_timers(cpu_p, cycles);
        //run_interrupts(cpu_p);
        update_graphics(cpu_p, cycles);
    }
    render_screen();
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

        byte current_line = read_memory(cpu_p->memory_p, LY_INDEX);
        printf("CURRENT LINE %u \n", current_line);

        // reset counter
        scanline_counter = 456;

        // in vertical blank
        if (current_line == 144){
            //request_interrupt(cpu_p, 0);
        }
        
        // wrap around back to 0
        else if (current_line > 153){
            cpu_p->memory_p->memory[LY_INDEX] = 0;
        }

        else if (current_line < 144){
            draw_scanline(cpu_p->memory_p);
        }
        cpu_p->memory_p->memory[LY_INDEX]++;
    }
}

static void set_lcd_status(cpu *cpu_p){
    byte status = read_memory(cpu_p->memory_p, LCDC_STATUS_INDEX);

    // if LCD is turned off
    if (lcd_enabled(cpu_p->memory_p) == FALSE){
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

    if (current_line >= 144){
        mode = 1;
        // set status 2 LSB : 01
        // check for V blank interrupts
        status = SET_BIT(status, 0);
        status = CLEAR_BIT(status, 1);
        interrupted = TEST_BIT(status, 4);
    } else {
        int oam_cycles = 80;
        int vram_cycles = 172;
        int mode2_bounds = 456 - oam_cycles;
        int mode3_bounds = mode2_bounds - vram_cycles;

        // mode 2 
        if (scanline_counter >= mode2_bounds){
            mode = 2;
            // set status 2 LSB : 10 
            status = SET_BIT(status, 1);
            status = CLEAR_BIT(status, 0);
            interrupted = TEST_BIT(status, 5);
        } else if (scanline_counter >= mode3_bounds){
            mode = 3;
            // set status 2 LSB : 11 
            status = SET_BIT(status, 1);
            status = SET_BIT(status, 0);
        } else {
            mode = 0;
            // set status 2 LSB : 00 
            status = CLEAR_BIT(status, 1);
            status = CLEAR_BIT(status, 0);
            // check for H blank interrupt
            interrupted = TEST_BIT(status, 3);
        }
    }

    // check if H blank or V Blank interrupt
    if (interrupted && (mode != current_mode)){
       // service_interrupt(cpu_p, 1);
    }

    // when 0xFF44 == 0xFF45
    if (current_line == read_memory(cpu_p->memory_p, LYC_INDEX)){
        // bit 2 (coincidence flag) is set when condition is true 
        status = SET_BIT(status, 2);
        // if bit 6 and bit 2 are enabled then request interrupt
        if (TEST_BIT(status, 6) > 0){
           // request_interrupt(cpu_p, 1);
        }
    } else {
        status = CLEAR_BIT(status, 2);
    }

    // update LCDC status
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
    // if (TEST_BIT(lcdc, 1)){
    //     render_sprites(memory_p, lcdc);
    // }
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
    byte window_x = read_memory(memory_p, WINDOW_X_INDEX) - 7;
    
    // testing scroll Y
    //scroll_y = 0;
    printf("SCROLL Y : %u\n", scroll_y);

    bool windowed = FALSE;

    // verify if window is enabled in LCD
    if (TEST_BIT(lcdc, 5)){
        // check is current scanline is within the window Y
        if (window_y <= read_memory(memory_p, LY_INDEX)){
            windowed = TRUE;
        }
    }

    /* background tile data set selection
            bit 4 of LCD
                0 -> 8800 - 0x97FF (UNSIGNED)
                1 -> 0x8000 - 0x8FFF (SIGNED)
     */ 

    if (TEST_BIT(lcdc, 4)){
        tile_data = 0x8000;
    } else {
        tile_data = 0x8800;
        unsig = FALSE;
    }

    if (windowed == FALSE){
        /* background tile map selection
            bit 3 of LCD 
                0 -> 0x9800 - 0x9BFF
                1 -> 0x9C00 - 0x9FFF
         */
        if (TEST_BIT(lcdc, 3)){
            background_memory = 0x9C00;
        } else {
            background_memory = 0x9800;
        }
    } 
    else {
        /* window tile map selection
            bit 6 of LCD
                0 -> 0x9800 - 0x9BFF
                1 -> 0x9C00 - 0x9FFF
         */
        if (TEST_BIT(lcdc, 6)){
            background_memory = 0x9C00;
        } else {
            background_memory = 0x9800;
        }
    }

    byte y_position = 0;
    
    // y position used to calculate which 32 vertical tiles the current scanline is drawing
    if (windowed == FALSE){
        y_position = scroll_y + read_memory(memory_p, LY_INDEX);
        printf(" Y POSITION %u : ", y_position);
    } 
    else {
        y_position = read_memory(memory_p, LY_INDEX) - window_y;
    }

    // which 8 vertical pixel are currently tile is the scanline on 
    word tile_row = (((byte) (y_position / 8)) * 32);
    printf("bg_tile_row : %u \n", tile_row);

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
        printf("bg_tile_column : %u ", tile_column);
        signed_word tile_number;

        // get tile identity number based on signed or unsigned.
        // 32x32 tiles in BG. each tile can be picked based on horizontal and vertical tile
        word tile_address = background_memory + tile_row + tile_column;
        printf("tile_map_address : 0x%04X ", tile_address);
        if (unsig){
            tile_number = (byte) read_memory(memory_p, tile_address);
            printf("tile_map_number : %d ", tile_number);
        } 
        else {
            tile_number = (signed_byte) read_memory(memory_p, tile_address);
        }

        // find the tile data related to tile identity number
        word tile_location = tile_data;

        if (unsig){
            tile_location += (tile_number * TILE_SIZE);
            printf("tile_data_location : 0x%04X ", tile_location);
        } else {
            tile_location += ((tile_number + 128) * TILE_SIZE);
        }

        // find correct vertical line of the tile to get the tile data from memory
        byte line = y_position % 8;
        line *= 2; // each vertical line takes 2 bytes of memory
        byte data1 = read_memory(memory_p, tile_location + line);
        byte data2 = read_memory(memory_p, tile_location + line + 1);

        // pixel 0 in the tile is 7 of data 1 and data 2
        // invert the position of pixel
        int color_bit = x_position % 8;
        printf("color_bit %d ", color_bit);
        color_bit -= 7;
        color_bit *= -1;
        printf("flipped %d ", color_bit);

        // combine data 2 and data 1 to get color id for this pixel of the tile
        int color_number = bit_get_value(data2, color_bit);
        color_number <<= 1;
        color_number |= bit_get_value(data1, color_bit);
        printf("pixel color number %d ", color_number);

        // have to color for the bit; get the actual color from palette 0xFF47
        
        // get color 
        byte col = get_color(memory_p, color_number, BACKGROUND_PALETTE);
        printf("pixel palette color %u ", col);
        int red = 0;
        int green = 0;
        int blue = 0;

        // setup RGB values
        
        switch(col){
            case 0: red = 255; green = 255; blue = 255; break; // WHITE
            case 1: red = 0xCC; green = 0xCC; blue = 0xCC; break ; // LIGHT GRAY
            case 2:	red = 0x77; green = 0x77; blue = 0x77; break ; // DARK GRAY
        }
        
        // read current line
        int final_y = read_memory(memory_p, LY_INDEX);

        // safety check that in bound 
        if ((final_y < 0 ) || (final_y > 143) || (pixel < 0) || (pixel > 159)){
            printf("FAILED SAFETY CHECK");
            continue;
        }
        printf(" screen_data[%d][%d] ", pixel, final_y);
        screen_data[pixel][final_y][0] = red;
        screen_data[pixel][final_y][1] = green;
        screen_data[pixel][final_y][2] = blue;
        printf("\n");
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
    int bit = (data & mask) ? 1 : 0;
	return bit;
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
                    case 0:red =255; green=255; blue=255; break; // WHITE
                    // case 1:red =0xCC; green=0xCC; blue=0xCC; break; // LIGHT_GRAY
                    // case 2:red=0x77; green=0x77; blue=0x77; break; // DARK GRAY
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

void setup_gl_context(){
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1.0, 1.0);
    glClearColor(0, 0, 0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glShadeModel(GL_FLAT);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
}

void render_screen(){
 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 	glLoadIdentity();
 	glRasterPos2i(-1, 1);
	glPixelZoom(1, -1);
 	glDrawPixels(160, 144, GL_RGB, GL_UNSIGNED_BYTE, screen_data);
	SDL_GL_SwapWindow(sdl_window);
}

void initialize_sdl_window(){
	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ){
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
	}
	else{
        //Create window
        sdl_window = SDL_CreateWindow( "matchaGb", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
        if( sdl_window == NULL )
        {
            printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() ); 
        }
	}
}

void initialize_gl_context(){
    gl_context = SDL_GL_CreateContext(sdl_window);
    if (gl_context == NULL){
        printf("GL context could not be created!");
    } else {
        setup_gl_context();
    }
}

void initialize_screen_data(){
    for (int x = 0; x < SCREEN_WIDTH; x++){
        for (int y = 0; y < SCREEN_HEIGHT; y++){
            screen_data[x][y][0] = 255;
            screen_data[x][y][1] = 255;
            screen_data[x][y][2] = 255;
        }
    }
}

void print_cpu_content(cpu *cpu_p){

    printf("\nPC -- PC:0x%04X\n", cpu_p->PC);

    printf("AF -- A:0x%02X F:0x%02X \n", cpu_p->AF.hi, cpu_p->AF.lo);
    printf("BC -- B:0x%02X C:0x%02X \n", cpu_p->BC.hi, cpu_p->BC.lo);
    printf("DE -- D:0x%02X E:0x%02X \n", cpu_p->DE.hi, cpu_p->DE.lo);
    printf("HL -- H:0x%02X L:0x%02X \n", cpu_p->HL.hi, cpu_p->HL.lo);
    printf("SP -- S:0x%02X P:0x%02X \n", cpu_p->SP.hi, cpu_p->SP.lo);

    printf("halted -- :%u\n", cpu_p->halted);
    printf("pending_interrupt_enable -- :%u\n", cpu_p->pending_interrupt_enable);
    printf("interrupt_request -- :%u\n", cpu_p->PC);
    printf("\n-------------------------------------\n");
}
