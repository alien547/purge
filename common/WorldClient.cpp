#include <cmath>
#include "SDL_mixer.h"
#include "Settings.h"
#include "World.h"
#include "Tool/Math.h"
#include "Tool/Random.h"
#include "Tool/Render.h"
#include "Tool/Font.h"
#include "Tool/Image.h"
#include "Tool/Audio.h"
using namespace std;
using namespace Settings;

#ifndef SERVER_BUILD
uint8_t World::ir_of_cell(const Cell& c){
    if(c.info&(1<<CELL_WALL))return IR_WALL;
    if(c.info&(1<<CELL_WATER))return IR_WATER;
    if(c.grass_type==0)return IR_GRASS_DRY;
    if(c.grass_type==1)return IR_GRASS;
    if(c.grass_type==2)return IR_GRASS_DEEP;
}

uint8_t World::ir_of_zombie(const Zombie& z){
    return z.dead?IR_DEAD_ZOMBIE:IR_ZOMBIE;
}

pair<char, SDL_Color> World::get_zombie_appearance(Zombie& z){
    char a;
    SDL_Color color;
    if(z.type==NORMAL_ZOMBIE){
        a='E';
        color={200, 50, 50, 255};
    }else if(z.type==FAST_ZOMBIE){
        a='F';
        color={200, 50, 200, 255};
    }else if(z.type==TANK_ZOMBIE){
        a='T';
        color={70, 90, 130, 255};
    }else if(z.type==INVISIBLE_ZOMBIE){
        a='V';
        color={30, 30, 30, 128};
    }
    if(z.dead)color={128, 128, 128, 192};
    return {a, color};
}

