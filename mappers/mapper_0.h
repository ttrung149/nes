#ifndef MAPPER_0_H_
#define MAPPER_0_H_

#include "base_mapper.h"

class Mapper_0 : public Mapper {
public:
    Mapper_0(uint8_t num_prg_banks, uint8_t num_chr_banks);
    ~Mapper_0();

public:
    bool cpu_mapped_read(uint16_t addr, uint32_t &mapped_addr) override;
    bool cpu_mapped_write(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppu_mapped_read(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppu_mapped_write(uint16_t addr, uint32_t &mapped_addr) override;
};

#endif
