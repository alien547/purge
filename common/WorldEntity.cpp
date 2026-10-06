#include <cmath>
#include "Settings.h"
#include "World.h"
#include "Tool/Math.h"
#include "Tool/Random.h"
using namespace std;
using namespace Settings;

void World::clean_supply(Supply* supply){
    if(!supply)return;
    for(int i=0; i<supply->items.size(); ++i){
        if(supply->items[i]->type.first==ITEM_EMPTY){
            supply->items.erase(supply->items.begin()+i--);
        }
    }
    supply->items.push_back(unique_ptr<Item>(new Item()));
}

pair<float, float> World::select_weighted_position(){
    const int MAX_RADIUS=20;
    if(humans.empty())return {-1, -1};
    Human& h=humans[random(0, humans.size()-1)];
    for(int attempt=0; attempt<3; ++attempt){
        float r=sqrtf(randomf(0, 1))*MAX_RADIUS;
        float prob=1.0f-r/MAX_RADIUS-0.1f;
        if(randomf(0, 1)>prob)continue;
        float angle=randomf(0, 1)*2.0f*PI;
        float x=h.physics_params.x+r*cosf(angle), y=h.physics_params.y+r*sinf(angle);
        if(x<0||x>=width||y<0||y>=height||(get_cell(x, y).info&(1<<CELL_WALL)))continue;
        return {x, y};
    }
    int t=3;
    float x=-1, y=-1;
    while(t--){
        x=randomf(0, width-1);
        y=randomf(0, height-1);
        if(!(get_cell(x, y).info&(1<<CELL_WALL)))break;
    }
    if(t==-1)return {-1, -1};
    return {x, y};
}

void World::add_force(PhysicsParams& params, float force_x, float force_y){
    params.accel_x+=force_x/params.mass;
    params.accel_y+=force_y/params.mass;
}

bool World::update_physics(PhysicsParams& params){
    const float FRIC=0.01f;
    float old_x=params.x, old_y=params.y, new_x=params.x, new_y=params.y;
    auto change_vel=[&](float& vel, float& accel){
        vel=(vel+accel)*params.drag;
        if(vel>FRIC){
            vel-=FRIC;
        }else if(vel<-FRIC){
            vel+=FRIC;
        }else{
            vel=0;
        }
    };
    change_vel(params.vel_x, params.accel_x);
    change_vel(params.vel_y, params.accel_y);
    new_x+=params.vel_x*time_scale;
    new_y+=params.vel_y*time_scale;
    if(has_wall(old_x, old_y, new_x, old_y)){
        params.vel_x*=-params.boun;
    }else{
        params.x=new_x;
    }
    if(has_wall(old_x, old_y, old_x, new_y)){
        params.vel_y*=-params.boun;
    }else{
        params.y=new_y;
    }
    params.x=Math::clamp(params.x, 0.0f, float(width-1));
    params.y=Math::clamp(params.y, 0.0f, float(height-1));
    if(get_cell(lround(params.x), lround(params.y)).info&(1<<CELL_WALL)){
        params.x=old_x;
        params.y=old_y;
    }
    bool stun=params.accel_x*params.accel_x+params.accel_y*params.accel_y>0.64f;
    params.accel_x=0.0f;
    params.accel_y=0.0f;
    return stun;
}

