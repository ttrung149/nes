#include <inttypes.h>
#include <vector>
#include <map>
#include <string>

#ifndef CPU_6502_
#define CPU_6502_

// Forward-declaration for class 'Bus' defined in 'bus.cpp'
class Bus;

class cpu6502 {
public:
    cpu6502();
    ~cpu6502();

// Bus connection
private:
    Bus *bus;

public:
    void connect_to_bus(Bus *b);
    uint8_t read_from_bus(uint16_t addr);
	void write_to_bus(uint16_t addr, uint8_t data);

// 6502 CPU's internal state attributes
public:
    enum flags : uint8_t {
		C = (0x1 << 0),	                // Carry bit
		Z = (0x1 << 1),	                // Zero
		I = (0x1 << 2),	                // Disable interrupts
		D = (0x1 << 3),	                // Decimal mode
		B = (0x1 << 4),	                // Break
		U = (0x1 << 5),	                // Unused
		V = (0x1 << 6),	                // Overflow
		N = (0x1 << 7)	                // Negative
	};

    uint8_t  a;                         // Accumulator register
	uint8_t  x;                         // X register
	uint8_t  y;                         // Y register
	uint8_t  stkp;                      // Stack pointer 
	uint16_t pc;                        // Program counter
	uint8_t  status;                    // Status register

// 6502 CPU's internal state helper attributes
private:
    uint8_t  _fetched; 
	uint16_t _temp;
	uint16_t _addr_abs;
	uint16_t _addr_rel;
	uint8_t  _opcode;
	uint8_t  _remaining_cycles;
	uint32_t _clock_count;

    uint8_t fetch();

// 6502 CPU internal state methods 
public:
    void clock();
    void reset();
    void irq();
    void nmi();

// Flags getters and setters
private:
    uint8_t get_flag(flags f);
	void    set_flag(flags f, bool v);

// Addressing modes for 6502 instructions 
private: 
	uint8_t IMP();	uint8_t IMM();	
	uint8_t ZP0();	uint8_t ZPX();	
	uint8_t ZPY();	uint8_t REL();
	uint8_t ABS();	uint8_t ABX();	
	uint8_t ABY();	uint8_t IND();	
	uint8_t IZX();	uint8_t IZY();

// Opcodes for 6502 instructions 
private:
    struct Instruction {
		std::string mnemonic;
		uint8_t (cpu6502::*operate  ) (void);
		uint8_t (cpu6502::*addr_mode) (void);
		uint8_t cycles;
	};

    // Instructions lookup table
    static const std::vector<Instruction> instructions_table;

    // Helper functions: update 6502's registers state from instructions  
	uint8_t ADC();	uint8_t AND();	uint8_t ASL();	uint8_t BCC();
	uint8_t BCS();	uint8_t BEQ();	uint8_t BIT();	uint8_t BMI();
	uint8_t BNE();	uint8_t BPL();	uint8_t BRK();	uint8_t BVC();
	uint8_t BVS();	uint8_t CLC();	uint8_t CLD();	uint8_t CLI();
	uint8_t CLV();	uint8_t CMP();	uint8_t CPX();	uint8_t CPY();
	uint8_t DEC();	uint8_t DEX();	uint8_t DEY();	uint8_t EOR();
	uint8_t INC();	uint8_t INX();	uint8_t INY();	uint8_t JMP();
	uint8_t JSR();	uint8_t LDA();	uint8_t LDX();	uint8_t LDY();
	uint8_t LSR();	uint8_t NOP();	uint8_t ORA();	uint8_t PHA();
	uint8_t PHP();	uint8_t PLA();	uint8_t PLP();	uint8_t ROL();
	uint8_t ROR();	uint8_t RTI();	uint8_t RTS();	uint8_t SBC();
	uint8_t SEC();	uint8_t SED();	uint8_t SEI();	uint8_t STA();
	uint8_t STX();	uint8_t STY();	uint8_t TAX();	uint8_t TAY();
	uint8_t TSX();	uint8_t TXA();	uint8_t TXS();	uint8_t TYA();

	uint8_t XXX(); // no-op

public:
    static std::string hex_str(uint32_t num, uint8_t num_half_bytes);
    std::map<uint16_t, std::string> disasm(uint16_t begin, uint16_t end);

    bool instr_completed();
};

#endif
