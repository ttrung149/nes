#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "cpu.h"
#include "bus.h"

#ifndef EMULATOR_H_
#define EMULATOR_H_

class Emulator {
public:
    enum MODE { DEBUG_MODE, NORMAL_MODE };

private:
   Bus main_bus; 
   cpu6502 cpu;

// GUI attributes
private:
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Event event;
    MODE mode;

// Emulator methods
public:
    Emulator(MODE m);
    ~Emulator();

    void begin();
    void stop();

// GUI display helper functions
private:
    void _init_gui(MODE m);

private:
    void _init_display_disassembler();
    void _display_disassembler();

private:
    void _init_display_mem_seg();
    void _display_mem_seg();

//-----------------------------------------------------------------------------
// Debugging GUI
//-----------------------------------------------------------------------------
private:
    std::string _hex_str(uint32_t num, uint8_t num_half_bytes);
    void _render_str(const std::string &str, TTF_Font *font,
                                        const SDL_Color &col, SDL_Rect &bound);
// GUI helper: Registers display
private:
    TTF_Font *regs_font;
    SDL_Rect regs_rects[5];

    void _init_regs_renderer();
    void _render_regs();

// GUI helper: Flags status bar
private:
    TTF_Font *flags_font;
    SDL_Rect flags_rects[8];

    SDL_Texture *set_flags_textures[8];
    SDL_Texture *unset_flags_textures[8];

    void _init_flags_renderer();
    void _render_flags();
    void _destroy_flags_renderer();
};

#endif
