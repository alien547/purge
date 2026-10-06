#ifndef FONT_H
#define FONT_H

#include <string>
#include <SDL_ttf.h>

struct DrawTextOptions{
    SDL_Color color={255, 255, 255, 255};
    bool cache=false, small=false, center=false;
};

extern int text_anomaly_level;

void init_font(const std::string& path);
void cleanup_font();
void init_char_atlas();
void apply_text_anomaly(std::string& input, int seed);
SDL_Texture* create_text_texture(const std::string& text, bool cache, bool small);
int draw_text(int x, int y, const std::string& text, const DrawTextOptions& opts=DrawTextOptions());
void draw_char(int x, int y, char c, SDL_Color color={255, 255, 255, 255}, bool center=false);
void draw_char_shadow(int x, int y, char c, float dir_x, float dir_y, float shadow_len, SDL_Color color={255, 255, 255, 255}, bool center=false);

#endif

