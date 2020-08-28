#include "mapper_0.h"

Mapper_0::Mapper_0(uint8_t num_prg_banks, uint8_t num_chr_banks) : 
    Mapper(num_prg_banks, num_chr_banks) {}

Mapper_0::~Mapper_0() {}

bool Mapper_0::cpu_mapped_read(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        mapped_addr = addr & (num_prg_banks> 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

bool Mapper_0::cpu_mapped_write(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        mapped_addr = addr & (num_prg_banks> 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

bool Mapper_0::ppu_mapped_read(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        mapped_addr = addr;
        return true;
    }
    return false;
}

bool Mapper_0::ppu_mapped_write(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        if (num_chr_banks == 0) { mapped_addr = addr; return true; }
	}
    return false;
};
