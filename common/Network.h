#ifndef NETWORK_H
#define NETWORK_H

#include "Settings.h"

constexpr uint32_t
KEYS_SIZE       = 256,
KEY_MOUSE_LEFT  = 254,
KEY_MOUSE_RIGHT = 255,

MAX_MAP_FILE_NAME_SIZE     = 16,
MAX_ITEM_NAME_SIZE         = 16,
MAX_CHAT_MESSAGE_SIZE      = 32,
MAX_SUPPLY_NAME_SIZE       = 16,
MAX_SUPPLY_ITEM_SIZE       = 8,
MAX_WEAPON_IMAGE_PATH_SIZE = 16,
MAX_HUMAN_SIZE             = 8,
MAX_HUMAN_NAME_SIZE        = 16,
MAX_LOCATION_NAME_SIZE     = 32,
MAX_ZOMBIE_SIZE            = 16,
MAX_BULLET_SIZE            = 8,
MAX_SUPPLY_SIZE            = 8,
MAX_LIGHT_SIZE             = 16,
MAX_SFX_NAME_SIZE          = 16,
MAX_MUSIC_NAME_SIZE        = 16,

PROTOCOL_VERSION = 3;

constexpr uint8_t
SERVER_MESSAGE_INFO = 0,
SERVER_MESSAGE_WARN = 1,
SERVER_MESSAGE_KICK = 2;

constexpr uint8_t
PKT_KEY_STATE        = 0,
PKT_CHAT_MESSAGE     = 1,
PKT_INVENTORY_ACTION = 2,
PKT_ASSIGN_ID        = 3,
PKT_HANDSHAKE        = 4,
PKT_OPEN_SUPPLY      = 5,
PKT_CLOSE_SUPPLY     = 6,
PKT_SFX_EVENT        = 7,
PKT_PLAYER_JOIN      = 8,
PKT_PLAYER_LEAVE     = 9,
PKT_WORLD_SNAPSHOT   = 10,
PKT_PARTICLE_EVENT   = 11,
PKT_SERVER_MESSAGE   = 12,
PKT_BLOOD_EVENT      = 13,
PKT_PLAYER_DEAD      = 14,
PKT_MUSIC_EVENT      = 15;

#pragma pack(push, 4)
struct KeyState{
    uint8_t tag=PKT_KEY_STATE;
    uint8_t keys[32];
    float mouse_x, mouse_y;
};

struct ItemData{
    char name[MAX_ITEM_NAME_SIZE];
    int16_t value;
    int8_t type, subtype;
};

struct ChatMessage{
    uint8_t tag=PKT_CHAT_MESSAGE;
    char sender_name[MAX_HUMAN_NAME_SIZE], content[MAX_CHAT_MESSAGE_SIZE];
};

struct ServerMessage{
    uint8_t tag=PKT_SERVER_MESSAGE;
    char content[MAX_CHAT_MESSAGE_SIZE];
    uint8_t type;
};

struct InventoryAction{
    uint8_t tag=PKT_INVENTORY_ACTION;
    uint64_t supply_id;
    uint8_t action;
    int8_t src_slot, dst_slot;
};

struct AssignID{
    uint8_t tag=PKT_ASSIGN_ID;
    uint64_t id;
};

struct Handshake{
    uint8_t tag=PKT_HANDSHAKE;
    uint32_t protocol_version;
};

struct OpenSupply{
    uint8_t tag=PKT_OPEN_SUPPLY;
    ItemData items[MAX_SUPPLY_ITEM_SIZE];
    char name[MAX_SUPPLY_NAME_SIZE];
    uint64_t id;
    int8_t size;
};

struct CloseSupply{
    uint8_t tag=PKT_CLOSE_SUPPLY;
    uint64_t id;
};

struct PlayerDead{
    uint8_t tag=PKT_PLAYER_DEAD;
};

struct SfxEvent{
    uint8_t tag=PKT_SFX_EVENT;
    char name[MAX_SFX_NAME_SIZE];
    float x, y, base_vol, max_dist;
};

struct MusicEvent{
    uint8_t tag=PKT_MUSIC_EVENT;
    char name[MAX_MUSIC_NAME_SIZE];
};

struct ParticleEvent{
    uint8_t tag=PKT_PARTICLE_EVENT;
    uint8_t r, g, b;
    float x, y;
    int16_t speed_min, speed_max, max_life_min, max_life_max, size_min, size_max, count;
};

struct BloodEvent{
    uint8_t tag=PKT_BLOOD_EVENT;
    float x, y, dir_x, dir_y;
    int16_t count;
};

struct PlayerJoin{
    uint8_t tag=PKT_PLAYER_JOIN;
    char name[MAX_HUMAN_NAME_SIZE], country[MAX_LOCATION_NAME_SIZE], province[MAX_LOCATION_NAME_SIZE];
    uint64_t id;
    void make(const std::string& name, const std::string& country, const std::string& province, uint64_t id);
};

struct PlayerLeave{
    uint8_t tag=PKT_PLAYER_LEAVE;
    uint64_t id;
};

struct WorldSnapshot{
    ItemData now_use[Settings::NOW_USE_SIZE], bag[Settings::BAG_SIZE];
    char map_file_name[MAX_MAP_FILE_NAME_SIZE], weapon_image_path[MAX_WEAPON_IMAGE_PATH_SIZE];
    float flash_battery, stamina, sanity, weapon_load;
    float minute, env_light_intensity;
    int32_t health, max_health, experience, money, upgrade_health;
    int32_t total_kills;
    uint32_t sequence;
    int16_t ammo[Settings::AMMO_TYPE_SIZE];
    int16_t weapon_now_ammo;
    int16_t day, hour, rain_time;
    int8_t search_item, go_exit, hurt, weapon_switch_time, stun_time, nv_boot_time;
    int8_t humans_size, zombies_size, bullets_size, supplies_size, lights_size;
    int8_t sky_flash, env_play_sound_info;
    uint8_t env_light_color_r, env_light_color_g, env_light_color_b;
    bool flash, night_vision;
    bool rain_heavy;

    struct HumanData{
        uint64_t id;
        float x, y, dir_x, dir_y, slash_angle;
        int8_t weapon_type, slash_time;
    }humans[MAX_HUMAN_SIZE];

    struct ZombieData{
        float x, y;
        int8_t type;
        bool dead;
    }zombies[MAX_ZOMBIE_SIZE];

    struct BulletData{
        float x, y;
    }bullets[MAX_BULLET_SIZE];

    struct SupplyData{
        float x, y;
        bool open;
    }supplies[MAX_SUPPLY_SIZE];

    struct LightData{
        float x, y, r, intensity, dir_angle, cone_angle;
        uint8_t color_r, color_g, color_b;
    }lights[MAX_LIGHT_SIZE];
};
#pragma pack(pop)

inline void set_key(KeyState& k_s, int scancode, bool pressed){
    k_s.keys[scancode/8]&=~(1<<(scancode%8));
    k_s.keys[scancode/8]|=(pressed?1:0)<<(scancode%8);
}

inline bool get_key(const KeyState& k_s, int scancode){
    return (k_s.keys[scancode/8]>>(scancode%8))&1;
}

#endif

