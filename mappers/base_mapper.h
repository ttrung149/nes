#ifndef MAPPER_H_
#define MAPPER_H_

#include <inttypes.h>

class Mapper {
public:
    Mapper(uint8_t num_prg_banks, uint8_t num_chr_banks);
    virtual ~Mapper() = 0;

protected:
    uint8_t num_prg_banks;
    uint8_t num_chr_banks;

public:
    // Transform CPU bus address into PRG ROM offset
    virtual bool get_cpu_read_mapped_addr(uint16_t addr, uint16_t &mapped_addr) = 0;
    virtual bool get_cpu_write_mapped_addr(uint16_t addr, uint16_t &mapped_addr) = 0;

    // Transform PPU bus address into CHR ROM offset
    virtual bool get_ppu_read_mapped_addr(uint16_t addr, uint16_t &mapped_addr) = 0;
    virtual bool get_ppu_write_mapped_addr(uint16_t addr, uint16_t &mapped_addr) = 0;
};

#endif