bool World::has_wall(float x1, float y1, float x2, float y2, bool skip){
    const float EPS=1e-6f;
    float dx=x2-x1, dy=y2-y1;
    int start_x=lround(x1), start_y=lround(y1), end_x=lround(x2), end_y=lround(y2);
    int cur_x=start_x, cur_y=start_y;
    int step_x=(dx>0)?1:(dx<0?-1:0), step_y=(dy>0)?1:(dy<0?-1:0);
    float t_max_x=(dx!=0)?((step_x>0)?(float(cur_x)+0.5f-x1)/dx:(float(cur_x)-0.5f-x1)/dx):INFINITY;
    float t_max_y=(dy!=0)?((step_y>0)?(float(cur_y)+0.5f-y1)/dy:(float(cur_y)-0.5f-y1)/dy):INFINITY;
    float t_dx=(dx!=0)?1.0f/fabs(dx):INFINITY;
    float t_dy=(dy!=0)?1.0f/fabs(dy):INFINITY;
    auto is_blocked=[&](int x, int y)->bool{
        if(skip&&((x==start_x&&y==start_y)||(x==end_x&&y==end_y)))return false;
        if(x<0||x>=width||y<0||y>=height)return true;
        return get_cell(x, y).info&(1<<CELL_WALL);
    };

    if(is_blocked(cur_x, cur_y))return true;
    while(cur_x!=end_x||cur_y!=end_y){
        if(cur_x==end_x)t_max_x=INFINITY;
        if(cur_y==end_y)t_max_y=INFINITY;
        if(t_max_x<t_max_y-EPS){
            cur_x+=step_x;
            t_max_x+=t_dx;
            if(is_blocked(cur_x, cur_y))return true;
        }else if(t_max_y<t_max_x-EPS){
            cur_y+=step_y;
            t_max_y+=t_dy;
            if(is_blocked(cur_x, cur_y))return true;
        }else{
            int nx1=cur_x+step_x, ny1=cur_y, nx2=cur_x, ny2=cur_y+step_y, nx_diag=cur_x+step_x, ny_diag=cur_y+step_y;
            if(is_blocked(nx1, ny1))return true;
            if(is_blocked(nx2, ny2))return true;
            cur_x=nx_diag;
            cur_y=ny_diag;
            t_max_x+=t_dx;
            t_max_y+=t_dy;
            if(is_blocked(cur_x, cur_y))return true;
        }
    }
    return false;
}

bool World::is_point_visible(float x1, float y1, float x2, float y2, float see_len, float direction_x, float direction_y, float see_angle, VisibilityStamp* vis){
    if(vis&&vis->frame==now_frame)return vis->visible;
    bool visible=([&]()->bool{
        float dx=x2-x1, dy=y2-y1;
        float dist_sq=dx*dx+dy*dy;
        if(dist_sq<EPSILON)return true;
        if(dist_sq>see_len*see_len)return false;
        float dot=(direction_x*dx+direction_y*dy)/sqrtf(dist_sq);
        if(dot<cosf(see_angle))return false;
        return !has_wall(x1, y1, x2, y2, true);
    })();
    if(vis){
        vis->frame=now_frame;
        vis->visible=visible;
    }
    return visible;
}

void World::resolve_collisions(){
    const float MIN_DIST=0.25f;
    auto process=[&](PhysicsParams& a, PhysicsParams& b){
        float dx=a.x-b.x, dy=a.y-b.y, d_sq=dx*dx+dy*dy;
        if(d_sq>=MIN_DIST*MIN_DIST)return;
        if(d_sq<=0.01f){
            float u=random(0, 1)?0.1f:-0.1f, v=random(0, 1)?0.1f:-0.1f;
            a.x+=u;
            b.x-=u;
            a.y+=v;
            b.y-=v;
            return;
        }
        int i_x, i_y;
        float dist=sqrtf(d_sq);
        float n_x=dx/dist, n_y=dy/dist, overlap=MIN_DIST-dist;
        float a_shift=overlap*0.5f, b_shift=-overlap*0.5f;
        float a_x2=a.x+n_x*a_shift, a_y2=a.y+n_y*a_shift;
        i_x=lround(a_x2);
        i_y=lround(a_y2);
        if(0<=i_x&&i_x<width&&0<=i_y&&i_y<height&&!has_wall(a.x, a.y, a_x2, a_y2)){
            a.x=a_x2;
            a.y=a_y2;
        }
        float b_x2=b.x+n_x*b_shift, b_y2=b.y+n_y*b_shift;
        i_x=lround(b_x2);
        i_y=lround(b_y2);
        if(0<=i_x&&i_x<width&&0<=i_y&&i_y<height&&!has_wall(b.x, b.y, b_x2, b_y2)){
            b.x=b_x2;
            b.y=b_y2;
        }
        float rel_v_x=a.vel_x-b.vel_x, rel_v_y=a.vel_y-b.vel_y;
        float rel_v_n=rel_v_x*n_x+rel_v_y*n_y;
        if(rel_v_n<0){
            float restitution=(a.boun+b.boun)*0.5f;
            float impulse=-(1+restitution)*rel_v_n/(1.0f/a.mass+1.0f/b.mass);
            a.vel_x+=(impulse/a.mass)*n_x;
            a.vel_y+=(impulse/a.mass)*n_y;
            b.vel_x-=(impulse/b.mass)*n_x;
            b.vel_y-=(impulse/b.mass)*n_y;
        }
    };
    for(Human& hi : humans){
        auto func1=[&](Zombie* z)->bool{
            process(hi.physics_params, z->physics_params);
            return true;
        };
        auto func2=[&](Human* hj)->bool{
            if(&hi<hj)process(hi.physics_params, hj->physics_params);//防止重复处理
            return true;
        };
        solve_grid_zombie(hi.physics_params.x, hi.physics_params.y, MIN_DIST, func1);
        solve_grid_human(hi.physics_params.x, hi.physics_params.y, MIN_DIST, func2);
    }
    for(Zombie& zi : zombies){
        auto func=[&](Zombie* zj)->bool{
            if(&zi<zj)process(zi.physics_params, zj->physics_params);
            return true;
        };
        solve_grid_zombie(zi.physics_params.x, zi.physics_params.y, MIN_DIST, func);
    }
}

