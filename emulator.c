#include "environment.h"

#include "cartridge.h"
#include "memory.h"
int main(void){

    cartridge *cartridge_p = initialize_cartridge("Tetris.gb");

    memory_map *memory_map = initialize_memory(cartridge_p);

    free(cartridge_p);
    cartridge_p = NULL;
    
    return 0;
}