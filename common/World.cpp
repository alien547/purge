#include <sstream>
#include <fstream>
#include <algorithm>
#include "FastNoiseLite.h"
#include "Human.h"
#include "World.h"
#include "Tool/Random.h"
#include "Tool/Log.h"
using namespace std;
using namespace Settings;

void World::resize(int new_width, int new_height){
    if(new_width==width&&new_height==height)return;
    vector<Cell> new_cells(new_width*new_height);
    int copy_width=min(width, new_width), copy_height=min(height, new_height);
    for(int y=0; y<copy_height; ++y){
        for(int x=0; x<copy_width; ++x){
            new_cells[y*new_width+x]=cells[y*width+x];
        }
    }
    cells.swap(new_cells);
    width=new_width;
    height=new_height;
}

void World::generate_grass(int seed){
    if(seed==-1)seed=random(0, 0x7fffffff);
    FastNoiseLite noise;
    noise.SetSeed(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.08f);
    for(int y=0; y<height; ++y){
        for(int x=0; x<width; ++x){
            Cell& now_cell=get_cell(x, y);
            if(now_cell.info&(1<<CELL_WALL)||now_cell.info&(1<< CELL_WATER))continue;
            float val=noise.GetNoise(float(x), float(y));
            if(val<-0.3f){
                now_cell.grass_type=2;
            }else if(val<0.3f){
                now_cell.grass_type=1;
            }else{
                now_cell.grass_type=0;
            }
        }
    }
}

