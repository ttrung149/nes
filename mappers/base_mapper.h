#ifndef MAPPER_H_
#define MAPPER_H_

#include <inttypes.h>

class Mapper {
public:
    Mapper(uint8_t num_prg_banks, uint8_t num_chr_banks);
    ~Mapper();

protected:
    uint8_t num_prg_banks;
    uint8_t num_chr_banks;

public:
    // Transform CPU bus address into PRG ROM offset
    virtual bool cpu_mapped_read(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool cpu_mapped_write(uint16_t addr, uint32_t &mapped_addr) = 0;

    // Transform PPU bus address into CHR ROM offset
    virtual bool ppu_mapped_read(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool ppu_mapped_write(uint16_t addr, uint32_t &mapped_addr) = 0;
};

#endif
