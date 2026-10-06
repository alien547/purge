#ifndef RENDER_H
#define RENDER_H

#include <string>
#include <SDL.h>

extern SDL_Window* window;
extern SDL_Renderer* renderer;

extern bool is_foreground;

void init_render(const std::string& name);
void cleanup_render();
bool update_foreground(SDL_Event& e);
bool color_equal(const SDL_Color& a, const SDL_Color& b);
void clear_renderer();

#endif

