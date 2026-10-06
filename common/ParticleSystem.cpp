#include <cmath>
#include <SDL.h>
#include "Tool/Random.h"
#include "Tool/Render.h"
#include "Settings.h"
#include "ParticleSystem.h"
using namespace std;
using namespace Settings;

ParticleSystem::ParticleSystem(){
    free_list.reserve(MAX_PARTICLE_SIZE);
    for(int i=0; i<MAX_PARTICLE_SIZE; ++i){
        particles[i].active=false;
        free_list.push_back(i);
    }
}

void ParticleSystem::update(int max_x, int max_y){
    for(int i=0; i<MAX_PARTICLE_SIZE; ++i){
        Particle& p=particles[i];
        if(p.active==false)continue;
        p.x+=p.v_x;
        p.y+=p.v_y;
        p.life-=0.4f;
        if(p.x<0||p.x>=max_x||p.y<0||p.y>=max_y||p.life<=0){
            p.active=false;
            free_list.push_back(i);
        }
    }
}

void ParticleSystem::render(float view_x, float view_y){
    for(int i=0; i<MAX_PARTICLE_SIZE; ++i){
        Particle& p=particles[i];
        if(p.active==false)continue;
        int sx=lround((p.x-view_x)*FONT_SIZE), sy=lround((p.y-view_y)*FONT_SIZE);
        SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b, int(p.life/p.max_life*255));
        SDL_Rect rect={int(sx-p.size/2), int(sy-p.size/2), int(p.size), int(p.size)};
        SDL_RenderFillRect(renderer, &rect);
    }
}

void ParticleSystem::emit_explo(float x, float y, pair<int, int> speed_clamp, pair<int, int> max_life_clamp, pair<int, int> size_clamp, int count, ColorRGB color){
    while(--count>=0&&!free_list.empty()){
        int idx=free_list.back();
        free_list.pop_back();
        Particle& p=particles[idx];
        p.x=x;
        p.y=y;
        float angle=randomf(0, 1)*2.0f*PI, speed=random(speed_clamp.first, speed_clamp.second)*0.035f;
        p.v_x=cosf(angle)*speed;
        p.v_y=sinf(angle)*speed;
        p.max_life=random(max_life_clamp.first, max_life_clamp.second)*0.4f;
        p.life=p.max_life;
        p.size=random(size_clamp.first, size_clamp.second)*0.2f;
        p.color=color;
        p.active=true;
    }
}