void World::generate_level(int seed){
    if(seed==-1)seed=random(0, 0x7fffffff);

    FastNoiseLite terrain, water, room_noise;
    terrain.SetSeed(seed);
    terrain.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    terrain.SetFrequency(0.08f);
    water.SetSeed(seed+1);
    water.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    water.SetFrequency(0.04f);
    water.SetFractalType(FastNoiseLite::FractalType_FBm);
    water.SetFractalOctaves(3);
    room_noise.SetSeed(seed+2);
    room_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    room_noise.SetFrequency(0.15f);

    for(Cell& c : cells)c.info=0;
    lights.clear();
    supplies.clear();

    for(int y=1; y<height-1; ++y){
        for(int x=1; x<width-1; ++x){
            float w=water.GetNoise(float(x), float(y));
            if(w>0.45f)get_cell(x, y).info|=1<<CELL_WATER;
        }
    }

    int room_size=Math::clamp(min(width, height)/7+random(-2, 2), 8ll, 24ll);
    struct Room{
        int x1, y1, x2, y2;
        int cx()const{
            return (x1+x2)/2;
        }
        int cy()const{
            return (y1+y2)/2;
        }
    };
    vector<Room> rooms;

    for(int gy=2; gy<height-2; gy+=room_size){
        for(int gx=2; gx<width-2; gx+=room_size){
            if(!random(0, 9))continue;
            int bx1=gx+1, by1=gy+1;
            int bx2=min(gx+room_size-1, width-2), by2=min(gy+room_size-1, height-2);
            int bw=bx2-bx1, bh=by2-by1;
            if(bw<7||bh<7)continue;
            float sf=randomf(0.75f, 0.95f);
            int aw=int(bw*sf), ah=int(bh*sf);
            int ax=bx1+random(0, bw-aw), ay=by1+random(0, bh-ah);
            int ax2=ax+aw, ay2=ay+ah;
            for(int y=ay; y<=ay2; ++y){
                for(int x=ax; x<=ax2; ++x){
                    get_cell(x, y).info|=1<<CELL_WALL;
                }
            }
            struct Sub{int x1, y1, x2, y2;};
            vector<Sub> subs;
            function<void(int, int, int, int, int)> bsp;
            bsp=[&](int x1, int y1, int x2, int y2, int d){
                int w=x2-x1, h=y2-y1;
                if(d>=4||(w<9&&h<9)){
                    subs.push_back({x1, y1, x2, y2});
                    return;
                }
                bool horiz;
                if(w>h*1.3f){
                    horiz=false;
                }else if(h>w*1.3f){
                    horiz=true;
                }else{
                    horiz=random(0, 1);
                }
                if(horiz){
                    int sy=y1+random(4, h-4);
                    bsp(x1, y1, x2, sy, d+1);
                    bsp(x1, sy, x2, y2, d+1);
                }else{
                    int sx=x1+random(4, w-4);
                    bsp(x1, y1, sx, y2, d+1);
                    bsp(sx, y1, x2, y2, d+1);
                }
            };
            bsp(ax, ay, ax2, ay2, 0);
            for(Sub& s : subs){
                for(int y=s.y1+1; y<s.y2; ++y){
                    for(int x=s.x1+1; x<s.x2; ++x){
                        get_cell(x, y).info&=~(1<<CELL_WALL);
                    }
                }
            }
            for(int i=0; i<subs.size(); ++i){
                for(int j=i+1; j<subs.size(); ++j){
                    Sub& a=subs[i]; Sub& b=subs[j];
                    if(a.x2==b.x1&&max(a.y1,b.y1)<min(a.y2,b.y2)){
                        int ylo=max(a.y1,b.y1)+1, yhi=min(a.y2,b.y2)-1;
                        if(yhi>=ylo)get_cell(a.x2, random(ylo, yhi)).info&=~(1<<CELL_WALL);
                    }else if(b.x2==a.x1&&max(a.y1,b.y1)<min(a.y2,b.y2)){
                        int ylo=max(a.y1,b.y1)+1, yhi=min(a.y2,b.y2)-1;
                        if(yhi>=ylo)get_cell(a.x1, random(ylo, yhi)).info&=~(1<<CELL_WALL);
                    }else if(a.y2==b.y1&&max(a.x1,b.x1)<min(a.x2,b.x2)){
                        int xlo=max(a.x1,b.x1)+1, xhi=min(a.x2,b.x2)-1;
                        if(xhi>=xlo)get_cell(random(xlo, xhi), a.y2).info&=~(1<<CELL_WALL);
                    }else if(b.y2==a.y1&&max(a.x1,b.x1)<min(a.x2,b.x2)){
                        int xlo=max(a.x1,b.x1)+1, xhi=min(a.x2,b.x2)-1;
                        if(xhi>=xlo)get_cell(random(xlo, xhi), a.y1).info&=~(1<<CELL_WALL);
                    }
                }
            }
            int doors=random(1, 2);
            for(int d=0; d<doors; ++d){
                int side=random(0, 3);
                if(side==0){
                    get_cell(random(ax+1, ax2-1), ay).info&=~(1<<CELL_WALL);
                }else if(side==1){
                    get_cell(random(ax+1, ax2-1), ay2).info&=~(1<<CELL_WALL);
                }else if(side==2){
                    get_cell(ax, random(ay+1, ay2-1)).info&=~(1<<CELL_WALL);
                }else if(side==3){
                    get_cell(ax2, random(ay+1, ay2-1)).info&=~(1<<CELL_WALL);
                }
            }
            rooms.push_back({ax, ay, ax2, ay2});
        }
    }
    if(rooms.size()>=2){
        int n=rooms.size();
        vector<int> parent(n);
        for(int i=0; i<n; ++i)parent[i]=i;
        auto find=[&](int x){
            while(parent[x]!=x){
                parent[x]=parent[parent[x]];
                x=parent[x];
            }
            return x;
        };

        struct Edge{int a, b, d2;};
        vector<Edge> edges;
        for(int i=0;i<n;++i){
            for(int j=i+1;j<n;++j){
                int dx=rooms[i].cx()-rooms[j].cx(), dy=rooms[i].cy()-rooms[j].cy();
                edges.push_back({i, j, dx*dx+dy*dy});
            }
        }
        sort(edges.begin(), edges.end(), [](const Edge&a, const Edge&b){return a.d2<b.d2;});
        auto carve_corridor=[&](const Room& a, const Room& b){
            int ax=a.cx(), ay=a.cy(), bx=b.cx(), by=b.cy();
            auto dig=[&](int x, int y){
                if(x<1||x>=width-1||y<1||y>=height-1)return;
                Cell& c=get_cell(x, y);
                c.info&=~(1<<CELL_WALL);
            };
            if(random(0, 1)){
                for(int x=min(ax, bx); x<=max(ax, bx); ++x)dig(x, ay);
                for(int y=min(ay, by); y<=max(ay, by); ++y)dig(bx, y);
            }else{
                for(int y=min(ay, by); y<=max(ay, by); ++y)dig(ax, y);
                for(int x=min(ax, bx); x<=max(ax, bx); ++x)dig(x, by);
            }
        };

        int used=0;
        for(Edge& e : edges){
            int ra=find(e.a), rb=find(e.b);
            if(ra==rb)continue;
            parent[ra]=rb;
            carve_corridor(rooms[e.a], rooms[e.b]);
            if(++used==n-1)break;
        }
        for(int i=0; i<n/4; ++i){
            int a=random(0, n-1), b=random(0, n-1);
            if(a!=b)carve_corridor(rooms[a], rooms[b]);
        }
    }

    generate_grass();

    auto add_random_light=[&](float x, float y){
        Light lamp;
        lamp.color={Uint8(220+random(-20, 20)), Uint8(220+random(-20, 20)), Uint8(220+random(-20, 20))};
        lamp.r=randomf(3.5f, 6.5f);
        lamp.intensity=randomf(0.5f, 1.2f);
        lamp.dir_angle=randomf(0, 2*PI);
        lamp.cone_angle=(!random(0, 4))?randomf(PI/3, PI/2):PI;
        lamp.display=-1;
        lamp.x=x;
        lamp.y=y;
        lights.push_back(lamp);
    };

    vector<uint8_t> in_room(width*height, 0);
    for(Room& r : rooms){
        for(int y=r.y1; y<=r.y2; ++y){
            for(int x=r.x1; x<=r.x2; ++x){
                in_room[y*width+x]=1;
            }
        }
    }

    FastNoiseLite light_noise;
    light_noise.SetSeed(seed+5);
    light_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    light_noise.SetFrequency(0.12f);

    for(int y=2; y<height-2; ++y){
        for(int x=2; x<width-2; ++x){
            Cell& c=get_cell(x, y);
            if(c.info&(1<<CELL_WALL)||c.info&(1<<CELL_WATER))continue;
            int chance;
            float density=light_noise.GetNoise(float(x), float(y));
            chance=(density>0.5f)?40:(density>0.1f)?100:320;
            if(in_room[y*width+x])chance/=2;
            if(random(0, chance-1))continue;
            bool occupied=false;
            for(Light& l : lights){
                float dx=l.x-x, dy=l.y-y;
                if(dx*dx+dy*dy<1.0f){
                    occupied=true;
                    break;
                }
            }
            if(occupied)continue;
            add_random_light(x+randomf(-0.4f, 0.4f), y+randomf(-0.4f, 0.4f));
        }
    }

    for(Room& r : rooms){
        if(random(0, 2))continue;
        float sx=r.x1+(r.x2-r.x1)*randomf(0.1f, 0.9f), sy=r.y1+(r.y2-r.y1)*randomf(0.1f, 0.9f);
        if(get_cell(sx, sy).info&(1<<CELL_WALL))continue;
        add_supply(sx, sy, false, true, "军备箱", nullptr, int(sqrtf(randomf(10, 40))));
    }
}

