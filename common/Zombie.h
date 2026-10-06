#ifndef ZOMBIE_H
#define ZOMBIE_H

#include "WorldTypes.h"
#include "Settings.h"

class World;

//ɥʬ 
class Zombie{
    public:
        Zombie();
        Zombie(int d, int h, int type, float x, float y, float a);

        PhysicsParams physics_params={60, 0, 0, 0, 0, 0, 0, 0.4f, 0.8f};
        VisibilityStamp vis;
        std::pair<uint64_t, int> attacker_damage[Settings::MAX_ATTACKER_SIZE];
        std::pair<int, int> next_step={-1, -1};
        int path_update_time=0, attack=0, damage=0, health=0, type=Settings::NORMAL_ZOMBIE, death_time=0, stun_time=0, attacker_count=0;
        float accel=0, aggression=0, offset_x=0, offset_y=0;
        bool dead=false;

        void add_attacker_damage(uint64_t id, int damage);
        bool move_distance(World& world, float p_x, float p_y);
        std::pair<int, int> astar_next_pos(World& world, int p_x, int p_y);
        void move(World& world);
        bool update(World& world);
};

#endif

