#include "bus.h"

Bus::Bus() : clock_cycles(0), cpu(nullptr), ppu(nullptr) {
    for (auto &byte : cpu_ram) byte = 0x00;
}

Bus::~Bus() {}

void Bus::connect_to_cpu(cpu6502 *_cpu) { assert(_cpu); cpu = _cpu; }

void Bus::connect_to_ppu(ppu2C02 *_ppu) { assert(_ppu); ppu = _ppu; }

void Bus::connect_to_cartridge(const std::shared_ptr<Cartridge>& _cartridge) {
    cartridge = _cartridge;
    assert(ppu);
    ppu->connect_to_cartridge(_cartridge);
}

void Bus::write(uint16_t addr, uint8_t data) {
    cartridge->handle_cpu_write(addr, data);

    // Write to system RAM
    // The 2kB actual memory is mirrored to represent 8kB range
    if (addr >= SYSTEM_RAM_ADDR_LOWER && addr <= SYSTEM_RAM_ADDR_UPPER) {
	    cpu_ram[addr & 0x07FF] = data;
    }

    // Write to PPU
    else if (addr >= PPU_ADDR_LOWER && addr <= PPU_ADDR_UPPER) {
        assert(ppu);
        ppu->write_to_main_bus(addr & 0x0007, data);
    }
}

uint8_t Bus::read(uint16_t addr, bool read_only) {
    uint8_t data = cartridge->handle_cpu_read(addr);

    // Read from system RAM
    // The 2kB actual memory is mirrored to represent 8kB range
    if (addr >= SYSTEM_RAM_ADDR_LOWER && addr <= SYSTEM_RAM_ADDR_UPPER) {
	    data = cpu_ram[addr & 0x07FF];
    }

    // Read from PPU
    else if (addr >= PPU_ADDR_LOWER && addr <= PPU_ADDR_UPPER) {
        assert(ppu);
        data = ppu->read_from_main_bus(addr & 0x0007, read_only);
    }

	return data;
}

void Bus::clock() {
    assert(ppu); ppu->clock();
    assert(cpu); if (clock_cycles % 3 == 0) cpu->clock();
    clock_cycles++;
}

void Bus::reset() {
    assert(cpu != nullptr); cpu->reset();
    clock_cycles = 0;
}