float World::get_vel_scale(int x, int y){
    float scale=1;
    if(rain_time>0)scale/=1.3;
    Cell& now_cell=get_cell(x, y);
    if(now_cell.info&(1<<CELL_WATER))scale/=1.6;
    return scale;
}

Supply* World::get_supply(uint64_t id){
    for(unique_ptr<Supply>& s : supplies){
        if(s->id==id)return s.get();
    }
    return nullptr;
}

void World::trigger_slowmo(float target, float duration){
    #ifndef SERVER_BUILD
    time_scale_target=target;
    slowmo_time=duration;
    #endif
}

float World::light_intensity_at(const Light& l, float x, float y){
    float dx=x-l.x, dy=y-l.y;
    float dist_sq=dx*dx+dy*dy;
    float l_sq=l.r*l.r;
    if(dist_sq>l_sq)return 0.0f;
    if(dist_sq<EPSILON)return l.intensity;

    const int RAYS=3;
    int hits=0;
    float dist=sqrtf(dist_sq);
    float nx=dx/dist, ny=dy/dist;
    float px=-ny, py=nx;
    float spread=min(dist*0.4f, 2.0f);
    for(int i=0; i<RAYS; ++i){
        float t=2.0f*i/(RAYS-1)-1.0f;
        float sx=x+px*spread*t, sy=y+py*spread*t;
        if(sx<0||sx>=width||sy<0||sy>=height||has_wall(l.x, l.y, sx, sy, true))++hits;
    }
    float occlusion=1.0f-float(hits)/RAYS;
    if(occlusion<=0.0f)return 0.0f;
    float attenuation=l_sq/(l_sq+dist_sq);
    float intensity=attenuation*l.intensity*occlusion;
    if(l.cone_angle<PI-EPSILON){
        float angle=atan2f(dy, dx);
        float diff=fabsf(angle-l.dir_angle);
        if(diff>PI)diff=2.0f*PI-diff;
        if(diff>=l.cone_angle)return 0.0f;
        float cos_theta=cosf(diff);
        intensity*=cos_theta*cos_theta;
    }
    return intensity;
}

