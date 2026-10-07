#include <cmath>
#include "Settings.h"
#include "GameState.h"
#include "Tool/Render.h"
#include "Tool/Font.h"
#include "Tool/Image.h"
#include "Tool/Random.h"
#include "Tool/Math.h"
#include "Tool/Utils.h"

#ifndef WEB_BUILD
#include <enet/enet.h>
#endif

using namespace std;
using namespace Settings;

void GameState::draw_info(int x, int y, const Info& info){
    int prefix_width=0;
    if(!info.prefix.empty()){
        DrawTextOptions opts;
        opts.color={175, 215, 230, 255};
        prefix_width=draw_text(x, y, info.prefix, opts);
    }
    DrawTextOptions opts;
    if(info.type==INFO_SYSTEM){
        opts.color={200, 200, 200, 255};
    }else if(info.type==INFO_CHAT){
        opts.color={255, 255, 255, 255};
    }else if(info.type==INFO_ACHIEVE){
        opts.color={255, 215, 0, 255};
    }
    draw_text(x+prefix_width, y, info.message, opts);
}

void GameState::draw_crosshair(Human* p){
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    int max_offset=lround((100.0f/p->stability)*0.175f);
    mouse_x+=random(-max_offset, max_offset);
    mouse_y+=random(-max_offset, max_offset);
    int shake=lround(pow(1.0f-(p->stability-10.0f)/90.0f, 1.5f)*10.0f);
    int size=12+shake, gap=4+shake, thickness=2;
    SDL_Color color={255, 255, 255, 160};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect h_left={mouse_x-size, mouse_y-thickness/2, size-gap, thickness};
    SDL_RenderFillRect(renderer, &h_left);
    SDL_Rect h_right={mouse_x+gap, mouse_y-thickness/2, size-gap, thickness};
    SDL_RenderFillRect(renderer, &h_right);
    SDL_Rect v_up={mouse_x-thickness/2, mouse_y-size, thickness, size-gap};
    SDL_RenderFillRect(renderer, &v_up);
    SDL_Rect v_down={mouse_x-thickness/2, mouse_y+gap, thickness, size-gap};
    SDL_RenderFillRect(renderer, &v_down);
    SDL_Rect dot={mouse_x-1, mouse_y-1, 2, 2};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 160);
    SDL_RenderFillRect(renderer, &dot);
}

void GameState::draw_scoreboard(){
    #ifndef WEB_BUILD
    int start_x=SCREEN_WIDTH/5, start_y=60, end_x=SCREEN_WIDTH*4/5, end_y=SCREEN_HEIGHT-90;
    draw_rounded_rect_texture(start_x, start_y, end_x-start_x, end_y-start_y, 80);
    for(int i=0; i<world.humans.size(); ++i){
        Human& h=world.humans[i];
        draw_text(start_x+10, start_y+i*20+10, h.name);
        IpLocation loc=safe_map_find(player_locations, h.id);
        draw_text(start_x+80, start_y+i*20+10, loc.country+"  "+loc.province);
    }
    DrawTextOptions opts;
    opts.color={180, 180, 180, 200};
    opts.cache=true;
    opts.center=true;
    opts.small=true;
    draw_text(SCREEN_WIDTH/2, end_y-10, "IP 地理位置数据由 IPinfo 提供 (ipinfo.io)", opts);
    #endif
}

