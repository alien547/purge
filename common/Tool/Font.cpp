#include <sstream>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <SDL.h>
#include "Random.h"
#include "String.h"
#include "Render.h"
#include "Utf8.h"
#include "Settings.h"
#include "Font.h"
using namespace std;
using namespace Settings;

const int ATLAS_SIZE=256;

SDL_Texture* char_atlas=nullptr;
TTF_Font* font=nullptr, * small_font=nullptr;

SDL_Rect char_rects[128];
unordered_map<std::string, SDL_Texture*> text_cache;

int text_anomaly_level=0;

void init_font(const string& path){
    TTF_Init();
    font=TTF_OpenFont(path.c_str(), FONT_SIZE);
    small_font=TTF_OpenFont(path.c_str(), SMALL_FONT_SIZE);
    SDL_StopTextInput();
}

void cleanup_font(){
    TTF_CloseFont(font);
    TTF_CloseFont(small_font);
    TTF_Quit();
    for(auto& t : text_cache)SDL_DestroyTexture(t.second);
    SDL_DestroyTexture(char_atlas);
}

void init_char_atlas(){
    SDL_Surface* atlas_surf=SDL_CreateRGBSurfaceWithFormat(0, ATLAS_SIZE, ATLAS_SIZE, 32, SDL_PIXELFORMAT_RGBA8888);
    SDL_FillRect(atlas_surf, NULL, SDL_MapRGBA(atlas_surf->format, 0, 0, 0, 0));
    int x=0, y=0, row_height=0;
    for(int i=32; i<127; ++i){//½öäÖÈ¾¿É¼û×Ö·û
        char str[2]={char(i), '\0'};
        SDL_Surface* char_surf=TTF_RenderUTF8_Blended(font, str, {255, 255, 255, 255});
        if(x+char_surf->w>ATLAS_SIZE){
            x=0;
            y+=row_height;
            row_height=0;
        }
        SDL_Rect dst={x, y, char_surf->w, char_surf->h};
        SDL_BlitSurface(char_surf, NULL, atlas_surf, &dst);
        char_rects[i]=dst;
        x+=char_surf->w;
        row_height=max(row_height, char_surf->h);
        SDL_FreeSurface(char_surf);
    }
    char_atlas=SDL_CreateTextureFromSurface(renderer, atlas_surf);
    SDL_FreeSurface(atlas_surf);
}

void apply_text_anomaly(string& str, int seed){
    if(str.empty())return;
    vector<string> ch;
    for(int i=0; i<str.size(); ){
        int l=utf8_char_len((unsigned char)(str[i]));
        ch.emplace_back(str, i, l);
        i+=l;
    }
    int I=seed&0xFF;
    int del=I*5/255, dup=I*6/255, ins=I*3/255, swp=I*8/255;
    static const vector<string> noise_chars=[]{
        string s="!@#$%^&*()_+{}|:<>?~ £¬¡££¡£¿¡¢£»";
        vector<string> result;
        for(int i=0; i<s.size(); ){
            int l=utf8_char_len((unsigned char)(s[i]));
            result.push_back(s.substr(i,l));
            i+=l;
        }
        return result;
    }();
    for(int i=0; i<ch.size(); ++i){
        if(random(0, 100)<del){
            ch.erase(ch.begin()+i--);
            continue;
        }
        if(random(0, 100)<dup){
            ch.insert(ch.begin()+i++, ch[i]);
        }
        if(random(0, 100)<ins){
            ch.insert(ch.begin()+i++, noise_chars[random(0, noise_chars.size()-1)]);
        }
    }
    if(ch.size()>=2&&random(0, 100)<swp)swap(ch[random(0, ch.size()-1)], ch[random(0, ch.size()-1)]);
    str.clear();
    for(const string& c : ch)str+=c;
}

SDL_Texture* create_text_texture(const string& text, bool cache, bool small){
    string key=text+"_"+(small?"small":"normal");
    if(cache){
        auto it=text_cache.find(key);
        if(it!=text_cache.end())return it->second;
    }
    SDL_Surface* surface=TTF_RenderUTF8_Blended(small?small_font:font, text.c_str(), {255, 255, 255, 255});
    SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if(cache)text_cache[key]=texture;
    return texture;
}

