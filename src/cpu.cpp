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

        // Increment program counter
        pc++;
        _remaining_cycles = instructions_table[_opcode].cycles;

        // Fetches data with proper addressing mode and executes instruction
        uint8_t add_cycle_1 = (this->*instructions_table[_opcode].addr_mode)();
        uint8_t add_cycle_2 = (this->*instructions_table[_opcode].operate)();

        // Add additional clock cycles (if exists)
        _remaining_cycles += (add_cycle_1 & add_cycle_2);

        // Unused flag
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
 * UTILS FUNCTIONS
 *===========================================================================*/
std::string cpu6502::hex_str(uint32_t num, uint8_t num_half_bytes) {
    std::string str(num_half_bytes, '0');
    for (int i = num_half_bytes - 1; i >= 0; i--, num >>= 4)
        str[i] = "0123456789ABCDEF"[num & 0xF];
    return str;
}

/* Disassembler routine - huge creds to javidx9 */
std::map<uint16_t, std::string> cpu6502::disasm(uint16_t begin, uint16_t end) {
    uint32_t addr = begin;
    uint8_t value = 0x00, lo = 0x00, hi = 0x00;
    std::map<uint16_t, std::string> disasm_ouput;
    uint16_t line_addr = 0;

    assert(bus);
    while (addr <= (uint32_t)end) {
        line_addr = addr;
        std::string instruction_str = "$" + hex_str(addr, 4) + ":  ";

        uint8_t opcode = bus->read(addr, true);
        addr++;
        instruction_str += instructions_table[opcode].mnemonic + " ";

        if (instructions_table[opcode].addr_mode == &cpu6502::IMP) {
            instruction_str += " {IMP}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::IMM) {
            value = bus->read(addr, true);
            addr++;
            instruction_str += "#$" + hex_str(value, 2) + " {IMM}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::ZP0) {
            lo = bus->read(addr, true);
            addr++;
            hi = 0x00;
            instruction_str += "$" + hex_str(lo, 2) + " {ZP0}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::ZPX) {
            lo = bus->read(addr, true); addr++;
            hi = 0x00;
            instruction_str += "$" + hex_str(lo, 2) + ", X {ZPX}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::ZPY) {
            lo = bus->read(addr, true); addr++;
            hi = 0x00;
            instruction_str += "$" + hex_str(lo, 2) + ", Y {ZPY}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::REL) {
            value = bus->read(addr, true); addr++;
            instruction_str += "$" + hex_str(value, 2) + " [$" + hex_str(addr + (int8_t)value, 4) + "] {REL}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::ABS) {
            lo = bus->read(addr, true); addr++;
            hi = bus->read(addr, true); addr++;
            instruction_str += "$" + hex_str((uint16_t)(hi << 8) | lo, 4) + " {ABS}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::ABX) {
            lo = bus->read(addr, true); addr++;
            hi = bus->read(addr, true); addr++;
            instruction_str += "$" + hex_str((uint16_t)(hi << 8) | lo, 4) + ", X {ABX}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::ABY) {
            lo = bus->read(addr, true); addr++;
            hi = bus->read(addr, true); addr++;
            instruction_str += "$" + hex_str((uint16_t)(hi << 8) | lo, 4) + ", Y {ABY}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::IND) {
            lo = bus->read(addr, true); addr++;
            hi = bus->read(addr, true); addr++;
            instruction_str += "($" + hex_str((uint16_t)(hi << 8) | lo, 4) + ") {IND}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::IZX) {
            lo = bus->read(addr, true); addr++;
            hi = 0x00;
            instruction_str += "($" + hex_str(lo, 2) + ", X) {IZX}";
        }
        else if (instructions_table[opcode].addr_mode == &cpu6502::IZY) {
            lo = bus->read(addr, true); addr++;
            hi = 0x00;
            instruction_str += "($" + hex_str(lo, 2) + "), Y {IZY}";
        }
        else {
            instruction_str += "ILLEGAL ADDRESSING MODE";
        }

        disasm_ouput[line_addr] = instruction_str;
    }

    return disasm_ouput;
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
uint8_t cpu6502::ADC() {
    fetch();
    _temp = (uint16_t)a + (uint16_t)_fetched + (uint16_t)get_flag(C);

    set_flag(C, _temp > 255);
    set_flag(Z, (_temp & 0x00FF) == 0);
    set_flag(V, (~((uint16_t)a ^ (uint16_t)_fetched) &
                                    ((uint16_t)a ^ (uint16_t)_temp)) & 0x0080);
    set_flag(N, _temp & 0x80);
    a = _temp & 0x00FF;

    return 1;
}

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

uint8_t cpu6502::BRK() {
    pc++;

    write_to_bus(BASE_STKP + stkp, (pc >> 8) & 0x00FF);
    stkp--;
    write_to_bus(BASE_STKP + stkp, pc & 0x00FF);
    stkp--;

    set_flag(I, true);
    set_flag(B, true);
    write_to_bus(BASE_STKP + stkp, status);
    stkp--;

    set_flag(B, false);
    pc = (uint16_t)read_from_bus(0xFFFE) | ((uint16_t)read_from_bus(0xFFFF) << 8);

    return 0;
}

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

uint8_t cpu6502::ORA() {
    fetch();
    a |= _fetched;
    set_flag(Z, a == 0x00);
    set_flag(N, a & 0x80);
    return 1;
}

uint8_t cpu6502::PHA() {
    write_to_bus(BASE_STKP + stkp, a);
    stkp--;
    return 0;
}

uint8_t cpu6502::PHP() {
    write_to_bus(BASE_STKP + stkp, status | B | U);
    set_flag(B, false);
    set_flag(U, false);
    stkp--;
    return 0;
}

uint8_t cpu6502::PLA() {
    stkp++;
    a = read_from_bus(BASE_STKP + stkp);
    set_flag(Z, a == 0x00);
    set_flag(N, a & 0x80);
    return 0;
}

uint8_t cpu6502::PLP() {
    stkp++;
    status = read_from_bus(BASE_STKP + stkp);
    set_flag(U, 1);
    return 0;
}

uint8_t cpu6502::ROL() {
    fetch();

    _temp = (uint16_t)(_fetched << 1) | get_flag(C);
    set_flag(N, _temp & 0x0080);
    set_flag(Z, (_temp & 0x00FF) == 0x0000);
    set_flag(C, _temp & 0xFF00);
    if (instructions_table[_opcode].addr_mode == &cpu6502::IMP)
        a = _temp & 0x00FF;
    else
        write_to_bus(_addr_abs, _temp & 0x00FF);
    return 0;
}

uint8_t cpu6502::ROR() {
    fetch();
    _temp = (uint16_t)(get_flag(C) << 7) | (_fetched >> 1);
    set_flag(N, _temp & 0x0080);
    set_flag(Z, (_temp & 0x00FF) == 0x00);
    set_flag(C, _fetched & 0x01);
    if (instructions_table[_opcode].addr_mode == &cpu6502::IMP)
        a = _temp & 0x00FF;
    else
        write_to_bus(_addr_abs, _temp & 0x00FF);
    return 0;
}

uint8_t cpu6502::RTI() {
    stkp++;
    status = read_from_bus(BASE_STKP + stkp);
    status &= ~U;
    status &= ~B;

    stkp++;
    pc = (uint16_t)read_from_bus(BASE_STKP + stkp);
    stkp++;
    pc |= (uint16_t)read_from_bus(BASE_STKP + stkp) << 8;
    return 0;
}

uint8_t cpu6502::RTS() {
    stkp++;
    pc = (uint16_t)read_from_bus(0x0100 + stkp);
    stkp++;
    pc |= (uint16_t)read_from_bus(0x0100 + stkp) << 8;

    pc++;
    return 0;
}

uint8_t cpu6502::SBC() {
    fetch();
    uint16_t val = ((uint16_t)_fetched) ^ 0x00FF;
    _temp = (uint16_t)a + val + (uint16_t)get_flag(C);

    set_flag(C, _temp & 0xFF00);
    set_flag(Z, ((_temp & 0x00FF) == 0));
    set_flag(V, (_temp ^ (uint16_t)a) & (_temp ^ val) & 0x0080);
    set_flag(N, _temp & 0x0080);

    a = _temp & 0x00FF;
    return 1;
}

uint8_t cpu6502::SEC() { set_flag(C, true); return 0; }

uint8_t cpu6502::SED() { set_flag(D, true); return 0; }

uint8_t cpu6502::SEI() { set_flag(I, true); return 0; }

uint8_t cpu6502::STA() { write_to_bus(_addr_abs, a); return 0; }

uint8_t cpu6502::STX() { write_to_bus(_addr_abs, x); return 0; }

uint8_t cpu6502::STY() { write_to_bus(_addr_abs, y); return 0; }

uint8_t cpu6502::TAX() { x = a; set_flag(Z, x == 0x00); set_flag(N, x & 0x80); return 0; }

uint8_t cpu6502::TAY() { y = a; set_flag(Z, y == 0x00); set_flag(N, y & 0x80); return 0; }

uint8_t cpu6502::TSX() { x = stkp; set_flag(Z, x == 0x00); set_flag(N, x & 0x80); return 0; }

uint8_t cpu6502::TXA() { a = x; set_flag(Z, a == 0x00); set_flag(N, a & 0x80); return 0; }

uint8_t cpu6502::TXS() { stkp = x; return 0; }

uint8_t cpu6502::TYA() { a = y; set_flag(Z, a == 0x00); set_flag(N, a & 0x80); return 0; }

uint8_t cpu6502::XXX() { return 0; }

bool cpu6502::instr_completed() { return _remaining_cycles == 0; }