void GameState::draw_screen(){//绘制屏幕
    Human* p=get_now_player();
    if(!p){
        draw_text(5, 0, "等待服务器同步...");
        return;
    }
    text_anomaly_level=(100-p->sanity)/5;
    float shake_x=randomf(-world.shake_intensity, world.shake_intensity), shake_y=randomf(-world.shake_intensity, world.shake_intensity);
    if(p->stun_time>0){
        int k=p->stun_time*0.1f;
        shake_x+=randomf(-k, k);
        shake_y+=randomf(-k, k);
    }
    view_start_x=p->physics_params.x-SCREEN_WIDTH/40.0+shake_x, view_start_y=p->physics_params.y-SCREEN_HEIGHT/40.0+shake_y;
    int start_x=max(0, int(floor(view_start_x))), end_x=min(world.width, int(ceil(view_start_x+SCREEN_WIDTH/16.0)));
    int start_y=max(0, int(floor(view_start_y))), end_y=min(world.height, int(ceil(view_start_y+SCREEN_HEIGHT/16.0)));
    Weapon* w=p->get_now_weapon(this->world);
    Uint32 now=SDL_GetTicks();
    auto draw_bar=[](int x, int y, int width, int height, int max_val, int cur_val, SDL_Color fill_color, bool show_text){
        float ratio=Math::clamp(float(cur_val)/max_val, 0.0f, 1.0f);
        SDL_Color new_color={Uint8(min(fill_color.r+20, 255)), Uint8(min(fill_color.g+20, 255)), Uint8(min(fill_color.b+20, 255)), Uint8(min(fill_color.a+20, 255))};
        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 128);
        SDL_Rect bg_rect={x, y, width, height};
        SDL_RenderFillRect(renderer, &bg_rect);
        SDL_Rect fill_rect={x, y, int(width*ratio), height};
        SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
        SDL_RenderFillRect(renderer, &fill_rect);
        SDL_SetRenderDrawColor(renderer, new_color.r, new_color.g, new_color.b, new_color.a);
        SDL_RenderDrawRect(renderer, &bg_rect);
        if(show_text){
            string text=to_string(cur_val)+'/'+to_string(max_val);
            DrawTextOptions opts;
            opts.color=new_color;
            draw_text(x+width/5, y, text, opts);
        }
    };
    if(slowmo_visual>0){
        SDL_SetRenderDrawColor(renderer, 30, 60, 120, Uint8(50*slowmo_visual));
        SDL_Rect full={0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &full);
        draw_vignette(Uint8(210*slowmo_visual));
    }
    //游戏信息
    draw_text(SCREEN_WIDTH*3/6+100, 20, to_string(smooth_fps)+"帧");
    #ifndef WEB_BUILD
    if(is_connected&&server_peer)draw_text(SCREEN_WIDTH*4/6+100, 20, "ping "+to_string(server_peer->roundTripTime));
    #endif
    //地图信息
    world.draw_map(start_x, end_x, start_y, end_y, view_start_x, view_start_y, false, p, self);
    //玩家信息
    draw_bar(30, 20, 100, 30, p->max_health, p->health, {200, 0, 0, 192}, true);
    draw_bar(30, 50, 100, 10, 100, int(p->stamina), {50, 150, 255, 192}, false);
    draw_bar(30, 60, 100, 10, 100, int(p->sanity), {255, 255, 255, 192}, false);
    draw_bar(SCREEN_WIDTH/2-60, 20, 100, 30, p->upgrade_health, p->experience, {200, 150, 0, 192}, true);

    draw_text(SCREEN_WIDTH*2/3+100, SCREEN_HEIGHT*2/3, w->name);
    if(w->type.second!=MELEE){
        draw_text(SCREEN_WIDTH*2/3+150, SCREEN_HEIGHT*2/3, to_string(w->now_ammo)+'/'+to_string(p->ammo[w->type.second]));
        if(w->load<100)draw_bar(SCREEN_WIDTH*2/3+100, SCREEN_HEIGHT*2/3+25, 80, 10, 100, int(w->load), {200, 150, 0, 192}, false);
    }
    SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
    SDL_Rect rect={SCREEN_WIDTH*2/3+60, SCREEN_HEIGHT*2/3+40, 220, 125};
    SDL_RenderFillRect(renderer, &rect);
    draw_image(w->image_path, SCREEN_WIDTH*2/3+170, SCREEN_HEIGHT*2/3+100, true);
    if(p->weapon_switch_time!=0){
        DrawTextOptions opts;
        opts.cache=true;
        opts.center=true;
        draw_text(SCREEN_WIDTH*2/3+170, SCREEN_HEIGHT*2/3+85, "切换武器...", opts);
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 96);
        SDL_Rect rect={SCREEN_WIDTH*2/3+55, SCREEN_HEIGHT*2/3, 230, 175};
        SDL_RenderFillRect(renderer, &rect);
    }

    draw_bar(50, SCREEN_HEIGHT*2/3, 100, 30, 100, int(p->flash_battery), (p->flash||p->night_vision)?SDL_Color{255, 255, 100, 255}:SDL_Color{60, 60, 70, 255}, false);
    //消息
    for(int i=0; i<min(10, int(infos.size())); ++i)draw_info(SCREEN_WIDTH*2/3+10, 20*(7+i), infos[i]);
    //提示
    bool has_supply=false, has_exit=false, supply_is_open=false;
    auto func1=[&](Supply* s)->bool{
        if(fabs(s->x-p->physics_params.x)<1&&fabs(s->y-p->physics_params.y)<1){
            has_supply=true;
            if(s->open)supply_is_open=true;
            return false;
        }
        return true;
    };
    auto func2=[&](Exit* ex)->bool{
        if(fabs(ex->x-p->physics_params.x)<1&&fabs(ex->y-p->physics_params.y)<1){
            has_exit=true;
            return false;
        }
        return true;
    };
    world.solve_grid_supply(p->physics_params.x, p->physics_params.y, 1, func1);
    world.solve_grid_exit(p->physics_params.x, p->physics_params.y, 1, func2);
    if(has_supply){
        DrawTextOptions opts;
        opts.color={145, 240, 145, 255};
        opts.cache=true;
        draw_text(SCREEN_WIDTH/5*2-40, SCREEN_HEIGHT*2/3, "[F]", opts);
        draw_bar(SCREEN_WIDTH/5*2-20, SCREEN_HEIGHT*2/3+10, 50, 10, 20, supply_is_open?20:p->search_item, {145, 240, 145, 255}, false);
    }
    if(has_exit){
        DrawTextOptions opts;
        opts.color={145, 240, 145, 255};
        opts.cache=true;
        draw_text(SCREEN_WIDTH/5*3-40, SCREEN_HEIGHT*2/3, "[E]", opts);
        draw_bar(SCREEN_WIDTH/5*3-20, SCREEN_HEIGHT*2/3+10, 50, 10, 20, p->go_exit, {145, 240, 145, 255}, false);
    }
    //特效
    world.particle_system.render(view_start_x, view_start_y);
    if(p->hurt>0){
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, min(p->hurt*32, 255));
        SDL_Rect rect={0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &rect);
    }
    if(is_inventory_open){
        //背包
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
        SDL_Rect rect={0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &rect);
        draw_inventory();
    }else{
        //准星
        draw_crosshair(p);
    }
    if(tab_held&&is_multiplayer){
        //计分板
        draw_scoreboard();
    }
    if(p->stun_time>0){
        //眩晕
        float t=now*0.001f, r=p->stun_time/30.0f;
        int off=int(5+3*sinf(t*10));
        draw_stun_texture(-off, -off, {255, 100, 100, Uint8(180*r)});
        draw_stun_texture(off, off, {100, 100, 255, Uint8(180*r)});
    }
    draw_noise_texture(Uint8(Math::clamp((100-(p->sanity))*0.9f+5, 0.0f, 255.0f)));
    if(p->nv_boot_time>0){
        int t=max(10-p->nv_boot_time, 0);
        float darkness;
        darkness=1.0f-float(max(t-3, 0))/7;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, Uint8(darkness*230));
        SDL_Rect full={0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &full);
        if(t>3){
            Uint8 flicker=Uint8(Math::clamp(60+(10-t)*4*sinf(now*0.001f*30.0f), 0.0f, 255.0f));
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, flicker);
            SDL_RenderFillRect(renderer, &full);
        }
        draw_noise_texture(30);
        draw_vignette(230);
    }
    if(p->night_vision){
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MUL);
        SDL_SetRenderDrawColor(renderer, 80, 255, 80, 255);
        SDL_Rect full={0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &full);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        draw_noise_texture(30);
        draw_vignette(230);
    }
}

