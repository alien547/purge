#include "Settings.h"
#include "Render.h"
using namespace std;
using namespace Settings;

SDL_Window* window=nullptr;
SDL_Renderer* renderer=nullptr;

bool is_foreground=true;

void init_render(const string& name){
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
    window=SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_HIDDEN);
    renderer=SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE|SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void cleanup_render(){
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool update_foreground(SDL_Event& e){
    bool old_state=is_foreground;
    switch(e.window.event){
        case SDL_WINDOWEVENT_RESTORED:
        case SDL_WINDOWEVENT_SHOWN:
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            is_foreground=true;
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
        case SDL_WINDOWEVENT_HIDDEN:
        case SDL_WINDOWEVENT_FOCUS_LOST:
            is_foreground=false;
            break;
    }
    return old_state!=is_foreground;
}

bool color_equal(const SDL_Color& a, const SDL_Color& b){
    return a.r==b.r&&a.g==b.g&&a.b==b.b&&a.a==b.a;
}

void clear_renderer(){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

