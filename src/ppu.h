#ifndef PPU_H_
#define PPU_H_

#include <memory>
#include "mem.h"
#include "cartridge.h"

class ppu2C02 {
public:
    ppu2C02();
    ~ppu2C02();

    void clock();
    void connect_to_cartridge(const std::shared_ptr<Cartridge>& _cartridge);

private:
    std::shared_ptr<Cartridge> cartridge;

private:
    // uint8_t ppu_name_table[2][_1_KB];
    // uint8_t ppu_palette_table[32];

    int16_t scan_line;
    int16_t cycle;
    bool frame_completed;

public:
    uint8_t read_from_main_bus(uint16_t addr, bool read_only);
    void write_to_main_bus(uint16_t addr, uint8_t data);

    uint8_t read_from_ppu_bus(uint16_t addr, bool read_only);
    void write_to_ppu_bus(uint16_t addr, uint8_t data);
};

#endif
