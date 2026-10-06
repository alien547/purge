#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include "WorldTypes.h"
#include "Network.h"

class World;

//»À¿‡ 
class Human{
    public:
        Human();

        std::string name="Œ¥÷™";
        std::array<std::unique_ptr<Item>, Settings::NOW_USE_SIZE> now_use;
        std::array<std::unique_ptr<Item>, Settings::BAG_SIZE> bag;
        const KeyState* k_s=nullptr;
        std::function<void(Supply*)> on_supply_open;
        std::function<void()> on_death;
        PhysicsParams physics_params={70, 0, 0, 0, 0, 0, 0, 0.4f, 0.8f};
        VisibilityStamp vis;
        uint64_t id=0;
        int32_t health=100, max_health=100, experience=0, money=0, upgrade_health=400;
        int attacke=0, last_weapon_type=0, weapon_type=0, search_item=0, go_exit=0, hurt=0, rest=0, total_kills=0,
        slash_time=0, footstep_time=0, weapon_switch_time=0, stun_time=0, nv_boot_time=0;
        int ammo[Settings::AMMO_TYPE_SIZE]={0, 0, 0, 0};
        float flash_battery=100, stamina=100, max_stability=100, stability=100, slash_angle=0, sanity=100, pending_sanity_damage=0;
        std::pair<float, float> direction={1, 0};
        bool flash=false, run=false, night_vision=false;

        Weapon* get_now_weapon(World& world);
        void turn_towards(float target_angle);
        void aim();
        void move(World& world);
        void operate(World& world);
        void update(World& world);
        void apply_input(World& world, const KeyState& input);
        void respawn();
};

#endif

