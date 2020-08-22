#include "mem.h"
#include "bus.h"
#include "cpu.h"

cpu6502::cpu6502() :
    bus(nullptr), a(0x00), x(0x00), y(0x00), stkp(0x00), pc(0x0000), status(0x00),
    _fetched(0), _temp(0), _addr_abs(0), _addr_rel(0), _opcode(0), _remaining_cycles(0),
    _clock_count(0) {}

cpu6502::~cpu6502() {}

/*=============================================================================
 * MAIN BUS CONNECTION
 *===========================================================================*/
void cpu6502::connect_to_bus(Bus *b) {
    bus = b;
}
uint8_t cpu6502::read_from_bus(uint16_t addr) {
    assert(bus != nullptr);
    return bus->read(addr, false);
}
void cpu6502::write_to_bus(uint16_t addr, uint8_t data) {
    assert(bus != nullptr);
    bus->write(addr, data);
}

/*=============================================================================
 * 6502 CPU INTERNAL STATE METHODS
 *===========================================================================*/
// Flags getters
uint8_t cpu6502::get_flag(flags f) {
    return ((status & f) > 0) ? 0x01 : 0x00;
}

// Flags setters
void cpu6502::set_flag(flags f, bool v) {
    if (v) status |= f; else status &= ~f;
}

/* Emulates one CPU clock cycle */
void cpu6502::clock() {
    if (_remaining_cycles == 0) {
        // Get opcode for next instruction
        _opcode = read_from_bus(pc);
        set_flag(U, true);

        // Increment program counter
        pc++;
        _remaining_cycles = instructions_table[_opcode].cycles;

        // Fetches data with proper addressing mode and executes instruction
        uint8_t add_cycle_1 = (this->*instructions_table[_opcode].addr_mode)();
        uint8_t add_cycle_2 = (this->*instructions_table[_opcode].operate)();

        // Add additional clock cycles (if exists)
        _remaining_cycles += (add_cycle_1 & add_cycle_2);
        set_flag(U, true);
    }

    _clock_count++;
    _remaining_cycles--;
}

/* Sets CPU back to known state after reset */
void cpu6502::reset() {
    _addr_abs = RESET_PC;
    uint16_t lo = read_from_bus(_addr_abs);
    uint16_t hi = read_from_bus(_addr_abs + 1);
    pc = (hi << 8) | lo;

    a = 0x00; x = 0x00; y = 0x00;
    stkp = RESET_STKP; status = 0x00 | U;

    _addr_rel = 0x0000; _addr_abs = 0x0000; _fetched = 0x00;
    _remaining_cycles = 8;
}

/* Interrupt request */
void cpu6502::irq() {
    if (get_flag(I) == 0) {
        // Push program counter onto the stack
        write_to_bus(BASE_STKP + stkp, (pc >> 8) & 0x00FF);
        stkp--;
        write_to_bus(BASE_STKP + stkp, pc & 0x00FF);
        stkp--;

        // Push status register onto the stack
        set_flag(U, true);
        set_flag(I, true);
        set_flag(B, false);

        write_to_bus(BASE_STKP + stkp, status);
        stkp--;

        // Read new program counter at (pre-programmed) IRQ_PC
        _addr_abs = IRQ_PC;
        uint16_t lo = read_from_bus(_addr_abs);
        uint16_t hi = read_from_bus(_addr_abs + 1);
        pc = (hi << 8) | lo;

        _remaining_cycles = 7;
    }
}

/* Non-maskable interrupt request */
void cpu6502::nmi() {
    // Push program counter onto the stack
    write_to_bus(BASE_STKP + stkp, (pc >> 8) & 0x00FF);
    stkp--;
    write_to_bus(BASE_STKP + stkp, pc & 0x00FF);
    stkp--;

    // Push status register onto the stack
    set_flag(U, true);
    set_flag(I, true);
    set_flag(B, false);

    write_to_bus(BASE_STKP + stkp, status);
    stkp--;

    // Read new program counter at (pre-programmed) NMI_PC
    _addr_abs = NMI_PC;
    uint16_t lo = read_from_bus(_addr_abs);
    uint16_t hi = read_from_bus(_addr_abs + 1);
    pc = (hi << 8) | lo;

    _remaining_cycles = 8;
}

