#include <iostream>
#include "mem.h"
#include "cartridge.h"

Cartridge::Cartridge(const char *nes_file_name) :
    mapper_id(0), num_prg_banks(0), num_chr_banks(0)
{
    struct __attribute__((__packed__)) {
        char name[4];
        uint8_t num_prg_banks;
        uint8_t num_chr_banks;
        uint8_t mapper_1;
        uint8_t mapper_2;
        uint8_t prg_ram_size;
        uint8_t tv_system_1;
        uint8_t tv_system_2;
        uint8_t unused[5];
    } header;

    assert(nes_file_name);
    std::ifstream ifs;
    ifs.open(nes_file_name, std::ifstream::binary);

    if (ifs.is_open()) {
        ifs.read((char*) &header, sizeof(header));

        if (header.mapper_1 & 0x04)
            ifs.seekg(_HALF_KB, std::ios_base::cur);

        mirror = (header.mapper_1 & 0x01) ? VERTICAL : HORIZONTAL;

        // Hard coded for now
        uint8_t file_format_type = 1;

        switch (file_format_type) {
            case 0: break;
            case 1: {
                num_prg_banks = header.num_prg_banks;
                prg_memory_rom.resize(num_prg_banks * _16_KB);
                ifs.read((char *)prg_memory_rom.data(), prg_memory_rom.size());

                num_chr_banks = (header.num_chr_banks == 0) ? 1 : header.num_chr_banks;
                chr_memory_rom.resize(num_chr_banks * _8_KB);
                ifs.read((char *)chr_memory_rom.data(), chr_memory_rom.size());

                break;
            }
            case 2: break;
            default: break;
        }

        mapper_id = ((header.mapper_2 >> 4) << 4) | (header.mapper_1 >> 4);
        switch (mapper_id) {
            case 0: {
                mapper_ptr = std::make_shared<Mapper_0>(num_prg_banks, num_chr_banks);
                break;
            }
            default:
                std::cerr << "ERR: Mapper '" << (int)mapper_id << "' not supported!\n";
                exit(EXIT_FAILURE);
        }

        ifs.close();
    }
    else {
        std::cerr << "ERR: Cannot open file '" << nes_file_name << "'\n";
        exit(EXIT_FAILURE);
    }
}

Cartridge::~Cartridge() {}

uint8_t Cartridge::handle_cpu_read(uint16_t addr) {
    uint16_t mapped_addr = 0;
    if (mapper_ptr->get_cpu_read_mapped_addr(addr, mapped_addr)) {
        return prg_memory_rom[mapped_addr];
    }
    return 0;
}

void Cartridge::handle_cpu_write(uint16_t addr, uint8_t data) {
    uint16_t mapped_addr = 0;
    if (mapper_ptr->get_cpu_write_mapped_addr(addr, mapped_addr)) {
        prg_memory_rom[mapped_addr] = data;
    }
}

uint8_t Cartridge::handle_ppu_read(uint16_t addr) {
    uint16_t mapped_addr = 0;
    if (mapper_ptr->get_ppu_read_mapped_addr(addr, mapped_addr)) {
        return chr_memory_rom[mapped_addr];
    }
    return 0;
}

void Cartridge::handle_ppu_write(uint16_t addr, uint8_t data) {
    uint16_t mapped_addr = 0;
    if (mapper_ptr->get_ppu_write_mapped_addr(addr, mapped_addr)) {
        chr_memory_rom[mapped_addr] = data;
    }
}
