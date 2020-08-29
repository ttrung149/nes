#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <map>

#include "bus.h"
#include "texture.h"

class Emulator {
public:
    enum MODE { DEBUG_MODE, NORMAL_MODE };

private:
   Bus main_bus;
   cpu6502 cpu;
   ppu2C02 ppu;
   std::shared_ptr<Cartridge> cartridge;

/*=============================================================================
 * GUI attributes
 *===========================================================================*/
private:
    std::shared_ptr<Texture> video_text;
    SDL_Rect video_rect;
    void _init_video_renderer();
    void _render_video();

private:
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Event event;
    MODE mode;

/* Emulator methods */
public:
    Emulator(MODE m);
    ~Emulator();

    void begin();
    void stop();

/*=============================================================================
 * Debugging GUI
 *===========================================================================*/
private:
    void _init_debugging_gui_renderer();
    void _render_debugging_gui();

/* Render string helper function */
private:
    void _render_str(const std::string &str, TTF_Font *font,
                                        const SDL_Color &col, SDL_Rect &bound);

/* GUI helper: CHR ROM display */
private:
    std::shared_ptr<Texture> chr_rom_texts[2];
    SDL_Rect chr_rom_rects[2];
    void _init_chr_rom_renderer();
    void _render_chr_rom();

/* GUI helper: Disassembler output display */
private:
    TTF_Font *disasm_font;
    SDL_Rect disasm_instr_rects[27];

    std::map<uint16_t, std::string>_disasm_instr_map;
    void _init_disasm_renderer();
    void _render_disasm();

/* GUI helper: Registers display */
private:
    TTF_Font *regs_font;
    SDL_Rect regs_rects[5];

    void _init_regs_renderer();
    void _render_regs();

/* GUI helper: Flags status bar */
private:
    TTF_Font *flags_font;
    SDL_Rect flags_rects[8];

    void _init_flags_renderer();
    void _render_flags();
};

#endif