/* Populates the '_fetched' attribute */
uint8_t cpu6502::fetch() {
    if (!(instructions_table[_opcode].addr_mode == &cpu6502::IMP)) {
        _fetched = read_from_bus(_addr_abs);
    }
    return _fetched;
}

/*=============================================================================
 * ADDRESSING MODES
 *===========================================================================*/
uint8_t cpu6502::IMP() {
    _fetched = a;
    return 0;
}

uint8_t cpu6502::IMM() {
    _addr_abs = pc++;
    return 0;
}

uint8_t cpu6502::ZP0() {
    _addr_abs = read_from_bus(pc);
    pc++;
    _addr_abs &= 0x00FF;
    return 0;
}

uint8_t cpu6502::ZPX() {
    _addr_abs = read_from_bus(pc) + x;
    pc++;
    _addr_abs &= 0x00FF;
    return 0;
}

uint8_t cpu6502::ZPY() {
    _addr_abs = read_from_bus(pc) + y;
    pc++;
    _addr_abs &= 0x00FF;
    return 0;
}

uint8_t cpu6502::REL() {
    _addr_rel = read_from_bus(pc);
    pc++;

    // Handles negative _addr_rel case
    if (_addr_rel & 0x80) _addr_rel |= 0xFF00;
    return 0;
}

uint8_t cpu6502::ABS() {
    uint16_t lo = read_from_bus(pc);
    pc++;
    uint16_t hi = read_from_bus(pc);
    pc++;

    _addr_abs = (hi << 8) | lo;
    return 0;
}

uint8_t cpu6502::ABX() {
    uint16_t lo = read_from_bus(pc);
    pc++;
    uint16_t hi = read_from_bus(pc);
    pc++;

    _addr_abs = (hi << 8) | lo;
    _addr_abs += x;

    // Additional clock cycles if absolute address crosses pages
    return ((_addr_abs & 0xFF00) != (hi << 8)) ? 1 : 0;
}

uint8_t cpu6502::ABY() {
    uint16_t lo = read_from_bus(pc);
    pc++;
    uint16_t hi = read_from_bus(pc);
    pc++;

    _addr_abs = (hi << 8) | lo;
    _addr_abs += y;

    // Additional clock cycles if absolute address crosses pages
    return ((_addr_abs & 0xFF00) != (hi << 8)) ? 1 : 0;
}

uint8_t cpu6502::IND() {
    uint16_t lo = read_from_bus(pc);
    pc++;
    uint16_t hi = read_from_bus(pc);
    pc++;

    uint16_t ptr = (hi << 8) | lo;
    if (lo == 0x00FF)
        _addr_abs = (read_from_bus(ptr & 0xFF00) << 8) | read_from_bus(ptr);
    else
        _addr_abs = (read_from_bus(ptr + 1) << 8) | read_from_bus(ptr);

    return 0;
}

uint8_t cpu6502::IZX() {
    uint16_t t = read_from_bus(pc);
    pc++;

    uint16_t lo = read_from_bus((uint16_t)(t + (uint16_t)x) & 0x00FF);
    uint16_t hi = read_from_bus((uint16_t)(t + (uint16_t)x + 1) & 0x00FF);

    _addr_abs = (hi << 8) | lo;

    return 0;
}

uint8_t cpu6502::IZY() {
    uint16_t t = read_from_bus(pc);
    pc++;

    uint16_t lo = read_from_bus(t & 0x00FF);
    uint16_t hi = read_from_bus((t + 1) & 0x00FF);

    _addr_abs = ((hi << 8) | lo) + y;

    // Additional clock cycles if absolute address crosses pages
    return ((_addr_abs & 0xFF00) != (hi << 8)) ? 1 : 0;
}

/*=============================================================================
 * INSTRUCTIONS
 *===========================================================================*/
uint8_t cpu6502::ADC() { return 0; }

uint8_t cpu6502::AND() {
    fetch();
    a = a & _fetched;
    set_flag(Z, a == 0x00);
    set_flag(N, a & 0x80);
    return 1;
}

