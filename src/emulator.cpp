#include "emulator.h"

#define OPEN_SANS_FONT_DIR "utils/open-sans.ttf"
static const std::chrono::duration<int ,std::micro> REFRESH_PERIOD(16667);

/* NES resolution */
#define NES_WINDOW_WIDTH 256
#define NES_WINDOW_HEIGHT 240

/* GUI resolution */
#define VIDEO_WIDTH 768
#define VIDEO_HEIGHT 720

/* GUI size (with debugging display) */
#define DEBUG_WIDTH 1056
#define DEBUG_HEIGHT 720

static const char *FLAGS_CHAR[8] = { "N", "V", "_", "B", "D", "I", "Z", "C" };
#define NUM_FLAGS 8
#define NUM_REGS 5
#define NUM_DISASM_INSTR 25
#define CHR_ROM_SIZE 128
#define NUM_PALETTE_SELECTION 8

static const SDL_Color WHITE = { 255, 255, 255, 100 };
static const SDL_Color GREY  = { 180, 180, 180, 100 };
static const SDL_Color RED   = { 255, 0, 0, 100 };
static const SDL_Color BLUE  = { 0, 255, 255, 100 };
static const SDL_Color GREEN = { 0, 255, 0, 100 };

/*=============================================================================
 * EMULATOR METHODS
 *===========================================================================*/
Emulator::Emulator(Emulator::MODE m) : mode(m) {

    // Create bus connection
    cpu.connect_to_bus(&main_bus);
    main_bus.connect_to_cpu(&cpu);

    ppu.connect_to_bus(&main_bus);
    main_bus.connect_to_ppu(&ppu);

    cartridge = std::make_shared<Cartridge>("roms/nestest.nes");
    main_bus.connect_to_cartridge(cartridge);

    _disasm_instr_map = cpu.disasm(0x0000, 0xFFFF);
    cpu.reset();

    SDL_Init(SDL_INIT_VIDEO);

    // Create window size based on mode
    if (mode == DEBUG_MODE) {
        SDL_CreateWindowAndRenderer(DEBUG_WIDTH, DEBUG_HEIGHT, 0, &window, &renderer);
        _is_emulating = false;
    }
    else if (mode == NORMAL_MODE) {
        SDL_CreateWindowAndRenderer(VIDEO_WIDTH, VIDEO_HEIGHT, 0, &window, &renderer);
        _is_emulating = true;
    }

    SDL_RenderClear(renderer);
    TTF_Init();
}

Emulator::~Emulator() { stop(); }

void Emulator::_handle_debug_inputs(SDL_Scancode code) {
    switch (code) {
        case SDL_SCANCODE_N: {
            do { cpu.clock(); } while (!cpu.instr_completed());
            do { cpu.clock(); } while (cpu.instr_completed());
            break;
        }
        case SDL_SCANCODE_F: {
            do { main_bus.clock(); } while (!ppu.frame_completed());
            do { main_bus.clock(); } while (!cpu.instr_completed());
            ppu.reset_frame();
            break;
        }
        case SDL_SCANCODE_P: {
            _curr_palette_selection += 1;
            _curr_palette_selection &= 0x07;
            break;
        }
        case SDL_SCANCODE_SPACE: { _is_emulating = !_is_emulating; break; }
        case SDL_SCANCODE_R: { main_bus.reset(); break; }
        default: break;
    }
}

void Emulator::_handle_controller_inputs(SDL_Scancode code) {
    switch (code) {
        // First controller: up, left, down, right
        case SDL_SCANCODE_W:        main_bus.controller[0] |= 0x08; break;
        case SDL_SCANCODE_A:        main_bus.controller[0] |= 0x02; break;
        case SDL_SCANCODE_S:        main_bus.controller[0] |= 0x04; break;
        case SDL_SCANCODE_D:        main_bus.controller[0] |= 0x01; break;

        // First controller: A, B, Select, Start
        case SDL_SCANCODE_Q:        main_bus.controller[0] |= 0x80; break;
        case SDL_SCANCODE_E:        main_bus.controller[0] |= 0x40; break;
        case SDL_SCANCODE_LSHIFT:   main_bus.controller[0] |= 0x20; break;
        case SDL_SCANCODE_RETURN:   main_bus.controller[0] |= 0x10; break;

        // Second controller: up, left, down, right
        case SDL_SCANCODE_UP:       main_bus.controller[1] |= 0x08; break;
        case SDL_SCANCODE_LEFT:     main_bus.controller[1] |= 0x02; break;
        case SDL_SCANCODE_DOWN:     main_bus.controller[1] |= 0x04; break;
        case SDL_SCANCODE_RIGHT:    main_bus.controller[1] |= 0x01; break;

        // Second controller: A, B, Select, Start
        case SDL_SCANCODE_O:        main_bus.controller[1] |= 0x80; break;
        case SDL_SCANCODE_P:        main_bus.controller[1] |= 0x40; break;
        case SDL_SCANCODE_U:        main_bus.controller[1] |= 0x20; break;
        case SDL_SCANCODE_I:        main_bus.controller[1] |= 0x10; break;
        default: break;
    }
}

