#include <algorithm>
#include <queue>
#include <cmath>
#include "World.h"
#include "Human.h"
#include "Zombie.h"
#include "Tool/Random.h"
#include "Tool/Math.h"
using namespace std;
using namespace Settings;

Zombie::Zombie(){}

Zombie::Zombie(int d, int h, int t, float x, float y, float a)
:damage(d), health(h), type(t), accel(a){physics_params.x=x;physics_params.y=y;}

void Zombie::add_attacker_damage(uint64_t id, int damage){
    for(int i=0; i<attacker_count; ++i){
        if(attacker_damage[i].first==id){
            attacker_damage[i].second+=damage;
            return;
        }
    }
    if(attacker_count<MAX_ATTACKER_SIZE){
        attacker_damage[attacker_count++]={id, damage};
        return;
    }
    int min_damage_idx=0;
    for(int i=1; i<MAX_ATTACKER_SIZE; ++i){
        if(attacker_damage[i].second<attacker_damage[min_damage_idx].second){
            min_damage_idx=i;
        }
    }
    attacker_damage[min_damage_idx]={id, damage};
}

bool Zombie::move_distance(World& world, float p_x, float p_y){
    float dx=p_x-physics_params.x, dy=p_y-physics_params.y;
    float dist=sqrtf(dx*dx+dy*dy);
    if(dist==0){
        return false;
    }
    float k=accel/dist*world.get_vel_scale(lround(physics_params.x), lround(physics_params.y));
    world.add_force(physics_params, dx*k, dy*k);
    return true;
}

pair<int, int> Zombie::astar_next_pos(World& world, int p_x, int p_y){
    int wid=world.width;
    auto idx=[&](int x, int y){return y*wid+x;};
    const int MAX_STEPS=int(sqrtf(aggression)*2);
    int sx=lround(physics_params.x), sy=lround(physics_params.y);
    int abs_dx=abs(sx-p_x), abs_dy=abs(sy-p_y);
    if(abs_dx+abs_dy>MAX_STEPS)return {-1, -1};//Ã·«∞ªÿÕÀ
    struct Node{
        int x, y, g, h;
        int f()const{return g+h;}
        bool operator<(const Node& other)const{
            return f()>other.f();
        }
    };
    if(++world.astar_token>=1000000){
        fill(world.astar_seen.begin(), world.astar_seen.end(), 0);
        world.astar_token=1;
    }
    int add_x[4]={1, -1, 0, 0}, add_y[4]={0, 0, 1, -1};
    if(abs_dx<abs_dy){
        for(int i=0; i<4; ++i)swap(add_x[i], add_y[i]);
    }
    priority_queue<Node> p_q;
    p_q.push({sx, sy, 0, abs_dx+abs_dy});
    int pos=idx(sx, sy);
    world.astar_seen[pos]=world.astar_token;
    world.astar_gscore[pos]=0;
    world.astar_parent[pos][0]=-1;
    while(!p_q.empty()){
        Node cur=p_q.top();
        p_q.pop();
        if(cur.x==p_x&&cur.y==p_y)break;
        if(cur.g>MAX_STEPS)break;
        for(int i=0; i<4; ++i){
            int n_x=cur.x+add_x[i], n_y=cur.y+add_y[i];
            if(n_x<0||n_x>=world.width||n_y<0||n_y>=world.height)continue;
            Cell& now_cell=world.get_cell(n_x, n_y);
            if(now_cell.info&(1<<CELL_WALL))continue;
            int now_pos=idx(n_x, n_y), new_g=cur.g+1;
            if(world.astar_seen[now_pos]!=world.astar_token||new_g<world.astar_gscore[now_pos]){
                world.astar_seen[now_pos]=world.astar_token;
                world.astar_gscore[now_pos]=new_g;
                world.astar_parent[now_pos][0]=cur.x;
                world.astar_parent[now_pos][1]=cur.y;
                int h=abs(n_x-p_x)+abs(n_y-p_y);
                p_q.push({n_x, n_y, new_g, h});
            }
        }
    }
    if(world.astar_parent[idx(p_x, p_y)][0]==-1)return {-1, -1};
    int c_x=p_x, c_y=p_y;
    while(true){
        array<int, 2>& now_parent=world.astar_parent[idx(c_x, c_y)];
        if(now_parent[0]==sx&&now_parent[1]==sy)break;
        int p_x=now_parent[0], p_y=now_parent[1];
        if(p_x==-1)return {-1, -1};
        c_x=p_x;
        c_y=p_y;
    }
    return {c_x, c_y};
}

