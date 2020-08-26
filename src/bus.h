#ifndef BUS_H_
#define BUS_H_

#include "cpu.h"

class Bus {
private:
    uint32_t clock_cycles;

public:
    Bus();
    ~Bus();
    void connect_to_cpu(cpu6502 *_cpu);

    cpu6502 *cpu;

public:
    void clock();
    void reset();

    void write(uint16_t addr, uint8_t data);
	uint8_t read(uint16_t addr, bool read_only = false);

public:
    uint8_t cpu_ram[65536];

};

#endif
