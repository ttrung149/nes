// Major creds to javidx9 for the PPU tutorial. Would never be able to understand it
// all without this resource. Thanks so much :)
// https://www.youtube.com/watch?v=cksywUTZxlY&ab_channel=javidx9

#include "ppu.h"

/*=============================================================================
 * PPU methods
 *===========================================================================*/
ppu2C02::ppu2C02() : _scan_line(0), _cycle(0), _frame_completed(false) {}

ppu2C02::~ppu2C02() {}

void ppu2C02::connect_to_cartridge(const std::shared_ptr<Cartridge>& _cartridge) {
    assert(_cartridge);
    cartridge = _cartridge;
}

void ppu2C02::connect_to_bus(Bus *b) { assert(b); bus = b; }

/*=============================================================================
 * GUI HELPERS
 *===========================================================================*/
/* GUI helpers - set NES video texture for video rendering */
void ppu2C02::set_video_texture(std::shared_ptr<Texture> &video_text) {
    assert(video_text);
    _video_text = video_text;
}

/* GUI helpers - update palette selection texture */
void ppu2C02::get_palettes_texture(std::shared_ptr<Texture> &palettes_text,
                                                uint8_t palette) {
    for (uint8_t i = 0; i < 4; i++) {
        SDL_Color color = get_palette_from_offsets(i, palette);
        palettes_text->update_texture(i, color.r, color.g, color.b);
    }
}

/* GUI helpers - get palette from offset */
const SDL_Color &ppu2C02::get_palette_from_offsets(uint8_t idx, uint8_t palette) {
    return palettes[
        read_from_ppu_bus(PALETTE_ADDR_LOWER + (palette << 2) + idx, false) & 0x3F
    ];
}

/* GUI helpers - update pattern memory texture */
void ppu2C02::get_chr_rom_texture(std::shared_ptr<Texture> &chr_rom_text,
                                                uint8_t idx, uint8_t palette) {
    // Iterate through each tile
    for (uint16_t row_tile = 0; row_tile < 16; row_tile++) {
        for (uint16_t col_tile = 0; col_tile < 16; col_tile++) {

            // Get byte offset
            uint16_t b_offset = (((row_tile << 4) + col_tile) << 4);

            // Begin iteration through each pixel in tile
            for (uint8_t row_px = 0; row_px < 8; row_px++) {
                uint8_t lsb = read_from_ppu_bus(idx * 0x1000 + b_offset + row_px, false);
                uint8_t msb = read_from_ppu_bus(idx * 0x1000 + b_offset + row_px + 8, false);

                for (uint8_t col_px = 0; col_px < 8; col_px++) {
                    uint8_t px = ((msb & 0x01) << 1) | (lsb & 0x01);

                    // Get palette color
                    SDL_Color color = get_palette_from_offsets(px, palette);

                    // Update passed-in CHR ROM texture
                    uint16_t x = (col_tile << 3) + 7 - col_px;
                    uint16_t y = (row_tile << 3) + row_px;
                    chr_rom_text->update_texture((y << 7) + x, color.r, color.g, color.b);

                    lsb >>= 1; msb >>= 1;
                }
            }   // End pixel iteration
        }
    }   // End tile iteration
}

/*=============================================================================
 * PPU BG RENDERING HELPERS
 *===========================================================================*/
/* Increment the background tile "pointer" one tile horizontally */
void ppu2C02::_increment_scroll_x() {
    if (mask_register.render_background || mask_register.render_sprites) {
        if (vram_addr.coarse_x == 31) {
            vram_addr.coarse_x = 0;
            vram_addr.nametable_x = ~vram_addr.nametable_x;
        }
        else vram_addr.coarse_x++;
    }
}

/* Increment the background tile "pointer" one tile vertically */
void ppu2C02::_increment_scroll_y() {
    if (mask_register.render_background || mask_register.render_sprites) {
        if (vram_addr.fine_y < 7) vram_addr.fine_y++;
        else {
            vram_addr.fine_y = 0;
            if (vram_addr.coarse_y == 29) {
                vram_addr.coarse_y = 0;
                vram_addr.nametable_y = ~vram_addr.nametable_y;
            }
            else if (vram_addr.coarse_y == 31) {
                vram_addr.coarse_y = 0;
            }
            else {
                vram_addr.coarse_y++;
            }
        }
    }
}

