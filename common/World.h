#ifndef World_H
#define World_H

#include <vector>
#include <unordered_map>
#include <array>
#include <string>
#include <memory>
#include <SDL.h>
#include "Tool/Serialize.h"
#include "Zombie.h"
#include "Human.h"
#include "WorldTypes.h"
#include "Network.h"
#include "Settings.h"
#include "TaskScheduler.h"

#ifdef SERVER_BUILD
#include <enet/enet.h>
#else
#include "ParticleSystem.h"
#endif

struct GridCell{
    std::vector<Zombie*> zombies;
    std::vector<Human*> humans;
    std::vector<Exit*> exits;
    std::vector<Supply*> supplies;
};

struct InventoryResult{
    bool changed_supply=false;
    Supply* supply=nullptr;
};

class GameState;

struct _ENetHost;
struct _ENetPeer;
typedef struct _ENetHost ENetHost;
typedef struct _ENetPeer ENetPeer;

//类似 敌人、物品、子弹等 “客观存在”的 数据及逻辑  
class World{
    public:
        int width=10, height=10, day=1, hour=8, rain_time=0, sky_flash=0;
        float shake_intensity=0, env_light_intensity=0, env_light_intensity_target=0;
        ColorRGB env_light_color={255, 255, 255};
        double minute=0;
        std::string file_name, name;
        bool safe=false, rain_heavy=false, is_server=false;
        uint64_t now_frame=0;
        int8_t play_sfx_info=0;

        float time_scale=1.0f, time_scale_target=1.0f, slowmo_time=0;

        std::vector<SkyKey> sky_keys={
            {0.0f,  0.08f, {40, 50, 110}},
            {5.0f,  0.35f, {80, 90, 150}},
            {7.0f,  0.75f, {200, 160, 130}},
            {12.0f, 1.10f, {255, 250, 240}},
            {17.0f, 0.75f, {255, 200, 140}},
            {19.0f, 0.40f, {220, 130, 100}},
            {21.0f, 0.28f, {80, 90, 150}},
            {24.0f, 0.08f, {40, 50, 110}},
        };

        std::vector<std::array<int, 2>> astar_parent;
        std::vector<int> astar_gscore, astar_seen;
        int astar_token=1;

        int max_zombie_size=0, max_supply_size=0, last_spawn_x=0, last_spawn_y=0;

        uint64_t next_supply_id=1;

        std::unordered_map<uint64_t, GridCell> entity_grid;
        const float GRID_SIZE=2.0f;

        TaskScheduler tasks;

        #ifdef SERVER_BUILD
        ENetHost* server_host=nullptr;
        #else
        std::vector<Sfx> sfx;
        ParticleSystem particle_system;
        std::vector<BloodStain> blood_stains;
        #endif

        std::vector<Exit> exits;
        std::vector<Light> lights;
        std::vector<std::unique_ptr<Supply>> supplies;
        std::vector<Bullet> bullets;
        std::vector<Zombie> zombies;
        std::vector<Human> humans;
        std::vector<Cell> cells;
        std::vector<Weapon> weapons={{"刀", 50, Settings::MELEE, -1, -1, -1, 18, -1, 7.5f, -1, false, "knife"}, {"手枪", 180, Settings::PISTOL, 8, 8, 3.2f, 26, 14, 4.5f, 1.2f, false, "pistol"},
        {"冲锋枪", 300, Settings::SMG, 25, 25, 3.2f, 15, 12, 1.4f, 1.2f, false, "smg"}, {"霰弹枪", 350, Settings::SHOTGUN, 4, 4, 2.8f, 24, 8, 10.5f, 1.05f, false, "shotgun"},
        {"火箭筒", 500, Settings::RPG, 1, 1, 3.6f, 5, 21, 13.0f, 0.7f, true, "rpg"}, {"手", -1, Settings::MELEE, -1, -1, -1, 4, -1, 4.5f, -1, false, "hand"}};

