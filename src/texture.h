#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <SDL2/SDL.h>
#include <inttypes.h>
#include <vector>

class Texture {
public:
    Texture(SDL_Renderer *_renderer, uint16_t w, uint16_t h, const SDL_Rect &_boundaries);
    ~Texture();

    void update_texture(uint16_t idx, uint8_t r, uint8_t g, uint8_t b);
    void render_texture();

private:
    uint16_t width, height;

    SDL_Renderer *renderer;
    SDL_Rect boundaries;
    SDL_Texture *texture;
    std::vector<uint8_t> pixels_arr;
};

#endif
