#ifndef BUS_H_
#define BUS_H_

#include "mem.h"
#include "cpu.h"
#include "ppu.h"
#include "cartridge.h"

class Bus {
public:
    Bus();
    ~Bus();

    void connect_to_cpu(cpu6502 *_cpu);
    cpu6502 *cpu;

    void connect_to_ppu(ppu2C02 *_ppu);
    ppu2C02 *ppu;

public:
    void connect_to_cartridge(const std::shared_ptr<Cartridge>& _cartridge);
    std::shared_ptr<Cartridge> cartridge;

public:
    void clock();
    void reset();

    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr, bool read_only = false);

public:
    uint8_t cpu_ram[_2_KB];
    uint8_t controller[2];

private:
    uint32_t clock_cycles;
    uint8_t controller_states[2];

// OAM DMA
private:
    uint8_t dma_page = 0x00;
	uint8_t dma_addr = 0x00;
	uint8_t dma_data = 0x00;

    bool dma_idle = true;       // Delay CPU clock cycle to sync up DMA process
    bool dma_transfer = false;  // Indicates whether DMA transfer is happening
};

#endif
