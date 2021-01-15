#ifndef PPU_H_
#define PPU_H_

#include <memory>
#include "texture.h"
#include "mem.h"
#include "cartridge.h"

// Forward-declaration for class 'Bus' defined in 'bus.cpp'
class Bus;

class ppu2C02 {
public:
    ppu2C02();
    ~ppu2C02();

    void clock();
    void connect_to_bus(Bus *b);
    void connect_to_cartridge(const std::shared_ptr<Cartridge>& _cartridge);
    void reset();

/*=============================================================================
 * GUI HELPERS
 *===========================================================================*/
/* GUI helpers - used in 'Emulator' class to render video */
public:
    const SDL_Color &get_palette_from_offsets(uint8_t idx, uint8_t palette);
    void get_chr_rom_texture(std::shared_ptr<Texture> &chr_rom_text,
                                                uint8_t idx, uint8_t palette);

    void set_video_texture(std::shared_ptr<Texture> &video_text);
    void get_palettes_texture(std::shared_ptr<Texture> &palettes_text,
                                                uint8_t palette);
private:
    std::shared_ptr<Texture> _video_text;
    static const SDL_Color palettes[0x40];

private:
    Bus *bus;
    std::shared_ptr<Cartridge> cartridge;

/* PPU memory mapped registers */
private:
    /* PPU Status Register */
    union {
        uint8_t reg;
        struct __attribute__((__packed__)) {
            uint8_t unused : 5;
            uint8_t sprite_overflow : 1;
            uint8_t sprite_zero_hit : 1;
            uint8_t vertical_blank : 1;
        };
    } status_register;

    /* PPU Mask Register */
    union {
        uint8_t reg;
        struct __attribute__((__packed__)) {
            uint8_t grayscale : 1;
            uint8_t render_background_left : 1;
            uint8_t render_sprites_left : 1;
            uint8_t render_background : 1;
            uint8_t render_sprites : 1;
            uint8_t enhance_red : 1;
            uint8_t enhance_green : 1;
            uint8_t enhance_blue : 1;
        };
    } mask_register;

    /* PPU Control Register */
    union {
        uint8_t reg;
        struct __attribute__((__packed__)) {
            uint8_t nametable_x : 1;
            uint8_t nametable_y : 1;
            uint8_t increment_mode : 1;
            uint8_t pattern_sprite : 1;
            uint8_t pattern_background : 1;
            uint8_t sprite_size : 1;
            uint8_t slave_mode : 1;
            uint8_t enable_nmi : 1;
        };
    } control_register;

    /* PPU "Loopy" Register - (name tables) */
    union loopy_register {
        uint16_t reg = 0x0000;
        struct __attribute__((__packed__)) {
            uint16_t coarse_x : 5;
            uint16_t coarse_y : 5;
            uint16_t nametable_x : 1;
            uint16_t nametable_y : 1;
            uint16_t fine_y : 3;
            uint16_t unused : 1;
        };
    };

    loopy_register vram_addr;   // "pointer" address into name table
    loopy_register tram_addr;   // "temp" address into name table

    /* OAM Entry - internal memory to PPU storing sprites */
    struct OAMEntry {
        uint8_t y;
        uint8_t id;
        uint8_t attr;
        uint8_t x;
    } oam[64];

/* PPU foreground rendering helpers */
private:
    uint8_t oam_addr = 0x00;

    OAMEntry sprite_scanline[8];
    uint8_t _sprite_count;
    uint8_t _sprite_shifter_pattern_lo[8];
    uint8_t _sprite_shifter_pattern_hi[8];

    // Sprite zero collision flags
    bool _sprite_zero_hit = false;
    bool _sprite_zero_rendered = false;

    void _render_fg();

public:
    uint8_t *oam_ptr = (uint8_t *)oam;

/* PPU background rendering helpers */
private:
    uint8_t _fine_x = 0x00;     // Horizontal pixel offset

    // Background rendering attributes
    uint8_t   _bg_next_tile_id     = 0x00;
    uint8_t   _bg_next_tile_attrib = 0x00;
    uint8_t   _bg_next_tile_lsb    = 0x00;
    uint8_t   _bg_next_tile_msb    = 0x00;

    uint16_t  _bg_shifter_pattern_lo = 0x0000;
    uint16_t  _bg_shifter_pattern_hi = 0x0000;
    uint16_t  _bg_shifter_attrib_lo  = 0x0000;
    uint16_t  _bg_shifter_attrib_hi  = 0x0000;

    uint8_t _address_latch = 0x00;
    uint8_t _ppu_data_buffer = 0x00;

    void _increment_scroll_x();
    void _increment_scroll_y();
    void _transfer_address_x();
    void _transfer_address_y();
    void _load_bg_shifters();
    void _update_shifters();

    void _render_bg();

/*=============================================================================
 * PPU RAM
 *===========================================================================*/
private:
    uint8_t ppu_name_table[2][_1_KB];
    //uint8_t ppu_pattern_table[2][_4_KB];
    uint8_t ppu_palette_table[32];

    int16_t _scan_line;
    int16_t _cycle;
    bool _frame_completed;

public:
    bool frame_completed();
    void reset_frame();

/*=============================================================================
 * BUS COMMUNICATION
 *===========================================================================*/
public:
    uint8_t read_from_main_bus(uint16_t addr, bool read_only);
    void write_to_main_bus(uint16_t addr, uint8_t data);

    uint8_t read_from_ppu_bus(uint16_t addr, bool read_only);
    void write_to_ppu_bus(uint16_t addr, uint8_t data);

/* NMI */
private:
    bool _nmi;

public:
    bool nmi();
    void reset_nmi();
};
#endif
