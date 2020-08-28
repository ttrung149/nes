#ifndef MAPPER_0_H_
#define MAPPER_0_H_

#include "base_mapper.h"

class Mapper_0 : public Mapper {
public:
    Mapper_0(uint8_t num_prg_banks, uint8_t num_chr_banks);
    ~Mapper_0();

public:
    bool get_cpu_read_mapped_addr(uint16_t addr, uint16_t  &mapped_addr) override;
    bool get_cpu_write_mapped_addr(uint16_t addr, uint16_t &mapped_addr) override;

    bool get_ppu_read_mapped_addr(uint16_t addr, uint16_t &mapped_addr) override;
    bool get_ppu_write_mapped_addr(uint16_t addr, uint16_t &mapped_addr) override;
};

#endif