void World::add_explo(float x, float y, float r, int display, int damage, int id, int recursion_level){
    int cx=lround(x), cy=lround(y);
    bool underwater=(cx>=0&&cx<width&&cy>=0&&cy<height)&&(get_cell(cx, cy).info&(1<<CELL_WATER));
    float er=underwater?r*1.4f:r, ed=underwater?damage*1.1f:damage, kb=underwater?0.7f:1.1f;
    auto calc=[&](float ex, float ey, float& fx, float& fy)->int{
        float u=ex-x, v=ey-y;
        float ds=u*u+v*v;
        if(ds>=er*er||has_wall(x, y, ex, ey, true))return -1;
        float d=max(sqrtf(ds), EPSILON);
        int dmg=int(ed/(1.0f+ds));
        float f=dmg*(1.0f-d/er)*kb;
        fx=u/d*f;
        fy=v/d*f;
        return dmg;
    };
    auto func1=[&](Zombie* z)->bool{
        float fx, fy;
        int dmg=calc(z->physics_params.x, z->physics_params.y, fx, fy);
        add_force(z->physics_params, fx, fy);
        dmg=min(dmg, z->health);
        z->health-=dmg;
        z->add_attacker_damage(id, dmg);
        if(!underwater)emit_blood(z->physics_params.x, z->physics_params.y, fx*0.1f, fy*0.1f, dmg/3);
        return true;
    };
    auto func2=[&](Human* h)->bool{
        float fx, fy;
        int dmg=calc(h->physics_params.x, h->physics_params.y, fx, fy);
        add_force(h->physics_params, fx, fy);
        h->health-=min(dmg, h->health);
        if(!underwater)emit_blood(h->physics_params.x, h->physics_params.y, fx*0.1f, fy*0.1f, dmg/3);
        return true;
    };
    solve_grid_zombie(x, y, er, func1);
    solve_grid_human(x, y, er, func2);
    shake_intensity+=er*0.08f;
    struct LD{
        float rm, in;
        int d;
        ColorRGB c;
    };
    static const LD water[3]={{1.5f, 1.0f, 8, {180, 220, 255}}, {2.5f, 0.5f, 20, {100, 150, 220}}, {0.8f, 0.3f, 25, {200, 230, 255}}},
    land[3]={{1.2f, 1.2f, 8, {255, 200, 150}}, {2.5f, 0.6f, 20, {255, 150, 50}}, {0.8f, 0.4f, 25, {255, 100, 20}}};
    const LD* L=underwater?water:land;
    for(int i=0; i<3; ++i)add_light(x, y, er*L[i].rm, L[i].in, 0, PI, L[i].d, L[i].c);

    if(underwater){
        emit_particle(x, y, {2, 6}, {display*3/2, display*5/2}, {3, 8}, damage/6, {200, 230, 255});
        emit_particle(x, y, {3, 8}, {display*1/2, display*1/1}, {4, 10}, damage/6, {120, 180, 240});
    }else{
        emit_particle(x, y, {5, 13}, {display*7/10, display*13/10}, {5, 13}, damage/6, {255, 200, 0});
        emit_particle(x, y, {3, 8},  {display*1/1,  display*2/1},  {6, 14}, damage/6, {80, 60, 50});
    }
    add_sfx(underwater?"explo_underwater":"explo", x, y, 110, 25);
    if(underwater&&recursion_level==0){
        tasks.schedule(500, [this, x, y, r, display, damage, id](){
            add_explo(x, y, r*0.6f, display, int(damage*0.35f), id, 1);
        });
    }
}