unique_ptr<Item>& World::get_inv_item(int slot, Human* human, Supply* supply){
    if(slot<NOW_USE_SIZE)return human->now_use[slot];
    if(slot<NOW_USE_SIZE+BAG_SIZE)return human->bag[slot-NOW_USE_SIZE];
    return supply->items[slot-NOW_USE_SIZE-BAG_SIZE];
}

InventoryResult World::execute_inventory_action(uint64_t human_id, const InventoryAction& act){
    InventoryResult result;
    Human* human=nullptr;
    Supply* supply=get_supply(act.supply_id);
    for(Human& h : humans){
        if(h.id==human_id){
            human=&h;
            break;
        }
    }
    if(!human)return result;
    if(supply&&(fabs(human->physics_params.x-supply->x)>1||fabs(human->physics_params.y-supply->y)>1)){
        return result;
    }
    unique_ptr<Item>& src=get_inv_item(act.src_slot, human, supply);
    switch(act.action){
        case INV_SWAP:{
            unique_ptr<Item>& dst=get_inv_item(act.dst_slot, human, supply);
            if(!((src->type.first==ITEM_WEAPON||src->type.first==ITEM_EMPTY||act.dst_slot>=NOW_USE_SIZE)
            &&(dst->type.first==ITEM_WEAPON||dst->type.first==ITEM_EMPTY||act.src_slot>=NOW_USE_SIZE)))return result;
            swap(src, dst);
            break;
        }
        case INV_GET:{
            if(src->type.first!=ITEM_AMMO)return result;
            Ammo* a=(Ammo*)(src.get());
            human->ammo[a->type.second]+=a->count;
            src=unique_ptr<Item>(new Item());
            break;
        }
        case INV_USE:
            if(src->type.first!=ITEM_CONS)return result;
            if(src->type.second==CONS_MEDKIT){
                human->health=min(int32_t(human->health+human->max_health/3+random(-3, 3)), human->max_health);
            }else if(src->type.second==CONS_PSYCH){
                human->sanity=min(human->sanity+40.0f+randomf(-2, 2), 100.0f);
            }
            src=unique_ptr<Item>(new Item());
            break;
        case INV_AWAY:
            add_supply(human->physics_params.x, human->physics_params.y, true, false, "丢弃物", move(src));
            src=unique_ptr<Item>(new Item());
            break;
    }
    clean_supply(supply);
    if(supply){
        result.changed_supply=true;
        result.supply=supply;
    }
    return result;
}