void GameState::draw_inventory(){
    Human* p=get_now_player();
    if(!p)return;
    int total_size=NOW_USE_SIZE+BAG_SIZE+(inv_supply?inv_supply->items.size():0);
    auto get_pos=[](int i)->int{
        if(i<NOW_USE_SIZE){
            return i+1;
        }else if(i<NOW_USE_SIZE+BAG_SIZE){
            return i+2;
        }else{
            return i+3;
        }
    };
    draw_text(SCREEN_WIDTH/6+100, 20, world.name);
    draw_text(SCREEN_WIDTH/6*2+100, 20, "第"+to_string(world.day)+"天 "+(world.hour<10?"0":"")+to_string(world.hour)+":"+(world.minute<10?"0":"")+to_string(int(world.minute)));
    draw_text(SCREEN_WIDTH/6*3+100, 20, "金钱："+to_string(p->money));
    DrawTextOptions opts;
    opts.cache=true;
    draw_text(10, 0, "当前装备：", opts);
    draw_text(10, 20*(NOW_USE_SIZE+1), "背包：", opts);
    if(inv_supply)draw_text(10, 20*(NOW_USE_SIZE+BAG_SIZE+2), inv_supply->name+"：");
    for(int i=0; i<total_size; ++i){
        draw_text(20, 20*(get_pos(i)), world.get_inv_item(i, p, inv_supply)->name);
    }
    if(inv_swap_target==-1)opts.color=SDL_Color{255, 255, 0, 255};
    draw_text(5, 20*(get_pos(inv_selected)), "*", opts);
    opts.color={255, 255, 0, 255};
    if(inv_swap_target!=-1)draw_text(5, 20*(get_pos(inv_swap_target)), "*", opts);
    int display_ammo=total_size+3+(inv_supply?1:0);
    for(int i=0; i<AMMO_TYPE_SIZE; ++i){
        if(p->ammo[i]>0)draw_text(5, 20*(display_ammo++), AMMO_NAME[i]+"弹药："+to_string(p->ammo[i]));
    }

    if(inv_waiting_for_action){
        string prompt;
        for(int i=0; i<inv_options.size(); ++i)prompt+=to_string(i+1)+"."+inv_options[i]+" ";
        draw_text(5, 20*(total_size+8), prompt);
    }
}

