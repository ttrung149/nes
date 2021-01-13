#include "bus.h"

Bus::Bus() : cpu(nullptr), ppu(nullptr), clock_cycles(0) {
    for (auto &byte : cpu_ram) byte = 0x00;

    // Reset controller states
    controller_states[0] = 0x00;
    controller_states[1] = 0x00;
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

    // Write to main bus RAM
    // The 2kB actual memory is mirrored to represent 8kB range
    if (addr >= SYSTEM_RAM_ADDR_LOWER && addr <= SYSTEM_RAM_ADDR_UPPER) {
	    cpu_ram[addr & 0x07FF] = data;
    }

    // Write to PPU address range
    else if (addr >= SYSTEM_PPU_ADDR_LOWER && addr <= SYSTEM_PPU_ADDR_UPPER) {
        assert(ppu);
        ppu->write_to_main_bus(addr & 0x0007, data);
    }

    // Kicks off DMA to OAM memory
    else if (addr == OAM_ADDR) {
        dma_transfer = true;
        dma_page = data;
        dma_addr = 0x00;
    }

    // Write to controller address range
    else if (addr >= CONTROLLER_ADDR_LOWER && addr <= CONTROLLER_ADDR_UPPER) {
        controller_states[addr & 0x0001] = controller[addr & 0x0001];
    }
}

uint8_t Bus::read(uint16_t addr, bool read_only) {
    uint8_t data = cartridge->handle_cpu_read(addr);

    // Read from main bus RAM
    // The 2kB actual memory is mirrored to represent 8kB range
    if (addr >= SYSTEM_RAM_ADDR_LOWER && addr <= SYSTEM_RAM_ADDR_UPPER) {
	    data = cpu_ram[addr & 0x07FF];
    }

    // Read from PPU address range
    else if (addr >= SYSTEM_PPU_ADDR_LOWER && addr <= SYSTEM_PPU_ADDR_UPPER) {
        assert(ppu);
        data = ppu->read_from_main_bus(addr & 0x0007, read_only);
    }

    // Read from controller address range
    else if (addr >= CONTROLLER_ADDR_LOWER && addr <= CONTROLLER_ADDR_UPPER) {
        data = (controller_states[addr & 0x0001] & 0x80) > 0;
        controller_states[addr & 0x0001] <<= 1;
    }
    return data;
}

void Bus::clock() {
    assert(ppu); ppu->clock();
    assert(cpu);

    if (clock_cycles % 3 == 0) {
        // Stops bus clock to handle DMA transfer to OAM memory
        if (dma_transfer) {
            if (dma_idle) {
                if (clock_cycles % 2 == 1) dma_idle = false;
            }
            else {
                if (clock_cycles % 2 == 0) {
                    dma_data = read(dma_page << 8 | dma_addr, false);
                }
                else {
                    ppu->oam_ptr[dma_addr] = dma_data;
                    dma_addr++;

                    // DMA is done when 256 bytes have been written or when
                    // dma_addr rolls back to 0
                    if (dma_addr == 0x00) {
                        dma_transfer = false;
                        dma_idle = true;
                    }
                }
            }
        }

        // Non-DMA process
        else {
            cpu->clock();
        }
    }

    if (ppu->nmi()) { cpu->nmi(); ppu->reset_nmi(); }
    clock_cycles++;
}

void Bus::reset() {
    assert(cpu != nullptr); cpu->reset();
    assert(ppu != nullptr); ppu->reset();
    clock_cycles = 0;

    // Reset OAM DMA
    dma_page = 0x00;
	dma_addr = 0x00;
	dma_data = 0x00;
	dma_idle = true;
	dma_transfer = false;
}
