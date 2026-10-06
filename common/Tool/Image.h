#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <SDL_image.h>

void init_image(const std::string& path);
void cleanup_image();
void create_background_texture(SDL_Color top_color={45, 40, 40, 255}, SDL_Color bottom_color={85, 80, 80, 255}, float horizon=0.55f);
void create_rounded_rect_texture();
void create_noise_texture();
void create_stun_texture();
void create_light_orb_texture();
void create_vignette_texture();
void draw_background_texture();
void draw_rounded_rect_texture(int x, int y, int width, int height, Uint8 alpha);
void draw_noise_texture(Uint8 alpha);
void draw_stun_texture(int x, int y, SDL_Color color);
void draw_light_orb_texture(int cx, int cy, int radius_px, SDL_Color color);
void draw_vignette(Uint8 alpha);
void draw_circle_outline(int cx, int cy, int radius);
void draw_image(const std::string& name, int x, int y, bool center=false);

#endif