/* Transfer horizontal name table data from 'temp' to actual register */
void ppu2C02::_transfer_address_x() {
    if (mask_register.render_background || mask_register.render_sprites) {
        vram_addr.nametable_x = tram_addr.nametable_x;
        vram_addr.coarse_x    = tram_addr.coarse_x;
    }
}

/* Transfer vertical name table data from 'temp' to actual register */
void ppu2C02::_transfer_address_y() {
    if (mask_register.render_background || mask_register.render_sprites) {
        vram_addr.fine_y      = tram_addr.fine_y;
        vram_addr.nametable_y = tram_addr.nametable_y;
        vram_addr.coarse_y    = tram_addr.coarse_y;
    }
}

void ppu2C02::_load_bg_shifters() {
    _bg_shifter_pattern_lo = (_bg_shifter_pattern_lo & 0xFF00) | _bg_next_tile_lsb;
    _bg_shifter_pattern_hi = (_bg_shifter_pattern_hi & 0xFF00) | _bg_next_tile_msb;
    _bg_shifter_attrib_lo  = (_bg_shifter_attrib_lo & 0xFF00)  |
                                        ((_bg_next_tile_attrib & 0x01) ? 0xFF : 0x00);
    _bg_shifter_attrib_hi  = (_bg_shifter_attrib_hi & 0xFF00)  |
                                        ((_bg_next_tile_attrib & 0x02) ? 0xFF : 0x00);
}

void ppu2C02::_update_shifters() {
    if (mask_register.render_background) {
        _bg_shifter_pattern_lo <<= 1;   _bg_shifter_pattern_hi <<= 1;
        _bg_shifter_attrib_lo <<= 1;    _bg_shifter_attrib_hi <<= 1;
    }

    if (mask_register.render_sprites && _cycle >= 1 && _cycle < 258) {
        for (uint8_t i = 0; i < _sprite_count; i++) {
            if (sprite_scanline[i].x > 0) {
                sprite_scanline[i].x--;
            }
            else {
                _sprite_shifter_pattern_lo[i] <<= 1;
                _sprite_shifter_pattern_hi[i] <<= 1;
            }
        }
    }
}

/*=============================================================================
 * PPU CLOCK TICKS
 *===========================================================================*/
/* Background rendering */
void ppu2C02::_render_bg() {
    if (_scan_line == 0 && _cycle == 0) _cycle = 1;
    if (_scan_line == -1 && _cycle == 1) {
        status_register.vertical_blank = 0;
        status_register.sprite_overflow = 0;
        status_register.sprite_zero_hit = 0;

        std::memset(_sprite_shifter_pattern_lo, 0x00, 8 * sizeof(uint8_t));
        std::memset(_sprite_shifter_pattern_hi, 0x00, 8 * sizeof(uint8_t));
    }

    if ((_cycle >= 2 && _cycle < 258) || (_cycle >= 321 && _cycle < 338)) {
        _update_shifters();

        switch ((_cycle - 1) % 8) {
        case 0:
            _load_bg_shifters();
            _bg_next_tile_id =
                read_from_ppu_bus((0x2000 | (vram_addr.reg & 0x0FFF)), false);
            break;
        case 2:
            _bg_next_tile_attrib =
                read_from_ppu_bus((0x23C0 | (vram_addr.nametable_y << 11)
                                          | (vram_addr.nametable_x << 10)
                                          | ((vram_addr.coarse_y >> 2) << 3)
                                          | (vram_addr.coarse_x >> 2)), false);

            if (vram_addr.coarse_y & 0x02) _bg_next_tile_attrib >>= 4;
            if (vram_addr.coarse_x & 0x02) _bg_next_tile_attrib >>= 2;
            _bg_next_tile_attrib &= 0x03;
            break;
        case 4:
            _bg_next_tile_lsb =
                read_from_ppu_bus(((control_register.pattern_background << 12)
                                + ((uint16_t)_bg_next_tile_id << 4)
                                + (vram_addr.fine_y) + 0), false);
            break;
        case 6:
            _bg_next_tile_msb =
                read_from_ppu_bus(((control_register.pattern_background << 12)
                                + ((uint16_t)_bg_next_tile_id << 4)
                                + (vram_addr.fine_y) + 8), false);
            break;
        case 7:
            _increment_scroll_x();
            break;
        }
    }

    if (_cycle == 256) _increment_scroll_y();
    if (_cycle == 257) {
        _load_bg_shifters();
        _transfer_address_x();
    }

    if (_cycle == 338 || _cycle == 340) {
        _bg_next_tile_id = read_from_ppu_bus((0x2000 | (vram_addr.reg & 0x0FFF)), false);
    }

    if (_scan_line == -1 && _cycle >= 280 && _cycle < 305) {
        _transfer_address_y();
    }
}

