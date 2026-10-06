#include <string>
#include <vector>
#include <cmath>
#include <functional>
#include <SDL.h>
#include "Render.h"
#include "Font.h"
#include "Image.h"
#include "TextInput.h"
#include "String.h"
#include "Settings.h"
#include "Audio.h"
#include "Form.h"
using namespace std;
using namespace Settings;

Form::Form(const string& t):title(t){}

Form& Form::add(const std::string& label, short type, const std::string& value, float low, float high, std::vector<std::string> options, int select){
    FormField f;
    f.label=label;
    f.type=type;
    f.value=value;
    f.low=low;
    f.high=high;
    f.options=options;
    f.select=select;
    f.def=value;
    fields.push_back(f);
    return *this;
}

string Form::get(const string& l)const{
    for(const FormField& f : fields){
        if(f.label==l)return f.value;
    }
    return "";
}

int Form::int_get(const string& l)const{
    return safe_stoi(get(l));
}

float Form::float_get(const string& l)const{
    return safe_stof(get(l));
}

bool Form::check_get(const string& l)const{
    return get(l)=="1";
}

int Form::idx(const string& l)const{
    for(const FormField& f : fields){
        if(f.label==l)return f.select;
    }
    return 0;
}

void Form::reset(){
    for(FormField& f : fields){
        f.value=f.def;
        f.err.clear();
    }
}

bool Form::validate(FormField& f){
    bool ok=true;
    f.err.clear();
    if(f.type==FORM_TEXT&&f.value.empty()){
        f.err="不能为空";
        ok=false;
    }
    if(f.type==FORM_NUM){
        float v=safe_stof(f.value, NAN);
        if(isnan(v)){
            f.err="不是数字";
            ok=false;
        }else if(v<f.low||v>f.high){
            f.err="超出范围";
            ok=false;
        }
    }
    return ok;
}

bool Form::run(int x, int y){
    const int rh=28, lw=120;
    int max_vis=(SCREEN_HEIGHT-y-100)/rh;
    int total=fields.size()+1;
    SDL_Event e;
    while(true){
        while(SDL_PollEvent(&e)){
            switch(e.type){
                case SDL_QUIT:
                    return false;
                case SDL_KEYDOWN:
                    switch(e.key.keysym.sym){
                        case SDLK_ESCAPE:
                            return false;
                        case SDLK_TAB:
                            focus=(focus+(e.key.keysym.mod&KMOD_SHIFT?-1:1)+total)%total;
                            break;
                        case SDLK_UP:
                            focus=(focus-1+total)%total;
                            break;
                        case SDLK_DOWN:
                            focus=(focus+1)%total;
                            break;
                        case SDLK_RETURN:
                            if(focus==fields.size()){
                                on_submit(*this);
                                return true;
                            }
                            FormField& f=fields[focus];
                            if(f.type==FORM_CHECK){
                                f.value=f.value=="1"?"0":"1";
                            }else if(f.type==FORM_SELECT&&!f.options.empty()){
                                f.select=(f.select+1)%f.options.size();
                                f.value=f.options[f.select];
                            }else{
                                int ry=y+focus*rh;
                                string v=read_input(x+lw, ry, 30, f.value);
                                string last_value=f.value;
                                f.value=v;
                                if(!validate(f)){
                                    beep(SOUND_ASK);
                                    f.value=last_value;
                                }
                            }
                            break;
                    }
            }
        }

        clear_renderer();
        draw_background_texture();
        draw_text(x, y-40, title);
        for(int i=0; i<fields.size()&&i<max_vis; ++i){
            FormField& f=fields[i];
            int ry=y+i*rh;
            bool fc=(i==focus);
            DrawTextOptions opts;
            opts.cache=true;
            opts.color=fc?SDL_Color{255, 255, 0, 255}:SDL_Color{220, 220, 220, 255};
            draw_text(x, ry, f.label+":", opts);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);
            SDL_Rect box={x+lw, ry+1, 30*FONT_SIZE, 22};
            SDL_RenderFillRect(renderer, &box);
            SDL_SetRenderDrawColor(renderer, fc?255:100, fc?200:100, 0, 200);
            SDL_RenderDrawRect(renderer, &box);
            string v=f.value;
            if(f.type==FORM_CHECK){
                v=f.value=="1"?"[是]":"[否]";
            }
            draw_text(x+lw+6, ry, v);
            if(!f.err.empty()){
                DrawTextOptions opts;
                opts.color={255, 80, 80, 255};
                opts.small=true;
                draw_text(x+lw+6, ry+16, f.err, opts);
            }
        }
        int vis=(fields.size()<max_vis)?fields.size():max_vis;
        int by=y+vis*rh+10;
        DrawTextOptions opts;
        opts.color=(focus==fields.size())?SDL_Color{255, 255, 0, 255}:SDL_Color{200, 200, 200, 255};
        draw_text(x, by, "[ 提交 ]", opts);
        SDL_RenderPresent(renderer);
        SDL_Delay(SHORT_TIME);
    }
}

