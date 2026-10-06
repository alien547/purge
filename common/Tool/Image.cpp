#include <unordered_map>
#include <cmath>
#include <vector>
#include <SDL.h>
#include "Settings.h"
#include "Random.h"
#include "Render.h"
#include "Image.h"
using namespace std;
using namespace Settings;

unordered_map<string, SDL_Texture*> image_cache;
SDL_Texture* background_texture=nullptr, * rounded_rect_texture=nullptr, * noise_texture=nullptr, * stun_texture=nullptr, * light_orb_texture=nullptr, * vignette_texture=nullptr;

string image_path;
long long background_star_id=0;
float background_horizon=0.0f;

void init_image(const string& path){
    image_path=path;
    IMG_Init(IMG_INIT_PNG);
}

void cleanup_image(){
    for(auto& i : image_cache)SDL_DestroyTexture(i.second);
    SDL_DestroyTexture(background_texture);
    SDL_DestroyTexture(rounded_rect_texture);
    SDL_DestroyTexture(noise_texture);
    SDL_DestroyTexture(stun_texture);
    SDL_DestroyTexture(light_orb_texture);
    SDL_DestroyTexture(vignette_texture);
    IMG_Quit();
}

void create_background_texture(SDL_Color top_color, SDL_Color bottom_color, float horizon){
    if(background_texture)SDL_DestroyTexture(background_texture);
    background_horizon=horizon;
    SDL_Surface* surf=SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888);
    Uint32* pixels=(Uint32*)(surf->pixels);
    for(int y=0; y<SCREEN_HEIGHT; ++y){
        float t=float(y)/SCREEN_HEIGHT;
        float curve=powf(t, 0.8f);
        Uint8 r=Uint8(top_color.r+(bottom_color.r-top_color.r)*curve);
        Uint8 g=Uint8(top_color.g+(bottom_color.g-top_color.g)*curve);
        Uint8 b=Uint8(top_color.b+(bottom_color.b-top_color.b)*curve);
        float dist_to_horizon=fabsf(t-background_horizon);
        if(dist_to_horizon<0.15f){
            float glow=1.0f-(dist_to_horizon/0.15f);
            glow=glow*glow*40;
            r=Uint8(min(255, r+int(glow*1.5f)));
            g=Uint8(min(255, g+int(glow*0.8f)));
            b=Uint8(min(255, b+int(glow*0.2f)));
        }
        for(int x=0; x<SCREEN_WIDTH; ++x)pixels[y*SCREEN_WIDTH+x]=SDL_MapRGBA(surf->format, r, g, b, 255);
    }
    SDL_Texture* background=SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    SDL_SetTextureBlendMode(background, SDL_BLENDMODE_NONE);
    background_texture=background;
    background_star_id=random(0, 999999);
}

void create_rounded_rect_texture(){
    int width=SCREEN_WIDTH, height=SCREEN_HEIGHT, radius=30;
    SDL_Surface* surf=SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
    SDL_Color color={80, 80, 80, 0};
    SDL_FillRect(surf, NULL, SDL_MapRGBA(surf->format, 0, 0, 0, 0));
    Uint32* pixels=(Uint32*)(surf->pixels);
    int pitch=surf->pitch/4;
    Uint32 col=SDL_MapRGBA(surf->format, color.r, color.g, color.b, color.a);
    for(int y=0; y<height; ++y){
        for(int x=0; x<width; ++x){
            bool in_corner=true;
            int dx=0, dy=0;
            if(x<radius&&y<radius){
                dx=radius-x;
                dy=radius-y;
            }else if(x>=width-radius&&y<radius){
                dx=x-(width-radius);
                dy=radius-y;
            }else if(x<radius&&y>=height-radius){
                dx=radius-x;
                dy=y-(height-radius);
            }else if(x>=width-radius&&y>=height-radius){
                dx=x-(width-radius);
                dy=y-(height-radius);
            }else{
                in_corner=false;
            }
            Uint8 alpha=0;
            if(in_corner){
                float dist=sqrtf(dx*dx+dy*dy);
                if(dist<=radius-1){
                    alpha=255;
                }else if(dist<=radius+1){
                    float t=(dist-(radius-1))/2.0f;
                    alpha=(Uint8)((1.0f-t)*255);
                }else{
                    alpha=0;
                }
            }else if((radius<=x&&x<width-radius&&0<=y&&y<height)||(radius<=y&&y<height-radius&&0<=x&&x<width)){
                alpha=255;
            }
            if(alpha>0){
                Uint32 new_col=col|alpha;
                pixels[y*pitch+x]=new_col;
            }
        }
    }
    SDL_Texture* tex=SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    rounded_rect_texture=tex;
}

