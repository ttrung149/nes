#include "emulator.h"

/* Constants */
#define OPEN_SANS_FONT_DIR "utils/open-sans.ttf"

#define VIDEO_WIDTH 512
#define VIDEO_HEIGHT 480

#define DEBUG_WIDTH 720 
#define DEBUG_HEIGHT 480

static const char *FLAGS_CHAR[8] = { "N", "V", "U", "B", "D", "I", "Z", "C" }; 
#define NUM_FLAGS 8

#define NUM_REGS 5

static const SDL_Color WHITE = { 255, 255, 255, 100 };
static const SDL_Color GREEN = { 0, 255, 0, 80 };
static const SDL_Color RED   = { 255, 0, 0, 80 };

Emulator::Emulator(Emulator::MODE m) : mode(m) {
    cpu.connect_to_bus(&main_bus);
    main_bus.connect_to_cpu(&cpu);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(DEBUG_WIDTH, DEBUG_HEIGHT, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    TTF_Init();
}

Emulator::~Emulator() { stop(); }

void Emulator::begin() {
    _init_flags_renderer();
    _init_regs_renderer();

    assert(renderer);

    while (true) {
        SDL_RenderClear(renderer);
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) { stop(); break; }

        _render_flags();
        _render_regs();

        SDL_RenderPresent(renderer);
    }
}

void Emulator::stop() {
    assert(window != nullptr);
    assert(renderer != nullptr);

    _destroy_flags_renderer();

    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

/*=============================================================================
 * UTILS FUNCTIONS 
 *===========================================================================*/
std::string Emulator::_hex_str(uint32_t num, uint8_t num_half_bytes) {
    std::string str(num_half_bytes, '0');
    for (int i = num_half_bytes - 1; i >= 0; i--, num >>= 4)
        str[i] = "0123456789ABCDEF"[num & 0xF];
    return str;
}

void Emulator::_render_str(const std::string &str, TTF_Font *font, 
                                     const SDL_Color &col, SDL_Rect &bound) {
    SDL_Surface *surf = TTF_RenderText_Solid(font, str.c_str(), col);
    assert(surf);

    SDL_Texture *text = SDL_CreateTextureFromSurface(renderer, surf);
    assert(text);

    bound.w = surf->w;
    bound.h = surf->h; 
    
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
    regs_rects[0].x = VIDEO_WIDTH;          regs_rects[1].x = VIDEO_WIDTH + 60;
    regs_rects[0].y = 30;                   regs_rects[1].y = 30;

    // Y register                           // PC register
    regs_rects[2].x = VIDEO_WIDTH + 120;    regs_rects[3].x = VIDEO_WIDTH;
    regs_rects[2].y = 30;                   regs_rects[3].y = 50;

    // STKP registers
    regs_rects[4].x = VIDEO_WIDTH + 90;
    regs_rects[4].y = 50;
}

void Emulator::_render_regs() {
    std::string str;
    str = " A: $"; str += _hex_str(cpu.a, 2);
    _render_str(str, regs_font, WHITE, regs_rects[0]);

    str = " X: $"; str += _hex_str(cpu.x, 2);
    _render_str(str, regs_font, WHITE, regs_rects[1]);

    str = " Y: $"; str += _hex_str(cpu.y, 2);
    _render_str(str, regs_font, WHITE, regs_rects[2]);

    str = " PC: $"; str += _hex_str(cpu.pc, 4);
    _render_str(str, regs_font, WHITE, regs_rects[3]);

    str = " STKP: $"; str += _hex_str(cpu.stkp, 4);
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

        // Configure textures for set flags
        SDL_Surface *set_surface = TTF_RenderText_Solid(flags_font, FLAGS_CHAR[i], GREEN);
        assert(set_surface);

        flags_rects[i].w = set_surface->w;
        flags_rects[i].h = set_surface->h; 

        set_flags_textures[i] = SDL_CreateTextureFromSurface(renderer, set_surface);
        assert(set_flags_textures[i]);

        SDL_FreeSurface(set_surface);
    
        // Configure textures for unset flags
        SDL_Surface *unset_surface = TTF_RenderText_Solid(flags_font, FLAGS_CHAR[i], RED);
        assert(unset_surface);

        unset_flags_textures[i] = SDL_CreateTextureFromSurface(renderer, unset_surface);
        assert(unset_flags_textures[i]);

        SDL_FreeSurface(unset_surface);
    }
}

void Emulator::_render_flags() {
    assert(flags_font);
    SDL_Texture *flags_texture = nullptr;

    for (int i = 0; i < NUM_FLAGS; ++i) {
        flags_texture = (cpu.status & (0x80 >> i)) ? set_flags_textures[i]
                                                   : unset_flags_textures[i];
        assert(flags_texture);
        SDL_RenderCopy(renderer, flags_texture, nullptr, &flags_rects[i]);
    }
}

void Emulator::_destroy_flags_renderer() {
    for (int i = 0; i < NUM_FLAGS; ++i) {
        SDL_DestroyTexture(set_flags_textures[i]);
        SDL_DestroyTexture(unset_flags_textures[i]);
    }
}

