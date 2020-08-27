#include "ppu.h"

ppu2C02::ppu2C02() : scan_line(0), cycle(0), frame_completed(false) {}

ppu2C02::~ppu2C02() {}

void ppu2C02::connect_to_cartridge(const std::shared_ptr<Cartridge>& _cartridge) {
    cartridge = _cartridge;
}

/* PPU clock ticks */
void ppu2C02::clock() {
    cycle++;
    if (cycle >= 341) {
        cycle = 0; scan_line++;
        if (scan_line >= 261) { scan_line = -1; frame_completed = true; }
	}
}

/* Main bus read */
uint8_t ppu2C02::read_from_main_bus(uint16_t addr, bool read_only) {
    (void) read_only;

    uint8_t data = 0x00;

    assert(addr >= 0x0000 && addr <= 0x0007);
    switch (addr) {
        case 0x0000: // Control
            break;
        case 0x0001: // Mask
            break;
        case 0x0002: // Status
            break;
        case 0x0003: // OAM Address
            break;
        case 0x0004: // OAM Data
            break;
        case 0x0005: // Scroll
            break;
        case 0x0006: // PPU Address
            break;
        case 0x0007: // PPU Data
            break;
    }
    return data;
}

/* Main bus write */
void ppu2C02::write_to_main_bus(uint16_t addr, uint8_t data) {
    (void) data;
    assert(addr >= 0x0000 && addr <= 0x0007);
    switch (addr) {
        case 0x0000: // Control
            break;
        case 0x0001: // Mask
            break;
        case 0x0002: // Status
            break;
        case 0x0003: // OAM Address
            break;
        case 0x0004: // OAM Data
            break;
        case 0x0005: // Scroll
            break;
        case 0x0006: // PPU Address
            break;
        case 0x0007: // PPU Data
            break;
    }
}

/* PPU Bus read */
uint8_t ppu2C02::read_from_ppu_bus(uint16_t addr, bool read_only) {
    (void) addr;
    (void) read_only;

    return 0;
}

/* PPU Bus write */
void ppu2C02::write_to_ppu_bus(uint16_t addr, uint8_t data) {
    (void) addr;
    (void) data;
}