void World::save_map(){
    if(file_name.empty())return;
    string temp_path="../"+TEMP_PATH+file_name+".tmp", map_path="../"+DATA_PATH+MAPS_PATH+file_name+".map";
    ofstream new_map(temp_path.c_str(), ios::binary);
    if(new_map.is_open()){
        ostringstream mem(ios::binary);
        save_string(mem, name);
        save_value(mem, width, height, safe, next_supply_id);
        for(int j=0; j<height; ++j){
            for(int i=0; i<width; ++i){
                save_value(mem, get_cell(i, j).info);
            }
        }
        save_value(mem, uint16_t(supplies.size()));
        for(unique_ptr<Supply>& s : supplies){
            save_value(mem, s->x, s->y, s->open, s->lasting, s->id);
            save_string(mem, s->name);
            save_value(mem, uint16_t(s->items.size()));
            for(unique_ptr<Item>& i : s->items){
                save_item(mem, i.get());
            }
        }
        save_value(mem, uint16_t(zombies.size()));
        for(Zombie& z : zombies)save_value(mem, z.type, z.attack, z.damage, z.health, z.aggression, z.physics_params.x, z.physics_params.y, z.accel, z.dead, z.death_time);
        save_value(mem, uint16_t(exits.size()));
        for(Exit& exi : exits){
            save_value(mem, exi.x, exi.y, exi.go_x, exi.go_y);
            save_string(mem, exi.go_name);
        }
        save_value(mem, uint16_t(lights.size()));
        for(Light& l : lights)save_value(mem, l.x, l.y, l.r, l.intensity, l.dir_angle, l.cone_angle, l.display, uint8_t(l.color.r), uint8_t(l.color.g), uint8_t(l.color.b));
        string raw=mem.str();
        vector<char> compressed=compress_data(raw.data(), int(raw.size()));
        new_map.write(compressed.data(), compressed.size());
        new_map.close();
        remove(map_path.c_str());
        rename(temp_path.c_str(), map_path.c_str());
    }
}

bool World::load_map(const string& new_file_name, int x, int y){
    string full_file_name="../"+DATA_PATH+MAPS_PATH+new_file_name+".map";
    ifstream new_map(full_file_name, ios::binary);
    if(new_map.is_open()){
        #ifndef SERVER_BUILD
        stop_all_sfx();
        #endif
        vector<char> packet(istreambuf_iterator<char>(new_map), {});
        string raw;
        if(!decompress_data(packet, raw))return false;
        istringstream mem(raw, ios::binary);
        file_name=new_file_name;
        load_string(mem, name);
        load_value(mem, width, height, safe, next_supply_id);
        cells.assign(width*height, Cell());
        astar_parent.assign(width*height, {-1, -1});
        astar_gscore.assign(width*height, 0);
        astar_seen.assign(width*height, 0);
        astar_token=1;
        for(int j=0; j<height; ++j){
            for(int i=0; i<width; ++i){
                load_value(mem, get_cell(i, j).info);
            }
        }
        supplies.clear();
        zombies.clear();
        exits.clear();
        lights.clear();
        uint16_t size;
        load_value(mem, size);
        while(size--){
            uint16_t count;
            unique_ptr<Supply> temp(new Supply());
            load_value(mem, temp->x, temp->y, temp->open, temp->lasting, temp->id);
            load_string(mem, temp->name);
            load_value(mem, count);
            while(count--){
                temp->items.push_back(load_item(mem));
            }
            supplies.push_back(move(temp));
        }
        load_value(mem, size);
        while(size--){
            Zombie temp;
            load_value(mem, temp.type, temp.attack, temp.damage, temp.health, temp.aggression, temp.physics_params.x, temp.physics_params.y, temp.accel, temp.dead, temp.death_time);
            zombies.push_back(temp);
        }
        load_value(mem, size);
        while(size--){
            Exit temp;
            load_value(mem, temp.x, temp.y, temp.go_x, temp.go_y);
            load_string(mem, temp.go_name);
            exits.push_back(temp);
        }
        load_value(mem, size);
        while(size--){
            Light temp;
            uint8_t r, g, b;
            load_value(mem, temp.x, temp.y, temp.r, temp.intensity, temp.dir_angle, temp.cone_angle, temp.display, r, g, b);
            temp.color.r=Uint8(r);
            temp.color.g=Uint8(g);
            temp.color.b=Uint8(b);
            lights.push_back(temp);
        }
        generate_grass();
        tasks.clear();
        int total_size=width*height;
        max_zombie_size=max(total_size/25, 1);
        max_supply_size=max(total_size/64, 1);
        zombies.reserve(max_zombie_size);
        supplies.reserve(max_supply_size);
        for(Human& h : humans){
            h.physics_params.x=x;
            h.physics_params.y=y;
        }
        last_spawn_x=x;
        last_spawn_y=y;
        new_map.close();
        debug("进入 "+new_file_name, DEBUG_INFO);
        return true;
    }else{
        #ifndef EDITOR_BUILD
        debug("无法打开 "+full_file_name, DEBUG_ERROR);
        #endif
        return false;
    }
}