void World::add_light(float x, float y, float r, float intensity, float dir_angle, float cone_angle, int display, ColorRGB color){
    lights.push_back({color, x, y, r, intensity, dir_angle, cone_angle, display});
}

void World::add_supply(float x, float y, bool open, bool lasting, const string& name, unique_ptr<Item> item, int count){
    Supply* su=nullptr;
    if(x==-1){
        pair<float, float> pos=select_weighted_position();
        if(pos.first==-1)return;
        x=pos.first;
        y=pos.second;
    }
    if(supplies.size()>max_supply_size&&!supplies.empty()){
        int t=3;
        while(t--){
            int k=random(0, supplies.size()-1);
            bool p=true;
            unique_ptr<Supply>& s=supplies[k];
            auto func=[&](Human* h)->bool{
                float u=h->physics_params.x-s->x, v=h->physics_params.y-s->y;
                if(u*u+v*v<3.5f){
                    p=false;
                    return false;
                }
                return true;
            };
            solve_grid_human(s->x, s->y, 2.0f, func);
            if(!p)continue;
            swap(s, supplies.back());
            supplies.pop_back();
            break;
        }
        if(t==-1)return;
    }
    if(!su){
        supplies.push_back(unique_ptr<Supply>(new Supply(x, y, open, lasting, name)));
        su=supplies.back().get();
    }
    su->id=next_supply_id++;
    if(item){
        su->items.push_back(move(item));
    }else{
        while(count--){
            int type=random(0, 5);
            if(type<=1){
                int a=random(0, 2);
                if(a<=1){
                    su->items.push_back(unique_ptr<Item>(new Item("医疗包", 50, {ITEM_CONS, CONS_MEDKIT})));
                }else if(a<=2){
                    su->items.push_back(unique_ptr<Item>(new Item("镇静剂", 50, {ITEM_CONS, CONS_PSYCH})));
                }
            }else if(type<=3){
                int a=random(0, weapons.size()-2);
                su->items.push_back(unique_ptr<Weapon>(new Weapon(weapons[a])));
            }else if(type<=5){
                int a=random(0, AMMO_TYPE_SIZE-1), b;
                if(a==PISTOL){
                    b=random(9, 13);
                }else if(a==SMG){
                    b=random(23, 28);
                }else if(a==SHOTGUN){
                    b=random(6, 9);
                }else if(a==RPG){
                    b=random(2, 3);
                }
                su->items.push_back(unique_ptr<Item>(new Ammo(10, a, b)));
            }
        }
    }
    clean_supply(su);
}

void World::add_bullet(float x, float y, float vel_x, float vel_y, int damage, int health, int id, bool can_explo){
    bullets.push_back({x, y, vel_x, vel_y, damage, health, id, can_explo});
    add_sfx("gun_shot", x, y, 100, 20);
}

