#ifndef CARTRIDGE_H_
#define CARTRIDGE_H_

#include <vector>
#include <fstream>
#include "mapper_0.h"

class Cartridge {
public:	
    Cartridge(const std::string &nes_file_name);
    ~Cartridge();

    enum MIRROR { HORIZONTAL, VERTICAL, ONESCREEN_LO, ONESCREEN_HI } mirror;

private:
    uint8_t mapper_id;
    uint8_t num_prg_banks;
    uint8_t num_chr_banks;

    std::shared_ptr<Mapper> mapper_ptr;
    std::vector<uint8_t> prg_memory_rom;
    std::vector<uint8_t> chr_memory_rom;

public:
    uint8_t read_from_main_bus(uint16_t addr);
    void write_to_main_bus(uint16_t addr, uint8_t data);

    uint8_t read_from_ppu_bus(uint16_t addr);
    void write_to_ppu_bus(uint16_t addr, uint8_t data);
};

#endif
