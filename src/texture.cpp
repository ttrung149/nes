#include "texture.h"

/* Constructor */
Texture::Texture(SDL_Renderer *_renderer, uint16_t w, uint16_t h, const SDL_Rect &_bound) :
    width(w), height(h), renderer(_renderer), boundaries(_bound)
{
    assert(_renderer);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                          SDL_TEXTUREACCESS_STREAMING, w, h); 
    pixels_arr.resize(width * height * 4, 0x1F);
}

/* Destructor */
Texture::~Texture() { assert(texture); SDL_DestroyTexture(texture); }

/* Update texture */
void Texture::update_texture(uint16_t idx, uint8_t r, uint8_t g, uint8_t b) {
    assert(idx < width * height);

    pixels_arr[(idx << 2)    ] = r;
    pixels_arr[(idx << 2) + 1] = g;
    pixels_arr[(idx << 2) + 2] = b;
    pixels_arr[(idx << 2) + 3] = SDL_ALPHA_OPAQUE;
}

/* Render texture */
void Texture::render_texture() {
    assert(texture);
    SDL_UpdateTexture(texture, nullptr, pixels_arr.data(), width * 4);

    assert(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, &boundaries);
}