int draw_text(int x, int y, const string& text, const DrawTextOptions& opts){
    int line_height=TTF_FontHeight(opts.small?small_font:font), max_width=0;
    istringstream stream(text);
    string line;
    while(getline(stream, line)){
        if(!line.empty()){
            string str=line, cache_key=to_string(x)+"|"+to_string(y)+"|"+line+"|"+to_string(text_anomaly_level);
            static unordered_map<string, string> anomaly_level_cache;
            static Uint32 last_update_anomaly=SDL_GetTicks(), last_reset_anomaly=SDL_GetTicks();
            Uint32 now=SDL_GetTicks();
            if(now-last_reset_anomaly>20000){
                last_reset_anomaly=now;
                anomaly_level_cache.clear();
            }
            if(now-last_update_anomaly>1500+random(-300, 300)||anomaly_level_cache.find(cache_key)==anomaly_level_cache.end()){
                last_update_anomaly=now;
                for(int i=0; i<text_anomaly_level; ++i){
                    apply_text_anomaly(str, random(0, 100000));
                }
                anomaly_level_cache[cache_key]=str;
            }else{
                str=anomaly_level_cache[cache_key];
            }
            SDL_Texture* texture=create_text_texture(str, opts.cache, opts.small);
            int w, h;
            SDL_QueryTexture(texture, NULL, NULL, &w, &h);
            max_width=max(w, max_width);
            int draw_x=x+(opts.center?-w/2:0), draw_y=y+(opts.center?-h/2:0);
            SDL_SetTextureColorMod(texture, 0, 0, 0);
            SDL_SetTextureAlphaMod(texture, 100);
            SDL_Rect dst1={draw_x+2, draw_y+2, w, h};
            SDL_RenderCopy(renderer, texture, NULL, &dst1);
            SDL_SetTextureAlphaMod(texture, 200);
            SDL_Rect dst2={draw_x+1, draw_y+1, w, h};
            SDL_RenderCopy(renderer, texture, NULL, &dst2);
            SDL_SetTextureColorMod(texture, opts.color.r, opts.color.g, opts.color.b);
            SDL_SetTextureAlphaMod(texture, opts.color.a);
            SDL_Rect dst3={draw_x, draw_y, w, h};
            SDL_RenderCopy(renderer, texture, NULL, &dst3);
            if(!opts.cache)SDL_DestroyTexture(texture);
        }
        y+=line_height;
    }
    return max_width;
}

void draw_char(int x, int y, char c, SDL_Color color, bool center){
    int idx=(unsigned char)(c);
    if(idx<0||idx>=128)return;
    SDL_Rect& src=char_rects[idx];
    if(src.w==0||src.h==0)return;
    for(int i=0; i<text_anomaly_level; ++i){
        x+=random(-5, 5)/5;
        y+=random(-5, 5)/5;
    }
    SDL_SetTextureColorMod(char_atlas, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(char_atlas, color.a);
    SDL_Rect dst={x, y, src.w, src.h};
    if(center){
        dst.x-=dst.w/2;
        dst.y-=dst.h/2;
    }
    SDL_RenderCopy(renderer, char_atlas, &src, &dst);
}

void draw_char_shadow(int x, int y, char c, float dir_x, float dir_y, float shadow_len, SDL_Color color, bool center){
    int idx=(unsigned char)c;
    if(idx<0||idx>=128)return;
    SDL_Rect& src=char_rects[idx];
    if(src.w==0||src.h==0)return;
    float w=float(src.w), h=float(src.h);
    if(center){
        x-=int(w/2);
        y-=int(h/2);
    }
    float hw=w*0.5f, hh=h*0.5f;
    float u0=float(src.x)/ATLAS_SIZE, v0=float(src.y)/ATLAS_SIZE, u1=float(src.x+src.w)/ATLAS_SIZE, v1=float(src.y+src.h)/ATLAS_SIZE;
    float cx=x+hw, cy=y+hh;
    float length_px=shadow_len*FONT_SIZE;
    float near_proj=hw*fabs(dir_x)+hh*fabs(dir_y);
    float denom=2.0f*near_proj+0.001f;
    float corners[4][2]={{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
    float uvs[4][2]={{u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}};

    SDL_Vertex verts[4];
    float projs[4];
    for(int i=0; i<4; ++i){
        float ox=corners[i][0], oy=corners[i][1];
        float proj=ox*dir_x+oy*dir_y;
        float t=(near_proj-proj)/denom;
        float offset=length_px*(1.0f-t);
        float nx=ox+offset*dir_x, ny=oy+offset*dir_y;
        verts[i].position={cx+nx, cy+ny};
        verts[i].tex_coord={uvs[i][0], uvs[i][1]};
        Uint8 a=Uint8(color.a*t);
        verts[i].color={color.r, color.g, color.b, a};
    }
    const int indices[6]={0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(renderer, char_atlas, verts, 4, indices, 6);
}