ItemData World::item_to_net(const Item* item){
    ItemData net;
    net.type=item->type.first;
    net.subtype=item->type.second;
    net.value=item->value;
    strncpy(net.name, item->name.c_str(), MAX_ITEM_NAME_SIZE-1);
    net.name[MAX_ITEM_NAME_SIZE-1]='\0';
    return net;
}

unique_ptr<Item> World::net_to_item(const ItemData& net){
    unique_ptr<Item> result;
    switch(net.type){
        case ITEM_WEAPON:
            result.reset(new Weapon());
            break;
        case ITEM_AMMO:
            result.reset(new Ammo());
            break;
        case ITEM_CONS:
        case ITEM_EMPTY:
        default:
            result.reset(new Item());
            break;
    }
    result->type={net.type, net.subtype};
    result->name=net.name;
    result->value=net.value;
    return result;
}

void World::add_sfx(const string& name, float x, float y, float base_vol, float max_dist){
    #ifdef SERVER_BUILD
    SfxEvent ev;
    strncpy(ev.name, name.c_str(), MAX_SFX_NAME_SIZE-1);
    ev.name[MAX_SOUND_NAME_SIZE-1]='\0';
    ev.x=x;
    ev.y=y;
    ev.base_vol=base_vol;
    ev.max_dist=max_dist;
    ENetPacket* packet=enet_packet_create(&ev, sizeof(ev), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server_host, 1, packet);
    #else
    play_sfx_at(name, x, y, base_vol, max_dist);
    #endif
}

void World::add_music(const string& name, ENetPeer* peer){
    #ifdef SERVER_BUILD
    MusicEvent ev;
    strncpy(ev.name, name.c_str(), MAX_MUSIC_NAME_SIZE-1);
    ev.name[MAX_SOUND_NAME_SIZE-1]='\0';
    ENetPacket* packet=enet_packet_create(&ev, sizeof(ev), ENET_PACKET_FLAG_RELIABLE);
    if(peer==nullptr){
        enet_host_broadcast(server_host, 1, packet);
    }else{
        enet_peer_send(peer, 1, packet);
    }
    #else
    play_music_at(name);
    #endif
}

void World::emit_particle(float x, float y, pair<int, int> speed_clamp, pair<int, int> max_life_clamp, pair<int, int> size_clamp, int count, ColorRGB color){
    #ifdef SERVER_BUILD
    ParticleEvent ev;
    ev.x=x;
    ev.y=y;
    ev.speed_min=speed_clamp.first;
    ev.speed_max=speed_clamp.second;
    ev.max_life_min=max_life_clamp.first;
    ev.max_life_max=max_life_clamp.second;
    ev.size_min=size_clamp.first;
    ev.size_max=size_clamp.second;
    ev.count=count;
    ENetPacket* packet=enet_packet_create(&ev, sizeof(ev), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server_host, 1, packet);
    #else
    particle_system.emit_explo(x, y, speed_clamp, max_life_clamp, size_clamp, count, color);
    #endif
}

