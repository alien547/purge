#include <cmath>
#include "World.h"
using namespace std;

uint64_t World::grid_key(float x, float y){
    int g_x=int(floor(x/GRID_SIZE)), g_y=int(floor(y/GRID_SIZE));
    return (uint64_t(g_x)<<32)|uint64_t(g_y);
}

vector<const GridCell*> World::get_grids(float x, float y, float r){
    vector<const GridCell*> grids;
    int range=int(ceil(r/GRID_SIZE));
    int g_x=int(floor(x/GRID_SIZE)), g_y=int(floor(y/GRID_SIZE));
    for(int dx=-range; dx<=range; ++dx){
        for(int dy=-range; dy<=range; ++dy){
            uint64_t key=grid_key((g_x+dx)*GRID_SIZE, (g_y+dy)*GRID_SIZE);
            auto it=entity_grid.find(key);
            if(it==entity_grid.end())continue;
            grids.push_back(&(it->second));
        }
    }
    return grids;
}

void World::solve_grid_zombie(float x, float y, float r, function<bool(Zombie*)> func){
    vector<const GridCell*> grids=get_grids(x, y, r);
    for(const GridCell* g : grids){
        for(Zombie* z : g->zombies){
            if(!func(z))return;
        }
    }
}

void World::solve_grid_human(float x, float y, float r, function<bool(Human*)> func){
    vector<const GridCell*> grids=get_grids(x, y, r);
    for(const GridCell* g : grids){
        for(Human* h : g->humans){
            if(!func(h))return;
        }
    }
}

void World::solve_grid_exit(float x, float y, float r, function<bool(Exit*)> func){
    vector<const GridCell*> grids=get_grids(x, y, r);
    for(const GridCell* g : grids){
        for(Exit* ex : g->exits){
            if(!func(ex))return;
        }
    }
}

void World::solve_grid_supply(float x, float y, float r, function<bool(Supply*)> func){
    vector<const GridCell*> grids=get_grids(x, y, r);
    for(const GridCell* g : grids){
        for(Supply* s : g->supplies){
            if(!func(s))return;
        }
    }
}

void World::rebuild_grid(){
    entity_grid.clear();
    for(Zombie& z : zombies){
        uint64_t key=grid_key(z.physics_params.x, z.physics_params.y);
        entity_grid[key].zombies.push_back(&z);
    }
    for(Human& h : humans){
        uint64_t key=grid_key(h.physics_params.x, h.physics_params.y);
        entity_grid[key].humans.push_back(&h);
    }
    for(Exit& ex : exits){
        uint64_t key=grid_key(ex.x, ex.y);
        entity_grid[key].exits.push_back(&ex);
    }
    for(unique_ptr<Supply>& s : supplies){
        uint64_t key=grid_key(s->x, s->y);
        entity_grid[key].supplies.push_back(s.get());
    }
}

