#ifndef WORLD_TYPES_H
#define WORLD_TYPES_H

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include "Settings.h"

struct ColorRGB{
    uint8_t r, g, b;
};

struct VisibilityStamp{
    uint64_t frame=0;
    bool visible=false;
};

struct Item{//物品
    Item():name("空"), value(-1), type({Settings::ITEM_EMPTY, 0}){}
    Item(const std::string& n, int v, std::pair<int, int> t):name(n), value(v), type(t){}
    std::string name;
    std::pair<int, int> type;
    int value;
    virtual ~Item()=default;
};

struct Weapon : Item{//武器
    Weapon(){}
    Weapon(const std::string& n, int v, int t, int n_a, int m_a, float l_s, int d, int h, float a, float ve, bool c_e, const std::string i_p)
    :Item{n, v, {Settings::ITEM_WEAPON, t}}, now_ammo(n_a), max_ammo(m_a), load_speed(l_s), damage(d), health(h), attacke(a), vel(ve), can_explo(c_e), image_path(i_p){}
    int now_ammo, max_ammo, damage, health;
    float load=100, load_speed, attacke, vel;
    bool can_explo;
    std::string image_path;
};

struct Ammo : Item{//弹药
    Ammo(){}
    Ammo(int v, int t, int c)
    :Item{Settings::AMMO_NAME[t]+"弹药", v, {Settings::ITEM_AMMO, t}}, count(c){}
    int count;
};

struct Exit{//出口
    Exit(){}
    Exit(const std::string g_n, float x, float y, float g_x, float g_y)
    :go_name(g_n), x(x), y(y), go_x(g_x), go_y(g_y){}
    VisibilityStamp vis;
    std::string go_name;
    float x, y, go_x, go_y;
};

struct Light{//光
    ColorRGB color;
    float x, y, r, intensity, dir_angle, cone_angle;
    int display;
};

struct Supply{//补给
    Supply(){}
    Supply(float x, float y, bool o, bool l, const std::string& n)
    :x(x), y(y), open(o), lasting(l), name(n){}
    std::string name;
    std::vector<std::unique_ptr<Item>> items;
    VisibilityStamp vis;
    uint64_t id;
    float x, y;
    bool open, is_opening=false, lasting;
};

struct Bullet{//子弹
    Bullet(){}
    Bullet(float x, float y, float v_x, float v_y, int d, int h, int i, bool c_e)
    :x(x), y(y), vel_x(v_x), vel_y(v_y), damage(d), health(h), id(i), can_explo(c_e){}
    VisibilityStamp vis;
    float x, y, vel_x, vel_y, health;
    int damage, id;
    bool can_explo;
};

struct Cell{//记录单个格子的情况
    int8_t info=0, grass_type=0;
};

struct Sfx{//音效
    std::string name;
    int channel;
    float x, y, base_vol, max_dist;
};

struct PhysicsParams{//物理量
    float mass, x, y, vel_x, vel_y, accel_x, accel_y, boun, drag;
};

struct SkyKey{
    float hour;
    float intensity;
    ColorRGB color;
};

struct BloodStain{
    ColorRGB color;
    uint8_t alpha;
    float x, y;
    int display;
};

#endif