void World::add_zombie(){
    int zombie_damage, zombie_health, zombie_x, zombie_y;
    float zombie_accel, k=powf(day, 0.3f);
    pair<float, float> pos=select_weighted_position();
    if(pos.first==-1)return;
    zombie_x=pos.first;
    zombie_y=pos.second;
    if(zombies.size()>max_zombie_size&&!zombies.empty()){
        int t=3;
        while(t--){
            int k=random(0, zombies.size()-1);
            bool p;
            Zombie& z=zombies[k];
            auto func=[&](Human* h)->bool{
                float u=h->physics_params.x-z.physics_params.x, v=h->physics_params.y-z.physics_params.y;
                if(u*u+v*v<3.5f){
                    p=false;
                    return false;
                }
                return true;
            };
            solve_grid_human(z.physics_params.x, z.physics_params.y, 2.0f, func);
            if(!p)continue;
            swap(z, zombies.back());
            zombies.pop_back();
        }
        if(t==-1)return;
    }
    zombie_damage=randomf(7, 9)*k;
    zombie_health=randomf(70, 80)*k;
    zombie_accel=randomf(2.5f, 3.0f);
    Zombie zombie;
    if(day>=2&&!random(0, 5)){
        zombie=Zombie(zombie_damage, zombie_health, FAST_ZOMBIE, zombie_x, zombie_y, zombie_accel*2.3f);
        zombie.physics_params.mass=55;
    }else if(day>=2&&!random(0, 7)){
        zombie=Zombie(int(zombie_damage*2.3f), int(zombie_health*3.1f), TANK_ZOMBIE, zombie_x, zombie_y, zombie_accel*1.1f);
        zombie.physics_params.mass=110;
    }else if(day>=2&&!random(0, 8)){
        zombie=Zombie(zombie_damage, zombie_health, INVISIBLE_ZOMBIE, zombie_x, zombie_y, zombie_accel*1.3f);
    }else{
        zombie=Zombie(zombie_damage, zombie_health, NORMAL_ZOMBIE, zombie_x, zombie_y, zombie_accel);
    }
    zombies.push_back(zombie);
}

void World::light_update(){
    for(int i=0; i<lights.size(); ++i){
        Light& l=lights[i];
        if(l.display==-1)continue;
        if(--l.display<=0){
            swap(lights[i--], lights.back());
            lights.pop_back();
            continue;
        }
        l.intensity=l.intensity*l.display/(l.display+1);
    }
}

void World::bullet_update(){
    for(int i=0; i<bullets.size(); ++i){
        Bullet& b=bullets[i];
        float l_x=b.x, l_y=b.y;
        b.health-=1.0f*time_scale;
        b.x+=b.vel_x*time_scale;
        b.y+=b.vel_y*time_scale;
        if(0<=b.x&&b.x<width&&0<=b.y&&b.y<height){
            auto func1=[&](Zombie* z)->bool{
                if(b.health<=0)return false;
                float u=z->physics_params.x-b.x, v=z->physics_params.y-b.y;
                if(u*u+v*v<0.35f){
                    int damage=min(b.damage, z->health);
                    z->health-=damage;
                    z->aggression+=damage/6.0;
                    z->add_attacker_damage(b.id, damage);
                    float blen=max(sqrtf(b.vel_x*b.vel_x+b.vel_y*b.vel_y), EPSILON);
                    float kb=b.damage*0.4f;
                    add_force(z->physics_params, b.vel_x/blen*kb, b.vel_y/blen*kb);
                    emit_blood(z->physics_params.x, z->physics_params.y, b.vel_x/blen, b.vel_y/blen, damage/5);
                    b.health-=10;
                }
                return true;
            };
            auto func2=[&](Human* h)->bool{
                if(b.health<=0||h->id==b.id)return false;
                float u=h->physics_params.x-b.x, v=h->physics_params.y-b.y;
                if(u*u+v*v<0.35f){
                    int damage=min(b.damage, h->health);
                    h->health-=damage;
                    float blen=max(sqrtf(b.vel_x*b.vel_x+b.vel_y*b.vel_y), EPSILON);
                    float kb=b.damage*0.5f;
                    add_force(h->physics_params, b.vel_x/blen*kb, b.vel_y/blen*kb);
                    emit_blood(h->physics_params.x, h->physics_params.y, b.vel_x/blen, b.vel_y/blen, damage/5);
                    b.health-=10;
                }
                return true;
            };
            solve_grid_zombie(b.x, b.y, 1.0f, func1);
            solve_grid_human(b.x, b.y, 1.0f, func2);
            if(has_wall(l_x, l_y, b.x, b.y))b.health-=30;
            if(b.health>0)continue;
        }
        if(b.can_explo){
            bool k=get_cell(lround(b.x), lround(b.y)).info&(1<<CELL_WATER);
            add_explo(b.x, b.y, 4.6f, 11, 150, b.id, 0);
        }
        swap(bullets[i--], bullets.back());
        bullets.pop_back();
    }
}

void World::zombie_update(){
    for(int i=0; i<zombies.size(); ++i){
        if(!zombies[i].update(*this)){
            swap(zombies[i--], zombies.back());
            zombies.pop_back();
        }
    }
}