void World::draw_map(int start_x, int end_x, int start_y, int end_y, float view_start_x, float view_start_y, bool global_light, Human* p, int self){
    float human_view_scale=(p&&p->run)?0.94:1;
    auto draw_entity=[&](float x, float y, char a, SDL_Color color, uint8_t ir, VisibilityStamp* vis=nullptr){
        if(p){
            if(!is_point_visible(p->physics_params.x, p->physics_params.y, x, y,
            HUMAN_SEE_LEN*human_view_scale, p->direction.first, p->direction.second, HUMAN_SEE_ANGLE*human_view_scale, vis))return;
        }
        if(p&&p->night_vision){
            float illum=0.4f+env_light_intensity*0.6f;
            float brightness=0.15f+powf(Math::clamp(ir/255.0f*illum, 0.0f, 1.0f), 0.4f)*0.85f;
            color={Uint8(brightness*255*0.28f), Uint8(brightness*255), Uint8(brightness*255*0.32f), 255};
        }else{
            array<int16_t, 3> light=global_light?array<int16_t, 3>{255, 255, 255}:get_light_color(x, y);
            if(light[0]==0&&light[1]==0&&light[2]==0)return;
            color.r=Uint8(Math::clamp(color.r*light[0]/255, 0, 255));
            color.g=Uint8(Math::clamp(color.g*light[1]/255, 0, 255));
            color.b=Uint8(Math::clamp(color.b*light[2]/255, 0, 255));
        }
        draw_char(lround((x-view_start_x)*FONT_SIZE), lround((y-view_start_y)*FONT_SIZE), a, color, true);
    };
    Uint32 ticks=SDL_GetTicks();
    for(int y=start_y; y<end_y; ++y){
        for(int x=start_x; x<end_x; ++x){
            char a=' ';
            SDL_Color color={255, 255, 255, 255};
            Cell& now_cell=get_cell(x, y);
            if(now_cell.info&(1<<CELL_WALL)){
                a='#';
                color={128, 128, 128, 255};
            }else if(now_cell.info&(1<<CELL_WATER)){
                a='~';
                float wave=sinf(ticks*0.002f+x*0.8f+y*0.5f);
                int bright=55*(wave*0.5f+0.5f), alpha=230+25*(wave*0.5f+0.5f);
                color={Uint8(60+bright*0.2f), Uint8(60+bright*0.2f), Uint8(200+bright), Uint8(alpha)};
            }else{
                a='.';
                if(now_cell.grass_type==0){
                    color={160, 130, 80, 255};
                }else if(now_cell.grass_type==1){
                    color={120, 200, 120, 255};
                }else if(now_cell.grass_type==2){
                    color={60, 160, 60, 255};
                }
            }
            if(a=='.'){
                int hcount=0;
                if(p){
                    float u=x-p->physics_params.x, v=y-p->physics_params.y;
                    float dist_sq=u*u+v*v;
                    hcount=max(int((36-dist_sq)*0.08f), 0);
                }
                color.a=Uint8(Math::clamp(128/(hcount*0.5f+1), 0.0f, 255.0f));
                const float step=1.0f/(hcount*2+1);
                for(int j=-hcount; j<=hcount; ++j){
                    for(int i=-hcount; i<=hcount; ++i){
                        draw_entity(x+i*step, y+j*step, a, color, ir_of_cell(now_cell));
                    }
                }
            }else{
                draw_entity(x, y, a, color, ir_of_cell(now_cell));
            }
            if(rain_time>0&&!random(0, 12-(rain_heavy?4:0))){
                draw_entity(x, y, '\\', {50, 60, 255, Uint8(180+random(0, 3)*24)}, IR_RAIN);
            }
        }
    }
    for(BloodStain& b : blood_stains){
        draw_entity(b.x, b.y, '.', {b.color.r, b.color.g, b.color.b, b.alpha}, IR_BLOOD);
    }
    draw_shadows(view_start_x, view_start_y, p, self);
    //人类
    for(int i=0; i<humans.size(); ++i){
        Human& h=humans[i];
        draw_entity(h.physics_params.x, h.physics_params.y, i==self?'I':'H', i==self?SDL_Color{100, 255, 100, 255}:SDL_Color{0, 200, 200, 255}, IR_HUMAN, &h.vis);
        if(h.vis.visible){
            //名称
            DrawTextOptions opts;
            opts.color={255, 255, 255, 128};
            opts.center=true;
            opts.small=true;
            draw_text(lround((h.physics_params.x-view_start_x)*FONT_SIZE), lround((h.physics_params.y-view_start_y)*FONT_SIZE), h.name, opts);
            //视线
            const int DOTS=11;
            const float STEP=0.07f;
            for(int d=1; d<=DOTS; ++d){
                float fx=h.physics_params.x+h.direction.first*STEP*d, fy=h.physics_params.y+h.direction.second*STEP*d;
                Uint8 a=Uint8(220-i*20);
                draw_char(lround((fx-view_start_x)*FONT_SIZE), lround((fy-view_start_y-0.5f)*FONT_SIZE), '.', {255, 255, 255, a}, true);
            }
            //挥刀特效
            if(h.slash_time>0){
                float alpha=h.slash_time/6.0f, radius=1.3f, arc=1.3f;
                float start=h.slash_angle-arc/2;
                const int COUNT=30;
                for(int i=0; i<COUNT; ++i){
                    float t=float(i)/(COUNT-1);
                    float a=start+arc*t;
                    float p_x=h.physics_params.x+radius*cosf(a), p_y=h.physics_params.y+radius*sinf(a);
                    Uint8 a_mod=Uint8(alpha*180*(0.3f+0.7f*(1.0f-2.0f*fabs(t-0.5f))));
                    draw_char(lround((p_x-view_start_x)*FONT_SIZE), lround((p_y-view_start_y)*FONT_SIZE), '.', {200, 220, 255, a_mod}, true);
                }
            }
        }
    }
    //补给
    for(unique_ptr<Supply>& s : supplies){
        draw_entity(s->x, s->y, '+', {0, 255, 0, Uint8(s->open?128:255)}, IR_SUPPLY, &(s->vis));
    }
    //僵尸
    for(Zombie& z : zombies){
        pair<char, SDL_Color> k=get_zombie_appearance(z);
        draw_entity(z.physics_params.x, z.physics_params.y, k.first, k.second, ir_of_zombie(z), &z.vis);
    }
    //子弹
    for(Bullet& b : bullets){
        draw_entity(b.x, b.y, '\'', {255, 255, 200, 255}, IR_BULLET, &b.vis);
    }
    //出口
    for(Exit& ex : exits){
        draw_entity(ex.x, ex.y, 'Q', {0, 200, 255, Uint8(96+155*(0.4f+0.6f*sinf(ticks*0.004f)))}, IR_EXIT, &ex.vis);
    }
    for(Light& l : lights){
        if(p&&!is_point_visible(p->physics_params.x, p->physics_params.y, l.x, l.y, HUMAN_SEE_LEN*human_view_scale,
        p->direction.first, p->direction.second, HUMAN_SEE_ANGLE*human_view_scale))continue;
        float facing=1.0f;
        if(p&&l.cone_angle<PI-EPSILON){
            float dx=p->physics_params.x-l.x, dy=p->physics_params.y-l.y;
            float dist=sqrtf(dx*dx+dy*dy);
            if(dist>EPSILON){
                float to_player=atan2f(dy, dx);
                float diff=fabsf(to_player-l.dir_angle);
                if(diff>PI)diff=2.0f*PI-diff;
                float half=l.cone_angle;
                if(diff>=half*1.5f){
                    facing=0.0f;
                }else{
                    float cos_t=cosf(diff/half*PI*0.5f);
                    cos_t=Math::clamp(cos_t, 0.0f, 1.0f);
                    facing=cos_t*cos_t;
                }
            }
        }
        if(facing<=0.0f)continue;
        Uint8 alpha=Math::clamp(l.intensity*((p&&p->night_vision)?160:100), 0.0f, 255.0f)*facing;
        int sx=lround((l.x-view_start_x)*FONT_SIZE), sy=lround((l.y-view_start_y)*FONT_SIZE);
        int r=FONT_SIZE*min(l.r, 20.0f)*((p&&p->night_vision)?0.35f:0.2f);
        draw_light_orb_texture(sx, sy, r, {l.color.r, l.color.g, l.color.b, alpha});
        draw_light_orb_texture(sx, sy, r*2, {l.color.r, l.color.g, l.color.b, Uint8(alpha/2.5)});
    }
}

