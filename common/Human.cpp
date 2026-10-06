#include <cmath>
#include "Tool/Math.h"
#include "Tool/Random.h"
#include "Human.h"
#include "World.h"
#include "Network.h"
using namespace std;
using namespace Settings;

Human::Human(){
    for(unique_ptr<Item>& i : now_use)i=unique_ptr<Item>(new Item());
    for(unique_ptr<Item>& i : bag)i=unique_ptr<Item>(new Item());
}

Weapon* Human::get_now_weapon(World& world){
    unique_ptr<Item>& temp=now_use[weapon_type];
    return temp->type.first==ITEM_WEAPON?(Weapon*)(temp.get()):&world.weapons.back();
}

void Human::turn_towards(float target_angle){
    float current_angle=atan2f(direction.second, direction.first);
    float diff=target_angle-current_angle;
    while(diff>PI)diff-=2*PI;
    while(diff<-PI)diff+=2*PI;
    diff=Math::clamp(diff, -HUMAN_TURN_SPEED, HUMAN_TURN_SPEED);
    float new_angle=current_angle+diff;
    direction={cosf(new_angle), sinf(new_angle)};
}

void Human::aim(){
    float dx=k_s->mouse_x-physics_params.x, dy=k_s->mouse_y-physics_params.y;
    float k=sqrtf(dx*dx+dy*dy);
    if(k==0||run)return;
    turn_towards(atan2f(dy, dx));
}

void Human::move(World& world){
    float dx=(get_key(*k_s, SDL_SCANCODE_D)||get_key(*k_s, SDL_SCANCODE_RIGHT))-(get_key(*k_s, SDL_SCANCODE_A)||get_key(*k_s, SDL_SCANCODE_LEFT));
    float dy=(get_key(*k_s, SDL_SCANCODE_S)||get_key(*k_s, SDL_SCANCODE_DOWN))-(get_key(*k_s, SDL_SCANCODE_W)||get_key(*k_s, SDL_SCANCODE_UP));
    float dist=sqrtf(dx*dx+dy*dy);
    run=false;
    if(dist==0){
        return;
    }else if(--footstep_time<=0){
        footstep_time=12;
        world.add_sfx("footsteps", physics_params.x, physics_params.y, run?60:45, run?10:6);
    }
    run=(get_key(*k_s, SDL_SCANCODE_LSHIFT)||get_key(*k_s, SDL_SCANCODE_RSHIFT))&&!get_key(*k_s, KEY_MOUSE_LEFT)&&rest==0;
    float k=HUMAN_ACCEL/dist*world.get_vel_scale(lround(physics_params.x), lround(physics_params.y));
    if(run){
        turn_towards(atan2f(dy, dx));
        k*=1.8f;
        stamina=max(stamina-0.6f, 0.0f);
        if(stamina<=EPSILON)rest=40;
    }
    if(hurt>0)k*=0.5;
    world.add_force(physics_params, dx*k, dy*k);
}