/* Foreground rendering */
void ppu2C02::_render_fg() {
    if (_cycle == 257 && _scan_line >= 0) {
        _sprite_count = 0;

        // Reset sprite buffer
        std::memset(sprite_scanline, 0xFF, 8 * sizeof(OAMEntry));
        std::memset(_sprite_shifter_pattern_lo, 0x00, 8 * sizeof(uint8_t));
        std::memset(_sprite_shifter_pattern_hi, 0x00, 8 * sizeof(uint8_t));

        _sprite_zero_hit = false;
        uint8_t num_sprite_entries = 0;

        while (num_sprite_entries < 64 && _sprite_count < 9) {
            int16_t diff = (int16_t)_scan_line - (int16_t)oam[num_sprite_entries].y;
            int16_t sprite_size = control_register.sprite_size ? 16 : 8;

            if (diff >= 0 && diff < sprite_size && _sprite_count < 8) {
                if (num_sprite_entries == 0) {
                    _sprite_zero_hit = true;
                }

                std::memcpy(&sprite_scanline[_sprite_count], &oam[num_sprite_entries],
                                                                    sizeof(OAMEntry));
                _sprite_count++;
            }

            num_sprite_entries++;
        }
        status_register.sprite_overflow = (_sprite_count > 8);
    }

    if (_cycle == 340) {
        for (uint8_t i = 0; i < _sprite_count; i++) {
            uint8_t sprite_pattern_bits_lo, sprite_pattern_bits_hi;
            uint16_t sprite_pattern_addr_lo, sprite_pattern_addr_hi;

            // 8x8 sprite mode
            if (!control_register.sprite_size) {
                // normal orientation
                if (!(sprite_scanline[i].attr & 0x80)) {
                    sprite_pattern_addr_lo =
                        (control_register.pattern_sprite << 12)
                        | (sprite_scanline[i].id << 4)
                        | (_scan_line - sprite_scanline[i].y);

                }
                // vertically flipped
                else {
                    sprite_pattern_addr_lo =
                        (control_register.pattern_sprite << 12)
                        | (sprite_scanline[i].id << 4)
                        | (7 - (_scan_line - sprite_scanline[i].y));
                }
            }

            // 8x16 sprite mode
            else {
                // normal orientation
                if (!(sprite_scanline[i].attr & 0x80)) {
                    if (_scan_line - sprite_scanline[i].y < 8) {
                        sprite_pattern_addr_lo =
                            ((sprite_scanline[i].id & 0x01) << 12)
                            | ((sprite_scanline[i].id & 0xFE) << 4)
                            | ((_scan_line - sprite_scanline[i].y) & 0x07);
                    }
                    else {
                        sprite_pattern_addr_lo =
                            ((sprite_scanline[i].id & 0x01) << 12)
                            | (((sprite_scanline[i].id & 0xFE) + 1) << 4)
                            | ((_scan_line - sprite_scanline[i].y) & 0x07);
                    }
                }
                // vertically flipped
                else {
                    if (_scan_line - sprite_scanline[i].y < 8) {
                        sprite_pattern_addr_lo =
                            ((sprite_scanline[i].id & 0x01) << 12)
                            | (((sprite_scanline[i].id & 0xFE) + 1) << 4)
                            | (7 - (_scan_line - sprite_scanline[i].y) & 0x07);
                    }
                    else {
                        sprite_pattern_addr_lo =
                            ((sprite_scanline[i].id & 0x01) << 12)
                            | ((sprite_scanline[i].id & 0xFE) << 4)
                            | (7 - (_scan_line - sprite_scanline[i].y) & 0x07);
                    }
                }
            }

            sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;

            sprite_pattern_bits_lo = read_from_ppu_bus(sprite_pattern_addr_lo, false);
            sprite_pattern_bits_hi = read_from_ppu_bus(sprite_pattern_addr_hi, false);

            if (sprite_scanline[i].attr & 0x40) {
                auto flip_byte = [] (uint8_t b) {
                    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
                    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
                    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
                    return b;
                };

                sprite_pattern_bits_lo = flip_byte(sprite_pattern_bits_lo);
                sprite_pattern_bits_hi = flip_byte(sprite_pattern_bits_hi);
            }

            _sprite_shifter_pattern_lo[i] = sprite_pattern_bits_lo;
            _sprite_shifter_pattern_hi[i] = sprite_pattern_bits_hi;
        }
    }
}