void World::draw_shadows(float view_start_x, float view_start_y, Human* p, int self){
    auto draw_one=[&](float ox, float oy, char ch, VisibilityStamp* vis=nullptr){
        if(p){
            float k=p->run?0.94:1;
            if(!is_point_visible(p->physics_params.x, p->physics_params.y, ox, oy,
            HUMAN_SEE_LEN*k, p->direction.first, p->direction.second, HUMAN_SEE_ANGLE*k, vis))return;
        }
        for(Light& l : lights){
            float dx=ox-l.x, dy=oy-l.y;
            float dist=sqrtf(dx*dx+dy*dy);
            if(dist<EPSILON)continue;
            float intensity=light_intensity_at(l, ox, oy);
            if(intensity<=0.0f)continue;
            dx/=dist;
            dy/=dist;
            float shadow_len=min(dist*0.7f, 6.0f);
            Uint8 alpha=Uint8(Math::clamp(255.0f*intensity, 0.0f, 255.0f));
            draw_char_shadow(lround((ox-view_start_x)*FONT_SIZE), lround((oy-view_start_y)*FONT_SIZE), ch, dx, dy, shadow_len, {64, 64, 64, alpha}, true);
        }
    };
    for(int i=0; i<humans.size(); ++i){
        Human& h=humans[i];
        draw_one(h.physics_params.x, h.physics_params.y, i==self?'I':'H', &h.vis);
    }
    for(unique_ptr<Supply>& s : supplies){
        draw_one(s->x, s->y, '+', &(s->vis));
    }
    for(Zombie& z : zombies){
        pair<char, SDL_Color> k=get_zombie_appearance(z);
        draw_one(z.physics_params.x, z.physics_params.y, k.first, &z.vis);
    }
    for(Bullet& b : bullets){
        draw_one(b.x, b.y, '\'', &b.vis);
    }
    for(Exit& ex : exits){
        draw_one(ex.x, ex.y, 'Q', &ex.vis);
    }
}