void Human::operate(World& world){//玩家操作
    if(!k_s||stun_time>0)return;
    Weapon* w=get_now_weapon(world);
    move(world);
    aim();
    if(get_key(*k_s, KEY_MOUSE_LEFT)&&attacke==0&&w->load==100&&weapon_switch_time==0){
        if(world.safe){
            attacke=10;
        }else{
            attacke=w->attacke;
            if(w->type.second==MELEE){//近战
                auto func=[&](Zombie* z)->bool{
                    float u=z->physics_params.x-physics_params.x, v=z->physics_params.y-physics_params.y;
                    float dist=sqrtf(u*u+v*v);
                    if(dist<EPSILON||(dist<1.2f&&acos((u*direction.first+v*direction.second)/dist)<HUMAN_SEE_ANGLE)){
                        if(!(z->type==FAST_ZOMBIE&&!random(0,3))){
                            int damage=min(w->damage, z->health);
                            z->health-=damage;
                            z->add_attacker_damage(id, damage);
                            z->aggression+=damage/6.0;
                            float kb=w->damage*1.8f;
                            float d=max(dist, EPSILON);
                            world.add_force(z->physics_params, u/d*kb, v/d*kb);
                            world.emit_blood(z->physics_params.x, z->physics_params.y, direction.first, direction.second, damage/3);
                            world.shake_intensity+=0.06f;
                            world.trigger_slowmo(0.25f, 0.2f);
                        }
                        return false;
                    }
                    return true;
                };
                world.solve_grid_zombie(physics_params.x, physics_params.y, 1.5f, func);
                slash_time=5;
                slash_angle=atan2f(direction.second, direction.first);
                world.add_sfx("sword_whoosh", physics_params.x, physics_params.y, 70, 8);
            }else if(w->now_ammo>0){//远程
                float offset=randomf(-1, 1)/stability;
                float vel_x=direction.first*w->vel+(-direction.second*offset), vel_y=direction.second*w->vel+(direction.first*offset);
                --w->now_ammo;
                if(w->type.second==SHOTGUN){
                    for(int i=-1; i<=1; ++i)world.add_bullet(physics_params.x, physics_params.y,
                    vel_x-0.15*direction.second*i, vel_y+0.15*direction.first*i, w->damage, w->health, id, w->can_explo);
                    auto func=[&](){
                        world.add_sfx("shotgun_pump", physics_params.x, physics_params.y, 70, 8);
                    };
                    world.tasks.schedule(300, func);
                }else{
                    world.add_bullet(physics_params.x, physics_params.y, vel_x, vel_y, w->damage, w->health, id, w->can_explo);
                }
                float recoil=w->damage;
                if(w->type.second==PISTOL){
                    recoil*=1.2f;
                }else if(w->type.second==SMG){
                    recoil*=0.7f;
                }else if(w->type.second==SHOTGUN){
                    recoil*=2.1f;
                }else if(w->type.second==RPG){
                    recoil*=3.2f;
                }
                stability=max(stability-recoil, 10.0f);
                if(w->now_ammo==0&&ammo[w->type.second]!=0){
                    w->load=0;
                }
                world.add_light(physics_params.x, physics_params.y, 3.0f, 1.0f, 0, PI, 1, {230, 230, 200});
            }
        }
    }
        
    if(get_key(*k_s, SDL_SCANCODE_E)){
        auto func=[&](Exit* ex)->bool{
            if(fabs(physics_params.x-ex->x)<1&&fabs(physics_params.y-ex->y)<1){
                if(++go_exit>=20){
                    world.save_map();
                    world.load_map(ex->go_name, ex->go_x, ex->go_y);
                    go_exit=0;
                }
                return false;
            }
            return true;
        };
        world.solve_grid_exit(physics_params.x, physics_params.y, 1, func);
    }else{
        go_exit=0;
    }
    if(get_key(*k_s, SDL_SCANCODE_F)){
        auto func=[&](Supply* s)->bool{
            if(fabs(physics_params.x-s->x)<1&&fabs(physics_params.y-s->y)<1){
                if(!s->is_opening&&(s->open||++search_item>=20)){
                    s->open=true;
                    s->is_opening=true;
                    search_item=0;
                    on_supply_open(s);
                }
                return false;
            }
            return true;
        };
        world.solve_grid_supply(physics_params.x, physics_params.y, 1, func);
    }else{
        search_item=0;
    }
    if(get_key(*k_s, SDL_SCANCODE_Q)&&weapon_switch_time==0){
        w->load=100;
        weapon_switch_time=5;
        swap(weapon_type, last_weapon_type);
    }
    if(get_key(*k_s, SDL_SCANCODE_R)&&w->type.second!=MELEE&&w->load>=100&&w->now_ammo!=w->max_ammo&&ammo[w->type.second]!=0){
        w->load=0;
    }
    if(get_key(*k_s, SDL_SCANCODE_T)&&flash_battery>=EPSILON){
        flash=!flash;
        if(flash)flash_battery=max(flash_battery-0.6, 0.0);
        world.add_sfx("flashlight_click", physics_params.x, physics_params.y, 50, 8);
    }
    if(get_key(*k_s, SDL_SCANCODE_N)&&flash_battery>=EPSILON){
        if(night_vision){
            night_vision=false;
        }else{
            nv_boot_time=10;
        }
    }
    if(weapon_switch_time==0){
        for(int i=0; i<NOW_USE_SIZE; ++i){
            if(get_key(*k_s, SDL_SCANCODE_1+i)&&i<now_use.size()&&weapon_type!=i){
                w->load=100;
                weapon_switch_time=5;
                last_weapon_type=weapon_type;
                weapon_type=i;
            }
        }
    }
}

void Human::update(World& world){
    hurt=max(hurt-1, 0);
    rest=max(rest-1, 0);
    attacke=max(attacke-1, 0);
    slash_time=max(slash_time-1, 0);
    weapon_switch_time=max(weapon_switch_time-1, 0);
    if(flash)flash_battery=max(flash_battery-0.07f, 0.0f);
    if(night_vision)flash_battery=max(flash_battery-0.11f, 0.0f);
    if(!flash&&!night_vision)flash_battery=min(flash_battery+0.02f, 100.0f);
    if(run){
        max_stability=70;
    }else{
        max_stability=100;
    }
    if(nv_boot_time>0&&--nv_boot_time==0){
        flash_battery=max(flash_battery-0.7, 0.0);
        night_vision=true;
    }
    if(rest==0)stamina=min(stamina+0.16f, 100.0f);
    sanity=min(sanity+0.01f, 100.0f);
    if(pending_sanity_damage>0){
        float final_damage=pow(pending_sanity_damage/2.5f, 0.16f)*0.04f;
        sanity=max(sanity-final_damage, 0.0f);
        pending_sanity_damage=0;
    }
    stability=min(stability+1.0f, max_stability);
    stun_time=max(stun_time-1, 0);
    Weapon* w=get_now_weapon(world);
    if(w->type.second!=MELEE&&w->load<100&&stun_time==0){
        w->load+=w->load_speed;
        if(w->load>=100){
            int can_load_ammo=min(w->max_ammo-w->now_ammo, ammo[w->type.second]);
            w->now_ammo+=can_load_ammo;
            ammo[w->type.second]-=can_load_ammo;
            w->load=100;
            world.add_sfx("handgun_click", physics_params.x, physics_params.y, 60, 8);
        }
    }
    if(flash_battery<EPSILON){
        flash=false;
        flash_battery=EPSILON;
    }
    if(experience>=upgrade_health){
        upgrade_health*=2.1;
        max_health+=20;
        health*=float(max_health)/(max_health-20);
    }
    if(health<=0){
        on_death();
        respawn();
        experience=max(experience-100, 0);
        physics_params.x=world.last_spawn_x;
        physics_params.y=world.last_spawn_y;
        world.add_sfx("human_scream", physics_params.x, physics_params.y, 90, 15);
    }
}

void Human::apply_input(World& world, const KeyState& input){
    k_s=&input;
    operate(world);
    update(world);
}

void Human::respawn(){
    health=max_health;
    stamina=100.0f;
    rest=0;
    sanity=100.0f;
    stun_time=0;
    physics_params.accel_x=0;
    physics_params.accel_y=0;
    physics_params.vel_x=0;
    physics_params.vel_y=0;
}