/* Clock */
void ppu2C02::clock() {
    // Render background and foreground
    if (_scan_line >= -1 && _scan_line < 240) {
        _render_bg();
        _render_fg();
    }

    // Set up NMI
    if (_scan_line >= 241 && _scan_line < 261) {
        if (_scan_line == 241 && _cycle == 1) {
            status_register.vertical_blank = 1;
            if (control_register.enable_nmi) _nmi = true;
        }
    }

    // Display background pixels
    uint8_t bg_pixel = 0x00;
    uint8_t bg_palette = 0x00;

    if (mask_register.render_background) {
        uint16_t bit_mux = 0x8000 >> _fine_x;
        uint8_t p0_pixel = (_bg_shifter_pattern_lo & bit_mux) > 0;
        uint8_t p1_pixel = (_bg_shifter_pattern_hi & bit_mux) > 0;
        bg_pixel = (p1_pixel << 1) | p0_pixel;

        uint8_t bg_pal0 = (_bg_shifter_attrib_lo & bit_mux) > 0;
        uint8_t bg_pal1 = (_bg_shifter_attrib_hi & bit_mux) > 0;
        bg_palette = (bg_pal1 << 1) | bg_pal0;
    }

    // Display foreground pixels
    uint8_t fg_pixel = 0x00;
    uint8_t fg_palette = 0x00;
    uint8_t fg_priority = 0x00;

    if (mask_register.render_sprites) {
        _sprite_zero_rendered = false;

        for (uint8_t i = 0; i < _sprite_count; i++) {
            if (sprite_scanline[i].x == 0) {
                uint8_t fg_pixel_lo = (_sprite_shifter_pattern_lo[i] & 0x80) > 0;
                uint8_t fg_pixel_hi = (_sprite_shifter_pattern_hi[i] & 0x80) > 0;
                fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;

                fg_palette  = (sprite_scanline[i].attr & 0x03) + 0x04;
                fg_priority = (sprite_scanline[i].attr & 0x20) == 0;

                if (fg_pixel != 0) {
                    if (i == 0) {
                        _sprite_zero_rendered = true;
                    }

                    break;
                }
            }
        }
    }

    // Updates NES video during visible scan lines
    uint8_t final_pixel = 0x00;
    uint8_t final_palette = 0x00;

    if (bg_pixel == 0 && fg_pixel > 0) {
        final_pixel = fg_pixel;
        final_palette = fg_palette;
    }
    else if (bg_pixel > 0 && fg_pixel == 0) {
        final_pixel = bg_pixel;
        final_palette = bg_palette;
    }
    else if (bg_pixel > 0 && fg_pixel > 0) {
        if (fg_priority) {
            final_pixel = fg_pixel;
            final_palette = fg_palette;
        }
        else {
            final_pixel = bg_pixel;
            final_palette = bg_palette;
        }

        if (_sprite_zero_hit && _sprite_zero_rendered) {
            if (mask_register.render_background & mask_register.render_sprites) {
                if (~(mask_register.render_background_left| mask_register.render_sprites_left)){
                    if (_cycle >= 9 && _cycle < 258) {
                        status_register.sprite_zero_hit = 1;
                    }
                }
                else {
                    if (_cycle >= 1 && _cycle < 258) {
                        status_register.sprite_zero_hit = 1;
                    }

                }
            }
        }
    }

    assert(_video_text);
    SDL_Color color = get_palette_from_offsets(final_pixel, final_palette);

    if (_cycle < 256 && _scan_line < 240 && _scan_line >= 0) {
        _video_text->update_texture(
            (_cycle - 1) + (_scan_line << 8), color.r, color.g, color.b);
    }

    // Increments cycles and scan lines for each clock cycle
    _cycle++;

    if (_cycle >= 341) {
        _cycle = 0;
        _scan_line++;

        if (_scan_line >= 261) {
            _scan_line = -1;
            _frame_completed = true;
        }
    }
}

