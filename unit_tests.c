#include "minunit.h"
#include "cartridge.h"
#include "environment.h"

static cartridge *cartridge_p = NULL;
static char *file_name = "Tetris.gb";

void test_setup(void){
    cartridge_p = initialize_cartridge(file_name); 
}

void test_teardown(void){
    free(cartridge_p);
    cartridge_p = NULL;
}

MU_TEST(test_initialize_cartridge_tetris){

    mu_assert_string_eq((char *) cartridge_p->game_title, "TETRIS");
    mu_check(cartridge_p->cartridge_type == GAMEBOY);
    mu_check(cartridge_p->gameboy_indicator == 0);
    mu_check(cartridge_p->rom_banks * 16 == 32);
    mu_check(cartridge_p->ram_banks == 0);
    mu_check(cartridge_p->gameboy_type == 0);

}

MU_TEST_SUITE(test_suite){
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);
    MU_RUN_TEST(test_initialize_cartridge_tetris);
}

int main (int argc, char *argv[]){
    printf("START TEST");
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return 0;
}