        void resize(int new_width, int new_height);
        std::pair<float, float> select_weighted_position();
        void generate_grass(int seed=-1);
        void generate_level(int seed=-1);
        float get_vel_scale(int x, int y);
        Supply* get_supply(uint64_t id);
        void trigger_slowmo(float target, float duration);
        float light_intensity_at(const Light& l, float x, float y);
        uint64_t grid_key(float x, float y);
        std::vector<const GridCell*> get_grids(float x, float y, float r);
        void solve_grid_zombie(float x, float y, float r, std::function<bool(Zombie*)> func);
        void solve_grid_human(float x, float y, float r, std::function<bool(Human*)> func);
        void solve_grid_exit(float x, float y, float r, std::function<bool(Exit*)> func);
        void solve_grid_supply(float x, float y, float r, std::function<bool(Supply*)> func);
        void rebuild_grid();
        void add_force(PhysicsParams& params, float force_x, float force_y);
        bool update_physics(PhysicsParams& params);
        bool has_wall(float x1, float y1, float x2, float y2, bool skip=false);
        bool is_point_visible(float x1, float y1, float x2, float y2, float see_len, float direction_x, float direction_y, float see_angle, VisibilityStamp* vis=nullptr);
        void resolve_collisions();
        std::unique_ptr<Item>& get_inv_item(int slot, Human* human, Supply* supply=nullptr);
        void clean_supply(Supply* supply);
        InventoryResult execute_inventory_action(uint64_t human_id, const InventoryAction& action);
        void save_map();
        bool load_map(const std::string& new_file_name, int x, int y);
        ItemData item_to_net(const Item* item);
        std::unique_ptr<Item> net_to_item(const ItemData& net);
        void add_explo(float x, float y, float r, int display, int damage, int id, int recursion_level);
        void add_light(float x, float y, float r, float intensity, float dir_angle, float cone_angle, int display, ColorRGB color);
        void add_supply(float x=-1, float y=-1, bool open=false, bool lasting=false, const std::string& name="补给", std::unique_ptr<Item> item=nullptr, int count=1);
        void add_bullet(float x, float y, float vel_x, float vel_y, int damage, int health, int id, bool can_explo);
        void add_zombie();
        void add_sfx(const std::string& name, float x, float y, float base_vol, float max_dist);
        void add_music(const std::string& name, ENetPeer* peer);
        void emit_particle(float x, float y, std::pair<int, int> speed_clamp, std::pair<int, int> max_life_clamp, std::pair<int, int> size_clamp, int count, ColorRGB color);
        void emit_blood(float x, float y, float dir_x, float dir_y, int count);
        void env_update();
        void light_update();
        void bullet_update();
        void zombie_update();

        #ifndef SERVER_BUILD
        uint8_t ir_of_cell(const Cell& c);
        uint8_t ir_of_zombie(const Zombie& z);
        std::pair<char, SDL_Color> get_zombie_appearance(Zombie& z);
        void draw_map(int start_x, int end_x, int start_y, int end_y, float view_start_x, float view_start_y, bool global_light=false, Human* p=nullptr, int self=-1);
        void draw_shadows(float view_start_x, float view_start_y, Human* p, int self);
        void update_persistent_sounds();
        int play_sfx_at(const std::string& name, float x, float y, float base_vol, float max_dist);
        void play_music_at(const std::string& name);
        void update_audio_distances(float listener_x, float listener_y);
        void stop_all_sfx();
        std::array<int16_t, 3> get_light_color(float x, float y);
        #endif

        inline Cell& get_cell(int x, int y){return cells[y*width+x];}

        template<typename Stream>
        void save_item(Stream& data, Item* temp){
            save_value(data, temp->type.first, temp->type.second);
            save_string(data, temp->name);
            save_value(data, temp->value);
            if(temp->type.first==Settings::ITEM_WEAPON){
                Weapon* w=(Weapon*)(temp);
                save_value(data, w->now_ammo, w->max_ammo, w->load_speed, w->damage, w->health, w->attacke, w->vel, w->can_explo);
                save_string(data, w->image_path);
            }else if(temp->type.first==Settings::ITEM_AMMO){
                Ammo* a=(Ammo*)(temp);
                save_value(data, a->count);
            }
        }

        template<typename Stream>
        std::unique_ptr<Item> load_item(Stream& data){
            int type;
            load_value(data, type);
            if(type==Settings::ITEM_WEAPON){
                std::unique_ptr<Weapon> temp(new Weapon());
                temp->type.first=type;
                load_value(data, temp->type.second);
                load_string(data, temp->name);
                load_value(data, temp->value, temp->now_ammo, temp->max_ammo, temp->load_speed,
                temp->damage, temp->health, temp->attacke, temp->vel, temp->can_explo);
                load_string(data, temp->image_path);
                return move(temp);
            }else if(type==Settings::ITEM_AMMO){
                std::unique_ptr<Ammo> temp(new Ammo());
                temp->type.first=type;
                load_value(data, temp->type.second);
                load_string(data, temp->name);
                load_value(data, temp->value, temp->count);
                return move(temp);
            }else{
                std::unique_ptr<Item> temp(new Item());
                temp->type.first=type;
                load_value(data, temp->type.second);
                load_string(data, temp->name);
                load_value(data, temp->value);
                return move(temp);
            }
        }
};

#endif