bool ppu2C02::frame_completed() { return _frame_completed; }

void ppu2C02::reset_frame() { _frame_completed = false; }

void ppu2C02::reset() {
    // Reset buffers
    _address_latch = 0x00;
    _ppu_data_buffer = 0x00;
    _scan_line = 0;
    _cycle = 0;

    // Reset background rendering attributes
    _fine_x                  = 0x00;
    _bg_next_tile_id         = 0x00;
    _bg_next_tile_attrib     = 0x00;
    _bg_next_tile_lsb        = 0x00;
    _bg_next_tile_msb        = 0x00;
    _bg_shifter_pattern_lo   = 0x0000;
    _bg_shifter_pattern_hi   = 0x0000;
    _bg_shifter_attrib_lo    = 0x0000;
    _bg_shifter_attrib_hi    = 0x0000;

    // Reset PPU registers
    status_register.reg     = 0x00;
    mask_register.reg       = 0x00;
    control_register.reg    = 0x00;
    vram_addr.reg           = 0x0000;
    tram_addr.reg           = 0x0000;
}

/*=============================================================================
 * MAIN BUS CONNECTION
 *===========================================================================*/
/* Main bus read */
uint8_t ppu2C02::read_from_main_bus(uint16_t addr, bool read_only) {
    (void) read_only;
    uint8_t data = 0x00;
    assert(addr >= 0x0000 && addr <= 0x0007);

    switch (addr) {
        case 0x0000: // Control register (not readable)
            break;
        case 0x0001: // Mask register (not readable)
            break;
        case 0x0002: // Status register
            // Fetch 3 top bit of status register
            data = (status_register.reg & 0xE0) | (_ppu_data_buffer & 0x1F);
            status_register.vertical_blank &= 0x00;     // Clear vertical blank flag
            _address_latch &= 0x00;                     // Reset latch
            break;
        case 0x0003: // OAM Address
            break;
        case 0x0004: // OAM Data
            data = oam_ptr[oam_addr];
            break;
        case 0x0005: // Scroll register (not readable)
            break;
        case 0x0006: // PPU Address (not readable)
            break;
        case 0x0007: // PPU Data
            data = _ppu_data_buffer;
            _ppu_data_buffer = read_from_ppu_bus(vram_addr.reg, false);

            if (vram_addr.reg >= 0x3F00) data = _ppu_data_buffer;
            vram_addr.reg += (control_register.increment_mode ? 32 : 1);
            break;
    }
    return data;
}

/* Main bus write */
void ppu2C02::write_to_main_bus(uint16_t addr, uint8_t data) {
    assert(addr >= 0x0000 && addr <= 0x0007);
    switch (addr) {
        case 0x0000: // Control register
            control_register.reg = data;
            tram_addr.nametable_x = control_register.nametable_x;
            tram_addr.nametable_y = control_register.nametable_y;
            break;
        case 0x0001: // Mask register
            mask_register.reg = data;
            break;
        case 0x0002: // Status register
            break;
        case 0x0003: // OAM Address
            oam_addr = data;
            break;
        case 0x0004: // OAM Data
            oam_ptr[oam_addr] = data;
            break;
        case 0x0005: // Scroll
            if (_address_latch == 0) {
                _fine_x = data & 0x07;
                tram_addr.coarse_x = data >> 3;
                _address_latch = 1;
            }
            else {
                tram_addr.fine_y = data & 0x07;
                tram_addr.coarse_y = data >> 3;
                _address_latch = 0;
            }
            break;
        case 0x0006: // PPU Address
            // Simulates 2 clock cycles needed to read/write PPU address
            if (_address_latch == 0) {
                tram_addr.reg = (uint16_t) ((data & 0x3F) << 8) | (tram_addr.reg & 0x00FF);
                _address_latch = 1;
            }
            else if (_address_latch == 1) {
                // Transfer tram_addr to vram_addr
                tram_addr.reg = (tram_addr.reg & 0xFF00) | data;
                vram_addr = tram_addr;
                _address_latch = 0;
            }
            break;
        case 0x0007: // PPU Data
            write_to_ppu_bus(vram_addr.reg, data);

            // Writes to PPU automatically increments the name table
            vram_addr.reg += (control_register.increment_mode ? 32 : 1);
            break;
    }
}