void create_noise_texture(){
    int width=SCREEN_WIDTH, height=SCREEN_HEIGHT;
    SDL_Surface* surf=SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
    Uint32* pixels=(Uint32*)(surf->pixels);
    for(int i=0; i<width*height; ++i){
        Uint8 val=random(64, 192);
        pixels[i]=SDL_MapRGBA(surf->format, val, val, val, 255);
    }
    SDL_Texture* tex=SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    noise_texture=tex;
}

void create_stun_texture(){
    SDL_Surface* surf=SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888);
    Uint32* px=(Uint32*)(surf->pixels);
    int cx=SCREEN_WIDTH/2, cy=SCREEN_HEIGHT/2;
    float maxr=sqrtf(float(cx)*cx+float(cy)*cy);
    for(int y=0; y<SCREEN_HEIGHT; ++y){
        for(int x=0; x<SCREEN_WIDTH; ++x){
            float dx=x-cx, dy=y-cy;
            float d=sqrtf(dx*dx+dy*dy)/maxr;
            Uint8 a=Uint8(Math::clamp((d-0.2f)*(d-0.2f)*320.0f, 0.0f, 255.0f));
            px[y*SCREEN_WIDTH+x]=SDL_MapRGBA(surf->format, 255, 60, 255, a);
        }
    }
    SDL_Texture* tex=SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    stun_texture=tex;
}

void create_light_orb_texture(){
    int size=max(SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_Surface* surf=SDL_CreateRGBSurfaceWithFormat(0, size, size, 32, SDL_PIXELFORMAT_RGBA8888);
    Uint32* px=(Uint32*)(surf->pixels);
    float half=size*0.5f;
    for(int y=0; y<size; ++y){
        for(int x=0; x<size; ++x){
            float dx=(x+0.5f)-half, dy=(y+0.5f)-half;
            float d=sqrtf(dx*dx+dy*dy)/half;
            float a=expf(-d*d*3.5f);
            a=powf(a, 1.3f);
            px[y*size+x]=SDL_MapRGBA(surf->format, 255, 255, 255, Uint8(a*255));
        }
    }
    SDL_Texture* tex=SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    light_orb_texture=tex;
}

void create_vignette_texture(){
    SDL_Surface* surf=SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888);
    Uint32* px=(Uint32*)(surf->pixels);
    int cx=SCREEN_WIDTH/2, cy=SCREEN_HEIGHT/2;
    float maxr=sqrtf(float(cx)*cx+float(cy)*cy);
    for(int y=0; y<SCREEN_HEIGHT; ++y){
        for(int x=0; x<SCREEN_WIDTH; ++x){
            float dx=x-cx, dy=y-cy;
            float d=sqrtf(dx*dx+dy*dy)/maxr;
            float v=Math::clamp((d-0.35f)/0.65f, 0.0f, 1.0f);
            px[y*SCREEN_WIDTH+x]=SDL_MapRGBA(surf->format, 0, 0, 0, Uint8(v*v*255));
        }
    }
    SDL_Texture* tex=SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    vignette_texture=tex;
}