uint8_t cpu6502::ASL() {
    fetch();
    _temp = (uint16_t)_fetched << 1;
    set_flag(C, (_temp & 0xFF00) > 0);
    set_flag(Z, (_temp & 0x00FF) == 0x00);
    set_flag(N, _temp & 0x80);

    if (instructions_table[_opcode].addr_mode == &cpu6502::IMP)
        a = _temp & 0x00FF;
    else
        write_to_bus(_addr_abs, _temp & 0x00FF);
    return 0;
}

uint8_t cpu6502::BCC() {
    if (get_flag(C) == 0) {
        _remaining_cycles++;
        _addr_abs = pc + _addr_rel;

        if((_addr_abs & 0xFF00) != (pc & 0xFF00))
            _remaining_cycles++;

        pc = _addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BCS() {
    if (get_flag(C) == 1) {
        _remaining_cycles++;
        _addr_abs = pc + _addr_rel;

        if ((_addr_abs & 0xFF00) != (pc & 0xFF00))
            _remaining_cycles++;

        pc = _addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BEQ() {
    if (get_flag(Z) == 1) {
        _remaining_cycles++;
        _addr_abs = pc + _addr_rel;

        if ((_addr_abs & 0xFF00) != (pc & 0xFF00))
            _remaining_cycles++;

        pc = _addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BIT() {
    fetch();
    _temp = a & _fetched;
    set_flag(Z, (_temp & 0x00FF) == 0x00);
    set_flag(N, _fetched & (0x1 << 7));
    set_flag(V, _fetched & (0x1 << 6));
    return 0;
}

uint8_t cpu6502::BMI() {
    if (get_flag(N) == 1) {
        _remaining_cycles++;
        _addr_abs = pc + _addr_rel;

        if ((_addr_abs & 0xFF00) != (pc & 0xFF00))
            _remaining_cycles++;

        pc = _addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BNE() {
    if (get_flag(Z) == 0) {
        _remaining_cycles++;
        _addr_abs = pc + _addr_rel;

        if ((_addr_abs & 0xFF00) != (pc & 0xFF00))
            _remaining_cycles++;

        pc = _addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BPL() {
    if (get_flag(N) == 0) {
        _remaining_cycles++;
        _addr_abs = pc + _addr_rel;

        if ((_addr_abs & 0xFF00) != (pc & 0xFF00))
            _remaining_cycles++;

        pc = _addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BRK() { return 0; }

uint8_t cpu6502::BVC() {
    if (get_flag(V) == 0) {
        _remaining_cycles++;
        _addr_abs = pc + _addr_rel;

        if ((_addr_abs & 0xFF00) != (pc & 0xFF00))
            _remaining_cycles++;

        pc = _addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BVS() {
    if (get_flag(V) == 1) {
        _remaining_cycles++;
        _addr_abs = pc + _addr_rel;

        if ((_addr_abs & 0xFF00) != (pc & 0xFF00))
            _remaining_cycles++;

        pc = _addr_abs;
    }
    return 0;
}

uint8_t cpu6502::CLC() { set_flag(C, false); return 0; }

uint8_t cpu6502::CLD() { set_flag(D, false); return 0; }

uint8_t cpu6502::CLI() { set_flag(I, false); return 0; }

uint8_t cpu6502::CLV() { set_flag(V, false); return 0; }

uint8_t cpu6502::CMP() {
    fetch();
    _temp = (uint16_t)a - (uint16_t)_fetched;
    set_flag(C, a >= _fetched);
    set_flag(Z, (_temp & 0x00FF) == 0x0000);
    set_flag(N, _temp & 0x0080);
    return 1;
}

uint8_t cpu6502::CPX() {
    fetch();
    _temp = (uint16_t)x - (uint16_t)_fetched;
    set_flag(C, x >= _fetched);
    set_flag(N, _temp & 0x0080);
    set_flag(Z, (_temp & 0x00FF) == 0x0000);
    return 0;
}

uint8_t cpu6502::CPY() {
    fetch();
    _temp = (uint16_t)y - (uint16_t)_fetched;
    set_flag(C, y >= _fetched);
    set_flag(N, _temp & 0x0080);
    set_flag(Z, (_temp & 0x00FF) == 0x0000);
    return 0;
}

uint8_t cpu6502::DEC() {
    fetch();
    _temp = _fetched - 1;
    set_flag(Z, (_temp & 0x00FF) == 0x0000);
    set_flag(N, _temp & 0x0080);

    write_to_bus(_addr_abs, _temp & 0x00FF);
    return 0;
}

uint8_t cpu6502::DEX() { x--; set_flag(Z, x == 0x00); set_flag(N, x & 0x80); return 0; }

uint8_t cpu6502::DEY() { y--; set_flag(Z, y == 0x00); set_flag(N, y & 0x80); return 0; }

uint8_t cpu6502::EOR() {
    fetch();
    a = a ^ _fetched;
    set_flag(Z, a == 0x00);
    set_flag(N, a & 0x80);
    return 1;
}

uint8_t cpu6502::INC() {
    fetch();
    _temp = _fetched + 1;
    write_to_bus(_addr_abs, _temp & 0x00FF);
    set_flag(Z, (_temp & 0x00FF) == 0x0000);
    set_flag(N, _temp & 0x0080);
    return 0;
}

uint8_t cpu6502::INX() { x++; set_flag(Z, x == 0x00); set_flag(N, x & 0x80); return 0; }

uint8_t cpu6502::INY() { y++; set_flag(Z, y == 0x00); set_flag(N, y & 0x80); return 0; }

uint8_t cpu6502::JMP() { pc = _addr_abs;  return 0; }

uint8_t cpu6502::JSR() {
    pc--;

    write_to_bus(BASE_STKP + stkp, (pc >> 8) & 0x00FF); stkp--;
    write_to_bus(BASE_STKP + stkp, pc & 0x00FF);        stkp--;

    pc = _addr_abs;
    return 0;
}

uint8_t cpu6502::LDA() {
    fetch();
    a = _fetched;
    set_flag(Z, a == 0x00);
    set_flag(N, a & 0x80);
    return 1;
}

uint8_t cpu6502::LDX() {
    fetch();
    x = _fetched;
    set_flag(Z, x == 0x00);
    set_flag(N, x & 0x80);
    return 1;
}

uint8_t cpu6502::LDY() {
    fetch();
    y = _fetched;
    set_flag(Z, y == 0x00);
    set_flag(N, y & 0x80);
    return 1;
}

uint8_t cpu6502::LSR() {
    fetch();
    _temp = _fetched >> 1;

    set_flag(C, _fetched & 0x0001);
    set_flag(Z, (_temp & 0x00FF) == 0x0000);
    set_flag(N, _temp & 0x0080);

    if (instructions_table[_opcode].addr_mode == &cpu6502::IMP)
        a = _temp & 0x00FF;
    else
        write_to_bus(_addr_abs, _temp & 0x00FF);
    return 0;
}

uint8_t cpu6502::NOP() {
    switch (_opcode) {
        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC: return 1; break;
        default: break;
    }
    return 0;
}

uint8_t cpu6502::ORA() { return 0; }
uint8_t cpu6502::PHA() { return 0; }
uint8_t cpu6502::PHP() { return 0; }
uint8_t cpu6502::PLA() { return 0; }
uint8_t cpu6502::PLP() { return 0; }
uint8_t cpu6502::ROL() { return 0; }
uint8_t cpu6502::ROR() { return 0; }
uint8_t cpu6502::RTI() { return 0; }
uint8_t cpu6502::RTS() { return 0; }
uint8_t cpu6502::SBC() { return 0; }
uint8_t cpu6502::SEC() { return 0; }
uint8_t cpu6502::SED() { return 0; }
uint8_t cpu6502::SEI() { return 0; }
uint8_t cpu6502::STA() { return 0; }
uint8_t cpu6502::STX() { return 0; }
uint8_t cpu6502::STY() { return 0; }
uint8_t cpu6502::TAX() { return 0; }
uint8_t cpu6502::TAY() { return 0; }
uint8_t cpu6502::TSX() { return 0; }
uint8_t cpu6502::TXA() { return 0; }
uint8_t cpu6502::TXS() { return 0; }
uint8_t cpu6502::TYA() { return 0; }

uint8_t cpu6502::XXX() { return 0; }
