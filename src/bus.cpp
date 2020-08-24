#include "mem.h"
#include "bus.h"

Bus::Bus() : clock_cycles(0), cpu(nullptr) {
    for (auto &byte : cpu_ram) byte = 0x00;
}

Bus::~Bus() {}

void Bus::connect_to_cpu(cpu6502 *_cpu) { assert(_cpu != nullptr); cpu = _cpu; }

void Bus::write(uint16_t addr, uint8_t data) {
    if (addr >= 0x0000 && addr <= 0xFFFF)
        cpu_ram[addr] = data;
}

uint8_t Bus::read(uint16_t addr, bool read_only) {
    uint8_t data = 0x00;
    (void) read_only;

    if (addr >= 0x0000 && addr <= 0xFFFF)
        data = cpu_ram[addr];

	return data;
}

void Bus::clock() {
    assert(cpu != nullptr);

    if (clock_cycles % 3 == 0) cpu->clock();
    clock_cycles++;
}

void Bus::reset() {
    assert(cpu != nullptr);
    
    cpu->reset();
    clock_cycles = 0;
}