void World::emit_blood(float x, float y, float dir_x, float dir_y, int count){
    #ifdef SERVER_BUILD
    BloodEvent ev;
    ev.x=x;
    ev.y=y;
    ev.dir_x=dir_x;
    ev.dir_y=dir_y;
    ev.count=count;
    ENetPacket* packet=enet_packet_create(&ev, sizeof(ev), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server_host, 1, packet);
    #else
    float base_angle=(dir_x==0.0f&&dir_y==0.0f)?randomf(0, 2*PI):atan2f(dir_y, dir_x);
    while(count--){
        float angle=base_angle+randomf(-1.0f, 1.0f);
        float dist=randomf(0.3f, 1.2f);
        BloodStain b;
        b.x=x+cosf(angle)*dist;
        b.y=y+sinf(angle)*dist;
        b.color.r=Uint8(random(120, 180));
        b.color.g=Uint8(random(10, 25));
        b.color.b=Uint8(random(10, 25));
        b.alpha=255;
        b.display=random(120, 150);
        blood_stains.push_back(b);
    }
    #endif
}

void World::env_update(){
    minute+=0.06*time_scale;
    if(minute>=60){
        minute-=60;
        ++hour;
    }
    if(hour>=24){
        hour-=24;
        ++day;
    }
    if(shake_intensity>EPSILON){
        shake_intensity-=0.002f;
        shake_intensity*=0.92f;
    }else{
        shake_intensity=0;
    }
    rain_time=max(rain_time-1, 0);
    sky_flash=max(sky_flash-1, 0);
    if(!random(0, 500+(rain_time>0?500:0))){
        rain_time+=350;
        if(rain_time>400)rain_heavy=true;
    }
    if(rain_time>0){
        if(rain_time<250)rain_heavy=false;
        if(!random(0, 120+(sky_flash>0?120:0))){
            sky_flash+=4;
            auto func=[&](){
                add_sfx("thunder", -1, -1, 100, -1);
            };
            tasks.schedule(random(2, 11)*400, func);
        }
    }
    if(rain_time>0){
        play_sfx_info|=1<<SOUND_RAIN;
    }else{
        play_sfx_info&=~(1<<SOUND_RAIN);
    }
    light_update();

    float time=hour+minute/60.0f;
    int idx=0;
    while(idx<sky_keys.size()-2&&sky_keys[idx+1].hour<=time)++idx;
    const SkyKey& a=sky_keys[idx], & b=sky_keys[idx+1];
    float span=b.hour-a.hour;
    float t=Math::clamp(span>EPSILON?(time-a.hour)/span:0.0f, 0.0f, 1.0f);
    float target_i=a.intensity*(1.0f-t)+b.intensity*t;
    ColorRGB target_c={
        Uint8(a.color.r*(1.0f-t)+b.color.r*t),
        Uint8(a.color.g*(1.0f-t)+b.color.g*t),
        Uint8(a.color.b*(1.0f-t)+b.color.b*t),
    };
    if(rain_time>0)target_i*=rain_heavy?0.55f:0.8f;
    env_light_intensity_target=target_i;

    env_light_intensity+=(env_light_intensity_target-env_light_intensity)*0.05f;
    env_light_color.r=Uint8(env_light_color.r+(target_c.r-env_light_color.r)*0.05f);
    env_light_color.g=Uint8(env_light_color.g+(target_c.g-env_light_color.g)*0.05f);
    env_light_color.b=Uint8(env_light_color.b+(target_c.b-env_light_color.b)*0.05f);
    for(Human& h : humans){
        if(update_physics(h.physics_params))h.stun_time=20;
    }
    for(Zombie& z : zombies){
        if(update_physics(z.physics_params))z.stun_time=20;
    }
    #ifndef SERVER_BUILD
    for(int i=0; i<blood_stains.size(); ++i){
        BloodStain& b=blood_stains[i];
        if(--b.display<=0){
            swap(blood_stains[i--], blood_stains.back());
            blood_stains.pop_back();
            continue;
        }
        b.alpha=b.alpha*b.display/(b.display+1);
    }
    #endif
    resolve_collisions();
    rebuild_grid();
    tasks.update();
}