void Zombie::move(World& world){
    if(stun_time>0)return;
    Human* target=nullptr;
    float min_dist_sq=aggression*(type==FAST_ZOMBIE?5:4);
    auto func=[&](Human* h){
        float u=h->physics_params.x-physics_params.x, v=h->physics_params.y-physics_params.y;
        float dist_sq=u*u+v*v;
        float t=max(1.0f-dist_sq/(aggression*4), 0.0f);
        h->pending_sanity_damage+=t*t*damage*accel*0.01f;
        if(dist_sq<min_dist_sq){
            min_dist_sq=dist_sq;
            target=h;
        }
        return true;
    };
    world.solve_grid_human(physics_params.x, physics_params.y, sqrtf(min_dist_sq), func);
    if(--path_update_time<=0){
        if(target){
            aggression+=0.1f;
            next_step=astar_next_pos(world, lround(target->physics_params.x), lround(target->physics_params.y));
        }else{
            next_step={-1, -1};
        }
        if(next_step.first==-1){
            const int dx[4]={1, -1, 0, 0}, dy[4]={0, 0, 1, -1};
            int weights[4]={25, 25, 25, 25};
            int t=5;
            float go_x=physics_params.x, go_y=physics_params.y;
            while(t--){
                int num=random(1, 100), sum=0, dir, add_weight=0;
                for(int i=0; i<4; ++i){
                    bool k=sum<num;
                    sum+=weights[i];
                    if(k&&num<=sum){
                        dir=i;
                    }else{
                        int remove_weight=min(weights[i]-10, 4);
                        weights[i]-=remove_weight;
                        add_weight+=remove_weight;
                    }
                }
                weights[dir]+=add_weight;
                go_x+=dx[dir]*0.25f;
                go_y+=dy[dir]*0.25f;
            }
            next_step={Math::clamp(int(lround(go_x)), 0, world.width-1), Math::clamp(int(lround(go_y)), 0, world.height-1)};
        }
        path_update_time=4+random(0, 2);
        offset_x=random(-40, 40)*0.01f;
        offset_y=random(-40, 40)*0.01f;
    }
    if(type==FAST_ZOMBIE||random(0, 2))move_distance(world, next_step.first+offset_x, next_step.second+offset_y);
    physics_params.x=Math::clamp(physics_params.x, 0.0f, float(world.width-1)), physics_params.y=Math::clamp(physics_params.y, 0.0f, float(world.height-1));
    if(target){
        target->combat_intensity+=0.02f;
        float u=target->physics_params.x-physics_params.x, v=target->physics_params.y-physics_params.y;
        float dist_sq=u*u+v*v;
        if(dist_sq<0.25f&&attack==0){
            int dmg=min(damage, target->health);
            float dist=max(sqrtf(dist_sq), EPSILON);
            attack=60;
            if(type==FAST_ZOMBIE){
                attack=50;
            }else if(type==TANK_ZOMBIE){
                attack=90;
            }
            target->hurt=6;
            target->health-=dmg;
            if(type==TANK_ZOMBIE){
                const float knockback=130.0f;
                world.add_force(target->physics_params, u/dist*knockback, v/dist*knockback);
            }
            world.emit_blood(target->physics_params.x, target->physics_params.y, u/dist, v/dist, dmg/3);
            world.add_sfx("human_hit", target->physics_params.x, target->physics_params.y, 70, 10);
        }else if(dist_sq<16.0f&&!random(0, 200)&&type!=INVISIBLE_ZOMBIE){
            world.add_sfx("zombie_breath", physics_params.x, physics_params.y, 70, 5);
        }
    }
}

bool Zombie::update(World& world){
    if(dead){
        if(death_time>0&&--death_time==0)return false;
        return true;
    }
    aggression=Math::clamp(aggression-0.01f, 7.0f, 25.0f);
    attack=max(attack-1, 0);
    stun_time=max(stun_time-1, 0);
    if(health<=0){
        dead=true;
        death_time=120;
        if(attacker_count==0)return true;
        int max_damage_idx=0;
        for(int i=1; i<attacker_count; ++i){
            if(attacker_damage[i].second>attacker_damage[max_damage_idx].second){
                max_damage_idx=i;
            }
        }
        for(Human& h : world.humans){
            if(h.id==attacker_damage[max_damage_idx].first){
                h.experience+=damage*accel;
                h.combat_intensity+=1.1f;
                break;
            }
        }
        if(random(0, 1))world.add_supply(physics_params.x, physics_params.y);
        return true;
    }
    move(world);
    return true;
}

