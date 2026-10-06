#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <vector>
#include <utility>
#include "WorldTypes.h"

class ParticleSystem{
    public:
        ParticleSystem();

        struct Particle{
            ColorRGB color;
            float x, y, v_x, v_y, life, max_life, size;
            bool active;
        };

        static constexpr int MAX_PARTICLE_SIZE=1500;

        Particle particles[MAX_PARTICLE_SIZE];
        std::vector<int> free_list;

        void update(int max_x, int max_y);
        void render(float view_x, float view_y);
        void emit_explo(float x, float y, std::pair<int, int> speed_clamp, std::pair<int, int> max_life_clamp, std::pair<int, int> size_clamp, int count, ColorRGB color);
};

#endif