void World::update_persistent_sounds(){
    static int last_play_sfx_info=0;
    int changed=play_sfx_info^last_play_sfx_info;
    if(changed==0)return;
    if(changed&(1<<SOUND_RAIN)){
        if(play_sfx_info&(1<<SOUND_RAIN)){
            play_sfx(rain_heavy?"heavy_rain":"light_rain", -1, CHANNEL_RAIN);
        }else{
            Mix_HaltChannel(CHANNEL_RAIN);
        }
    }
    last_play_sfx_info=play_sfx_info;
}

int World::play_sfx_at(const string& name, float x, float y, float base_vol, float max_dist){
    if(!sound_on)return -1;
    int channel=play_sfx(name, 0, -1);
    if(channel==-1)return -1;
    sfx.push_back({name, channel, x, y, base_vol, max_dist});
    return channel;
}

void World::play_music_at(const string& name){
    play_music(name);
}

void World::update_audio_distances(float listener_x, float listener_y){
    if(!sound_on)return;
    for(int i=0; i<sfx.size(); ++i){
        Sfx& s=sfx[i];
        if(!Mix_Playing(s.channel)){
            swap(sfx[i--], sfx.back());
            sfx.pop_back();
            continue;
        }
        if(s.x!=-1){
            float dx=s.x-listener_x, dy=s.y-listener_y;
            float dist_sq=dx*dx+dy*dy, max_dist_sq=s.max_dist*s.max_dist;
            float vol_factor=dist_sq>=max_dist_sq?0.0f:1.0f-dist_sq/max_dist_sq;
            int new_vol=int(s.base_vol*vol_factor);
            Mix_Volume(s.channel, new_vol);

            float angle_deg=atan2f(dy, dx)*180.0f/PI;//方向感
            if(angle_deg<0)angle_deg+=360.0f;
            Uint8 angle_byte=Uint8(angle_deg*255.0f/360.0f);
            float dist_ratio_sq=(dist_sq>=max_dist_sq)?1.0f:dist_sq/max_dist_sq;
            Uint8 dist_byte=Uint8(dist_ratio_sq*255.0f);
            Mix_SetPosition(s.channel, angle_byte, dist_byte);
        }else{
            Mix_Volume(s.channel, s.base_vol);
            Mix_SetPosition(s.channel, 128, 128);
        }
    }
}

void World::stop_all_sfx(){
    for(Sfx& s : sfx){
        stop_sfx(s.channel);
    }
    sfx.clear();
}

array<int16_t, 3> World::get_light_color(float x, float y){
    float total_r=env_light_color.r*env_light_intensity, total_g=env_light_color.g*env_light_intensity, total_b=env_light_color.b*env_light_intensity;
    for(Light& l : lights){
        float dx=x-l.x, dy=y-l.y;
        if(dx*dx+dy*dy<EPSILON){
            total_r+=l.color.r;
            total_g+=l.color.g;
            total_b+=l.color.b;
            continue;
        }
        float intensity=light_intensity_at(l, x, y);
        if(intensity<=0.0f)continue;
        total_r+=l.color.r*intensity;
        total_g+=l.color.g*intensity;
        total_b+=l.color.b*intensity;
    }
    if(sky_flash&1){
        total_r+=80;
        total_g+=80;
        total_b+=80;
    }
    auto tone=[](float v)->float{
        const float WHITE=1.5f;
        float n=v/255.0f;
        n=n*(1.0f+n/(WHITE*WHITE))/(1.0f+n);
        return n*255.0f;
    };
    total_r=tone(total_r);
    total_g=tone(total_g);
    total_b=tone(total_b);
    return {int16_t(total_r), int16_t(total_g), int16_t(total_b)};
}
#endif