/* Begin emulation */
void Emulator::begin() {
    using namespace std::chrono;

    _init_video_renderer();
    if (mode == DEBUG_MODE) _init_debugging_gui_renderer();
    assert(renderer);

    while (true) {
        SDL_RenderClear(renderer);  SDL_PollEvent(&event);
        switch (event.type) {
            case SDL_KEYDOWN:
                _handle_controller_inputs(event.key.keysym.scancode);
                if (mode == DEBUG_MODE)
                    _handle_debug_inputs(event.key.keysym.scancode);
                break;
            case SDL_KEYUP:
                main_bus.controller[0] = 0x00;
                main_bus.controller[1] = 0x00;
                break;
            case SDL_QUIT: stop();
                return;
        }

        if (_is_emulating && system_clock::now() - _start > REFRESH_PERIOD) {
            do { main_bus.clock(); } while (!ppu.frame_completed());
            ppu.reset_frame();
            _start = system_clock::now();
        }

        if (mode == DEBUG_MODE) _render_debugging_gui();
        _render_video();
        SDL_RenderPresent(renderer);
    }
}

/* Stop emulation */
void Emulator::stop() {
    assert(window);
    assert(renderer);

    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

/*=============================================================================
 * VIDEO RENDERER
 *===========================================================================*/
void Emulator::_init_video_renderer() {
    video_rect.x = 0;
    video_rect.y = 0;
    video_rect.w = VIDEO_WIDTH;
    video_rect.h = VIDEO_HEIGHT;

    assert(renderer);
    video_text = std::make_shared<Texture>(renderer,
                            NES_WINDOW_WIDTH, NES_WINDOW_HEIGHT, video_rect);
    ppu.set_video_texture(video_text);
}

void Emulator::_render_video() { video_text->render_texture(); }

/*=============================================================================
 * DEBUGGING GUI HELPERS
 *===========================================================================*/
void Emulator::_init_debugging_gui_renderer() {
    _init_flags_renderer();
    _init_regs_renderer();
    _init_disasm_renderer();
    _init_palette_selection_renderer();
    _init_chr_rom_renderer();
}

void Emulator::_render_debugging_gui() {
    _render_flags();
    _render_regs();
    _render_disasm();
    _render_palette_selection();
    _render_chr_rom();
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
    assert(renderer);
    SDL_RenderCopy(renderer, text, nullptr, &bound);
    SDL_DestroyTexture(text);
}

/*=============================================================================
 * PALETTE SELECTION GUI RENDERER
 *===========================================================================*/
void Emulator::_init_palette_selection_renderer() {
    _curr_palette_selection = 0;
    assert(renderer);
    for (uint8_t i = 0; i < NUM_PALETTE_SELECTION; ++i) {
        palettes_rects[i].x = VIDEO_WIDTH + 10 + 30 * i;
        palettes_rects[i].y = 175;
        palettes_rects[i].w = 20;
        palettes_rects[i].h = 5;

        palettes_texts[i] = std::make_shared<Texture>(renderer, 4, 1, palettes_rects[i]);
    }
}

void Emulator::_render_palette_selection() {
    assert(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(
        renderer,
        VIDEO_WIDTH + 10 + 30 * _curr_palette_selection, 188,
        VIDEO_WIDTH + 10 + 30 * _curr_palette_selection + 20, 188
    );
    SDL_SetRenderDrawColor(renderer, 25, 25, 25, 100);

    for (uint8_t i = 0; i < NUM_PALETTE_SELECTION; ++i) {
        ppu.get_palettes_texture(palettes_texts[i], i);
        palettes_texts[i]->render_texture();
    }
}

/*=============================================================================
 * CHR ROM GUI RENDERER
 *===========================================================================*/
void Emulator::_init_chr_rom_renderer() {
    chr_rom_rects[0].x = VIDEO_WIDTH + 10;  chr_rom_rects[1].x = VIDEO_WIDTH + 148;
    chr_rom_rects[0].y = 40;                chr_rom_rects[1].y = 40;
    chr_rom_rects[0].w = CHR_ROM_SIZE;      chr_rom_rects[1].w = CHR_ROM_SIZE;
    chr_rom_rects[0].h = CHR_ROM_SIZE;      chr_rom_rects[1].h = CHR_ROM_SIZE;

    assert(renderer);
    chr_rom_texts[0] = std::make_shared<Texture>(renderer,
                            CHR_ROM_SIZE, CHR_ROM_SIZE, chr_rom_rects[0]);
    chr_rom_texts[1] = std::make_shared<Texture>(renderer,
                            CHR_ROM_SIZE, CHR_ROM_SIZE, chr_rom_rects[1]);
}

void Emulator::_render_chr_rom() {
    ppu.get_chr_rom_texture(chr_rom_texts[0], 0, _curr_palette_selection);
    ppu.get_chr_rom_texture(chr_rom_texts[1], 1, _curr_palette_selection);

    for (const auto &text : chr_rom_texts) text->render_texture();
}

/*=============================================================================
 * CPU REGISTERS GUI RENDERER
 *===========================================================================*/
void Emulator::_init_regs_renderer() {
    regs_font = TTF_OpenFont(OPEN_SANS_FONT_DIR, 18);
    assert(regs_font);

    // A register                           // X register
    regs_rects[0].x = VIDEO_WIDTH + 10;     regs_rects[1].x = VIDEO_WIDTH + 80;
    regs_rects[0].y = 195;                  regs_rects[1].y = 195;

    // Y register                           // PC register
    regs_rects[2].x = VIDEO_WIDTH + 150;    regs_rects[3].x = VIDEO_WIDTH + 10;
    regs_rects[2].y = 195;                  regs_rects[3].y = 215;

    // STKP registers
    regs_rects[4].x = VIDEO_WIDTH + 110;
    regs_rects[4].y = 215;
}

void Emulator::_render_regs() {
    assert(regs_font);
    std::string str;
    str = "A: $"; str += cpu6502::hex_str(cpu.a, 2);
    _render_str(str, regs_font, GREY, regs_rects[0]);

    str = "X: $"; str += cpu6502::hex_str(cpu.x, 2);
    _render_str(str, regs_font, GREY, regs_rects[1]);

    str = "Y: $"; str += cpu6502::hex_str(cpu.y, 2);
    _render_str(str, regs_font, GREY, regs_rects[2]);

    str = "PC: $"; str += cpu6502::hex_str(cpu.pc, 4);
    _render_str(str, regs_font, GREY, regs_rects[3]);

    str = "STKP: $"; str += cpu6502::hex_str(cpu.stkp, 4);
    _render_str(str, regs_font, GREY, regs_rects[4]);
}

/*=============================================================================
 * CPU FLAGS GUI RENDERER
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
        if (cpu.status & (0x80 >> i))
            _render_str(FLAGS_CHAR[i], flags_font, GREEN, flags_rects[i]);
        else
            _render_str(FLAGS_CHAR[i], flags_font, RED, flags_rects[i]);
    }
}

/*=============================================================================
 * DISASSEMBLED INSTRUCTIONS GUI RENDERER
 *===========================================================================*/
void Emulator::_init_disasm_renderer() {
    disasm_font = TTF_OpenFont(OPEN_SANS_FONT_DIR, 14);
    assert(disasm_font);
    for (int i = 0; i < NUM_DISASM_INSTR; ++i) {
        disasm_instr_rects[i].x = VIDEO_WIDTH + 10;
        disasm_instr_rects[i].y = 245 + i * 18;
    }
}

void Emulator::_render_disasm() {
    assert(disasm_font);
    auto it = _disasm_instr_map.find(cpu.pc);
    if (it == _disasm_instr_map.end()) {
        _render_str("Press 'N' to continue", disasm_font, WHITE, disasm_instr_rects[0]);
        return;
    }

    auto _it = it;
    int instr_line_idx = NUM_DISASM_INSTR / 2 - 1;

    // Render the previous 10 instructions before the current instruction
    _it--;
    while (_it != _disasm_instr_map.end() && instr_line_idx >= 0) {
        _render_str(_it->second, disasm_font, WHITE, disasm_instr_rects[instr_line_idx]);
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
        _render_str(_it->second, disasm_font, WHITE, disasm_instr_rects[instr_line_idx]);
        instr_line_idx++;
        _it++;
    }
}
