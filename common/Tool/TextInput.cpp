#include <deque>
#include <cmath>
#include <SDL.h>
#include "Settings.h"
#include "Render.h"
#include "Font.h"
#include "Utf8.h"
#include "Blur.h"
#include "Image.h"
#include "TextInput.h"
using namespace std;
using namespace Settings;

string read_input(int x, int y, int max_len, const string& default_value, function<bool(SDL_KeyboardEvent&, string&)> on_keydown, bool blurred_background){
    string input;
    deque<string> history_input;
    const int MAX_HISTORY_SIZE=50;
    bool done=false;
    int change_input_cursor=0;
    SDL_StartTextInput();
    SDL_Surface* snapshot=nullptr;
    SDL_Texture* clear_bg=nullptr, * blurred_bg=nullptr;
    if(blurred_background){
        snapshot=SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888);
        SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA8888, snapshot->pixels, snapshot->pitch);
        clear_bg=SDL_CreateTextureFromSurface(renderer, snapshot);
        blurred_bg=create_blurred_texture(snapshot, 1.0f);
        SDL_FreeSurface(snapshot);
    }
    Uint32 start_time=SDL_GetTicks();
    SDL_Event e;
    while(!done){
        while(SDL_PollEvent(&e)){
            bool back=false;
            string old_input=input;
            switch(e.type){
                case SDL_QUIT:
                    done=true;
                    break;
                case SDL_KEYDOWN:
                    if(on_keydown){
                        string result;
                        if(on_keydown(e.key, result)){
                            input=result;
                            done=true;
                            break;
                        }
                    }
                    if(e.key.keysym.sym==SDLK_BACKSPACE)pop_back_utf8(input);
                    if(e.key.repeat!=0)break;
                    switch(e.key.keysym.sym){
                        case SDLK_ESCAPE:
                            done=true;
                            break;
                        case SDLK_RETURN:
                            done=true;
                            break;
                        case SDLK_v:
                            if(e.key.keysym.mod&KMOD_CTRL){
                                char* clip=SDL_GetClipboardText();
                                if(clip){
                                    input+=clip;
                                    while(int(input.size())>max_len)pop_back_utf8(input);
                                    filter_special_chars(input);
                                    SDL_free(clip);
                                }
                            }
                            break;
                        case SDLK_z:
                            if(e.key.keysym.mod&KMOD_CTRL&&!history_input.empty()){
                                input=history_input.back();
                                history_input.pop_back();
                                back=true;
                            }
                            break;
                    }
                    break;
                case SDL_TEXTINPUT:
                    input+=e.text.text;
                    while(int(input.size())>max_len)pop_back_utf8(input);
                    filter_special_chars(input);
                    break;
            }
            if(!back&&old_input!=input){
                if(history_input.size()>=MAX_HISTORY_SIZE)history_input.pop_front();
                history_input.push_back(old_input);
            }
        }
        Uint32 elapsed=SDL_GetTicks()-start_time;
        float breath=fabs(cosf(elapsed*0.0005f))*0.6f;
        if(blurred_background){
            SDL_RenderCopy(renderer, blurred_bg, NULL, NULL);
            SDL_SetTextureAlphaMod(clear_bg, Uint8(breath*255));
            SDL_RenderCopy(renderer, clear_bg, NULL, NULL);
        }else{
            draw_background_texture();
        }

        SDL_Rect rect={x, y, max_len*FONT_SIZE, 24};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 40, 40, 50, 80);
        SDL_RenderDrawRect(renderer, &rect);
        SDL_Rect inner_rect={rect.x+1, rect.y+1, rect.w-2, rect.h-2};
        SDL_SetRenderDrawColor(renderer, 220, 220, 255, 30);
        SDL_RenderDrawRect(renderer, &inner_rect);

        if(change_input_cursor==-20)change_input_cursor=20;
        draw_text(x+5, y, input+(--change_input_cursor<0?"_":""));
        SDL_RenderPresent(renderer);
        SDL_Delay(SHORT_TIME);
    }
    SDL_SetTextureAlphaMod(clear_bg, 255);
    SDL_RenderCopy(renderer, clear_bg, NULL, NULL);
    draw_text(x, y, input);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(clear_bg);
    SDL_DestroyTexture(blurred_bg);
    SDL_StopTextInput();
    if(input.empty())return default_value;
    return input;
}

int8_t prompt_yes_no(){
    int8_t choice=-1;
    bool done=false;
    SDL_Event e;
    while(!done){
        while(SDL_PollEvent(&e)){
            switch(e.type){
                case SDL_QUIT:
                    done=true;
                    break;
                case SDL_KEYDOWN:
                    if(e.key.repeat!=0)break;
                    switch(e.key.keysym.sym){
                        case SDLK_ESCAPE:
                            done=true;
                            break;
                        case SDLK_y:
                        case SDLK_n:
                            choice=e.key.keysym.sym==SDLK_y;
                            done=true;
                            break;
                    }
                    break;
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(SHORT_TIME);
    }
    return choice;
}

int prompt_choice(int choice_size){
    int choice=-1;
    bool done=false;
    SDL_Event e;
    while(!done){
        while(SDL_PollEvent(&e)){
            switch(e.type){
                case SDL_QUIT:
                    done=true;
                    break;
                case SDL_KEYDOWN:
                    if(e.key.repeat!=0)break;
                    if(e.key.keysym.sym==SDLK_ESCAPE)done=true;
                    for(int i=0; i<choice_size; ++i){
                        if(e.key.keysym.sym==SDLK_1+i){
                            choice=i+1;
                            done=true;
                        }
                    }
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(SHORT_TIME);
    }
    return choice;
}