void draw_background_texture(){
    SDL_RenderCopy(renderer, background_texture, NULL, NULL);
    Uint32 ticks=SDL_GetTicks();
    for(long long i=background_star_id; i<background_star_id+35; ++i){
        int x=(i*137+i*i*13)%SCREEN_WIDTH, y=(i*251+i*i*17)%int(SCREEN_HEIGHT*background_horizon);
        float brightness=0.5f*(sinf(ticks*0.001f*1.1f+i*1.3f)+1.0f);
        Uint8 alpha=Uint8(max(brightness*255-10, 0.0f));
        SDL_SetRenderDrawColor(renderer, 255, 255, 200, alpha);
        SDL_RenderDrawPoint(renderer, x, y);
    }
}

void draw_rounded_rect_texture(int x, int y, int width, int height, Uint8 alpha){
    if(!rounded_rect_texture)create_rounded_rect_texture();
    SDL_SetTextureAlphaMod(rounded_rect_texture, alpha);
    SDL_Rect rect={x, y, width, height};
    SDL_RenderCopy(renderer, rounded_rect_texture, NULL, &rect);
}

void draw_noise_texture(Uint8 alpha){
    if(!noise_texture)create_noise_texture();
    SDL_SetTextureAlphaMod(noise_texture, alpha);
    SDL_RenderCopy(renderer, noise_texture, NULL, NULL);
}

void draw_stun_texture(int x, int y, SDL_Color color){
    if(!stun_texture)create_stun_texture();
    SDL_SetTextureAlphaMod(stun_texture, color.a);
    SDL_SetTextureColorMod(stun_texture, color.r, color.g, color.b);
    SDL_Rect rect={x, y, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, stun_texture, NULL, &rect);
}

void draw_light_orb_texture(int cx, int cy, int radius_px, SDL_Color color){
    if(!light_orb_texture)create_light_orb_texture();
    SDL_SetTextureColorMod(light_orb_texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(light_orb_texture, color.a);
    SDL_Rect dst={cx-radius_px, cy-radius_px, radius_px*2, radius_px*2};
    SDL_RenderCopy(renderer, light_orb_texture, NULL, &dst);
}

void draw_vignette(Uint8 alpha){
    if(!vignette_texture)create_vignette_texture();
    SDL_SetTextureAlphaMod(vignette_texture, alpha);
    SDL_RenderCopy(renderer, vignette_texture, NULL, NULL);
}

void draw_circle_outline(int cx, int cy, int radius){
    if(radius<=0)return;
    int x=radius, y=0, err=1-x;
    while(x>=y){
        SDL_RenderDrawPoint(renderer, cx+x, cy+y);
        SDL_RenderDrawPoint(renderer, cx+y, cy+x);
        SDL_RenderDrawPoint(renderer, cx-y, cy+x);
        SDL_RenderDrawPoint(renderer, cx-x, cy+y);
        SDL_RenderDrawPoint(renderer, cx-x, cy-y);
        SDL_RenderDrawPoint(renderer, cx-y, cy-x);
        SDL_RenderDrawPoint(renderer, cx+y, cy-x);
        SDL_RenderDrawPoint(renderer, cx+x, cy-y);
        ++y;
        if(err<0){
            err+=2*y+1;
        }else{
            --x;
            err+=2*(y-x)+1;
        }
    }
}

void draw_image(const string& name, int x, int y, bool center){
    SDL_Texture* tex;
    auto it=image_cache.find(name);
    if(it==image_cache.end()){
        tex=IMG_LoadTexture(renderer, (image_path+name+".png").c_str());
        image_cache[name]=tex;
    }else{
        tex=it->second;
    }
    int w, h;
    SDL_QueryTexture(tex, NULL, NULL, &w, &h);
    int draw_x=x+(center?-w/2:0), draw_y=y+(center?-h/2:0);
    SDL_Rect dst={draw_x, draw_y, w, h};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
}

