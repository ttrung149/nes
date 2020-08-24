#include <sstream>

#include "emulator.h"

/* Constants */
#define OPEN_SANS_FONT_DIR "utils/open-sans.ttf"

#define VIDEO_WIDTH 760
#define VIDEO_HEIGHT 720

#define DEBUG_WIDTH 1050
#define DEBUG_HEIGHT 720

static const char *FLAGS_CHAR[8] = { "N", "V", "U", "B", "D", "I", "Z", "C" };
#define NUM_FLAGS 8
#define NUM_REGS 5
#define NUM_DISASM_INSTR 27

static const SDL_Color WHITE = { 255, 255, 255, 100 };
static const SDL_Color RED   = { 255, 0, 0, 80 };
static const SDL_Color BLUE  = { 0, 255, 255, 80 };
static const SDL_Color GREEN = { 0, 255, 0, 80 };
static const SDL_Color GREY  = { 200, 200, 200, 100 };

Emulator::Emulator(Emulator::MODE m) : mode(m) {
    (void) mode;

    cpu.connect_to_bus(&main_bus);
    main_bus.connect_to_cpu(&cpu);

    std::stringstream ss;
    ss << "A2 0A 8E 00 00 A2 03 8E 01 00 AC 00 00 A9 00 18 6D 01 00 88 D0 FA 8D 02 00 EA EA EA";
    uint16_t offset = 0x8000;
    while (!ss.eof()) {
        std::string b;
        ss >> b;
        main_bus.cpu_ram[offset++] = (uint8_t)std::stoul(b, nullptr, 16);
	}

    // Reset vector
    main_bus.cpu_ram[0xFFFC] = 0x00;
    main_bus.cpu_ram[0xFFFD] = 0x80;

    _disasm_instr_map = cpu.disasm(0x0000, 0xFFFF);
    cpu.reset();

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(DEBUG_WIDTH, DEBUG_HEIGHT, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 0);
    SDL_RenderClear(renderer);

    TTF_Init();
}

Emulator::~Emulator() { stop(); }

void Emulator::begin() {
    _init_flags_renderer();
    _init_regs_renderer();
    _init_disasm_renderer();

    assert(renderer);

    while (true) {
        SDL_RenderClear(renderer);
        SDL_PollEvent(&event);

        switch (event.type) {
            case SDL_KEYDOWN: {
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_N: {
                        do { cpu.clock(); } while (!cpu.instr_completed());
                        break;
                    }
                    case SDL_SCANCODE_R: {
                        cpu.reset();
                        break;
                    }
                    default: break;
                }
                break;
            }

            case SDL_QUIT: { stop(); return; }
            default: break;
        }

        _render_flags();
        _render_regs();
        _render_disasm();

        SDL_RenderPresent(renderer);
    }
}

void Emulator::stop() {
    assert(window);
    assert(renderer);

    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Emulator::_render_str(const std::string &str, TTF_Font *font,
                                     const SDL_Color &col, SDL_Rect &bound) {
    SDL_Surface *surf = TTF_RenderText_Solid(font, str.c_str(), col);
    assert(surf);
    SDL_Texture *text = SDL_CreateTextureFromSurface(renderer, surf);
    assert(text);

    bound.w = surf->w;      // Resize boundaries box's width
    bound.h = surf->h;      // Rezize boundaries box's height

    SDL_FreeSurface(surf);
    SDL_RenderCopy(renderer, text, nullptr, &bound);
    SDL_DestroyTexture(text);
}

/*=============================================================================
 * CPU REGISTERS RENDERER
 *===========================================================================*/
void Emulator::_init_regs_renderer() {
    regs_font = TTF_OpenFont(OPEN_SANS_FONT_DIR, 18);
    assert(regs_font);

    // A register                           // X register
    regs_rects[0].x = VIDEO_WIDTH + 10;     regs_rects[1].x = VIDEO_WIDTH + 70;
    regs_rects[0].y = 30;                   regs_rects[1].y = 30;

    // Y register                           // PC register
    regs_rects[2].x = VIDEO_WIDTH + 130;    regs_rects[3].x = VIDEO_WIDTH + 10;
    regs_rects[2].y = 30;                   regs_rects[3].y = 50;

    // STKP registers
    regs_rects[4].x = VIDEO_WIDTH + 100;
    regs_rects[4].y = 50;
}

void Emulator::_render_regs() {
    assert(regs_font);
    std::string str;
    str = "A: $"; str += cpu6502::hex_str(cpu.a, 2);
    _render_str(str, regs_font, WHITE, regs_rects[0]);

    str = "X: $"; str += cpu6502::hex_str(cpu.x, 2);
    _render_str(str, regs_font, WHITE, regs_rects[1]);

    str = "Y: $"; str += cpu6502::hex_str(cpu.y, 2);
    _render_str(str, regs_font, WHITE, regs_rects[2]);

    str = "PC: $"; str += cpu6502::hex_str(cpu.pc, 4);
    _render_str(str, regs_font, WHITE, regs_rects[3]);

    str = "STKP: $"; str += cpu6502::hex_str(cpu.stkp, 4);
    _render_str(str, regs_font, WHITE, regs_rects[4]);
}

/*=============================================================================
 * CPU FLAGS RENDERER
 *===========================================================================*/
void Emulator::_init_flags_renderer() {
    flags_font = TTF_OpenFont(OPEN_SANS_FONT_DIR, 18);
    assert(flags_font);

    for (int i = 0; i < NUM_FLAGS; ++i) {
        flags_rects[i].x = VIDEO_WIDTH + 10 + i * 24;
        flags_rects[i].y = 5;
    }
}

void Emulator::_render_flags() {
    assert(flags_font);
    for (int i = 0; i < NUM_FLAGS; ++i) {
        if (cpu.status & (0x88 >> i))
            _render_str(FLAGS_CHAR[i], flags_font, GREEN, flags_rects[i]);
        else
            _render_str(FLAGS_CHAR[i], flags_font, RED, flags_rects[i]);
    }
}

/*=============================================================================
 * DISASSEMBLED INSTRUCTIONS RENDERER
 *===========================================================================*/
void Emulator::_init_disasm_renderer() {
    disasm_font = TTF_OpenFont(OPEN_SANS_FONT_DIR, 14);
    assert(disasm_font);

    for (int i = 0; i < NUM_DISASM_INSTR; ++i) {
        disasm_instr_rects[i].x = VIDEO_WIDTH + 10;
        disasm_instr_rects[i].y = 65 + i * 18;
    }
}

void Emulator::_render_disasm() {
    assert(disasm_font);
    auto it = _disasm_instr_map.find(cpu.pc);
    auto _it = it;

    int instr_line_idx = NUM_DISASM_INSTR / 2 - 1;

    // Render the previous 10 instructions before the current instruction
    _it--;
    while (_it != _disasm_instr_map.end() && instr_line_idx > 0) {
        _render_str(_it->second, disasm_font, GREY, disasm_instr_rects[instr_line_idx]);
        instr_line_idx--;
        _it--;
    }

    // Render current instruction that PC is pointing to
    instr_line_idx = NUM_DISASM_INSTR / 2;
    _render_str(it->second, disasm_font, BLUE, disasm_instr_rects[instr_line_idx]);
    instr_line_idx++;

    // Render the next 10 instructions after the current instruction
    _it = it;
    _it++;
    while (_it != _disasm_instr_map.end() && instr_line_idx < NUM_DISASM_INSTR) {
        _render_str(_it->second, disasm_font, GREY, disasm_instr_rects[instr_line_idx]);
        instr_line_idx++;
        _it++;
    }
}