/*=============================================================================
 * PPU BUS CONNECTION
 *===========================================================================*/
/* PPU Bus read */
uint8_t ppu2C02::read_from_ppu_bus(uint16_t addr, bool read_only) {
    (void) read_only;
    addr &= 0x3FFF;
    uint8_t data = cartridge->handle_ppu_read(addr);

    if (addr >= NAME_TABLE_ADDR_LOWER && addr <= NAME_TABLE_ADDR_UPPER) {
        addr &= 0x0FFF;
        if (cartridge->mirror == Cartridge::MIRROR::VERTICAL) {
            if (addr >= 0x0000 && addr <= 0x03FF)
                data = ppu_name_table[0][addr & 0x03FF];
            else if (addr >= 0x0400 && addr <= 0x07FF)
                data = ppu_name_table[1][addr & 0x03FF];
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                data = ppu_name_table[0][addr & 0x03FF];
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                data = ppu_name_table[1][addr & 0x03FF];
        }
        else if (cartridge->mirror == Cartridge::MIRROR::HORIZONTAL) {
            if (addr >= 0x0000 && addr <= 0x03FF)
                data = ppu_name_table[0][addr & 0x03FF];
            else if (addr >= 0x0400 && addr <= 0x07FF)
                data = ppu_name_table[0][addr & 0x03FF];
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                data = ppu_name_table[1][addr & 0x03FF];
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                data = ppu_name_table[1][addr & 0x03FF];
        }
    }
    else if (addr >= PALETTE_ADDR_LOWER && addr <= PALETTE_ADDR_UPPER) {
        addr &= 0x001F;
        if (addr == 0x0010)         addr = 0x0000;
        else if (addr == 0x0014)    addr = 0x0004;
        else if (addr == 0x0018)    addr = 0x0008;
        else if (addr == 0x001C)    addr = 0x000C;
        data = ppu_palette_table[addr] & (mask_register.grayscale ? 0x30 : 0x3F);
    }

    return data;
}

/* PPU Bus write */
void ppu2C02::write_to_ppu_bus(uint16_t addr, uint8_t data) {
    addr &= 0x3FFF;
    cartridge->handle_ppu_write(addr, data);

    if (addr >= NAME_TABLE_ADDR_LOWER && addr <= NAME_TABLE_ADDR_UPPER) {
        addr &= 0x0FFF;
        if (cartridge->mirror == Cartridge::MIRROR::VERTICAL) {
            if (addr >= 0x0000 && addr <= 0x03FF)
                ppu_name_table[0][addr & 0x03FF] = data;
            else if (addr >= 0x0400 && addr <= 0x07FF)
                ppu_name_table[1][addr & 0x03FF] = data;
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                ppu_name_table[0][addr & 0x03FF] = data;
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                ppu_name_table[1][addr & 0x03FF] = data;
        }
        else if (cartridge->mirror == Cartridge::MIRROR::HORIZONTAL) {
            if (addr >= 0x0000 && addr <= 0x03FF)
                ppu_name_table[0][addr & 0x03FF] = data;
            else if (addr >= 0x0400 && addr <= 0x07FF)
                ppu_name_table[0][addr & 0x03FF] = data;
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                ppu_name_table[1][addr & 0x03FF] = data;
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                ppu_name_table[1][addr & 0x03FF] = data;
        }
    }
    else if (addr >= PALETTE_ADDR_LOWER && addr <= PALETTE_ADDR_UPPER) {
        addr &= 0x001F;
        if (addr == 0x0010)         addr = 0x0000;
        else if (addr == 0x0014)    addr = 0x0004;
        else if (addr == 0x0018)    addr = 0x0008;
        else if (addr == 0x001C)    addr = 0x000C;
        ppu_palette_table[addr] = data;
    }
}

/* NMI */
bool ppu2C02::nmi() { return _nmi; }

void ppu2C02::reset_nmi() { _nmi = false; }
