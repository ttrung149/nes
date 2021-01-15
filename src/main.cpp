#include <iostream>
#include <cstring>
#include "emulator.h"

void display_help() {
    std::cout << "\n*=================================================="
              << "\n*  Another NES emulator"
              << "\n*  Copyright (c) 2020-2021 - Trung Truong"
              << "\n*==================================================";
    std::cout << "\n> Run \"./nes <filename.nes> ..\" to start NES game"
              << "\n> Optional flags:"
              << "\n>   --debug | -D : Show debug window"
              << "\n>   --help  | -H : Display this help message\n\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        display_help();
        return EXIT_FAILURE;
    }
    else if (argc == 2) {
        Emulator nes(Emulator::NORMAL_MODE, argv[1]);
        nes.begin();
        return EXIT_SUCCESS;
    }
    else {
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-H") == 0) {
                display_help();
                return EXIT_SUCCESS;
            }
            else if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-D") == 0) {
                Emulator nes(Emulator::DEBUG_MODE, argv[1]);
                nes.begin();
                return EXIT_SUCCESS;
            }
        }
    }
}
