#include "emulator.h"

int main(void) {
    Emulator nes(Emulator::DEBUG_MODE);

    nes.begin();

    return EXIT_SUCCESS;
}