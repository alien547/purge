#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <utility>
#include "Tool/Math.h"

//游戏 全局数据 
namespace Settings{
    constexpr short
    //窗口
    SCREEN_WIDTH    = 960,
    SCREEN_HEIGHT   = 540,
    FONT_SIZE       = 16,
    SMALL_FONT_SIZE = 12,
    //武器
    MELEE   = -1,
    PISTOL  = 0,
    SMG     = 1,
    SHOTGUN = 2,
    RPG     = 3,
    //消耗品
    CONS_MEDKIT = 0,
    CONS_PSYCH  = 1,
    //信息
    INFO_SYSTEM  = 0,
    INFO_CHAT    = 1,
    INFO_ACHIEVE = 2,
    //背包
    NOW_USE_SIZE = 4,
    BAG_SIZE     = 8,
    //背包操作
    INV_SWAP = 0,
    INV_GET  = 1,
    INV_USE  = 2,
    INV_AWAY = 3,
    //敌人
    NORMAL_ZOMBIE    = 0,
    FAST_ZOMBIE      = 1,
    TANK_ZOMBIE      = 2,
    INVISIBLE_ZOMBIE = 3,
    //物品
    ITEM_EMPTY  = -1,
    ITEM_CONS   = 0,
    ITEM_WEAPON = 1,
    ITEM_AMMO   = 2,
    //格子
    CELL_WALL     = 0,
    CELL_WATER    = 1,
    //游戏
    AMMO_TYPE_SIZE    = 4,
    MAX_ATTACKER_SIZE = 4,
    //声音
    TOTAL_CHANNELS    = 32,
    RESERVED_CHANNELS = 2,
    CHANNEL_RAIN      = 0,
    SOUND_RAIN        = 0,
    //休眠
    SHORT_TIME = 35,
    MID_TIME   = 250,
    LONG_TIME  = 1200;

    constexpr uint8_t
    IR_WATER       = 20,
    IR_WALL        = 90,
    IR_DEAD_ZOMBIE = 60,
    IR_BLOOD       = 80,
    IR_SUPPLY      = 180,
    IR_HUMAN       = 200,
    IR_EXIT        = 220,
    IR_GRASS_DRY   = 200,
    IR_GRASS       = 220,
    IR_GRASS_DEEP  = 230,
    IR_RAIN        = 25,
    IR_ZOMBIE      = 210,
    IR_BULLET      = 240;

    constexpr short
    VERSION[3]     = {1, 1, 2},
    LAST_UPDATE[3] = {2026, 10, 6};

    constexpr float
    //视野
    HUMAN_SEE_LEN   = 7.0f,
    HUMAN_SEE_ANGLE = 60*PI/180.0f,//60°
    //手电筒
    FLASH_LEN   = 6.0f,
    FLASH_ANGLE = 55*PI/180.0f,//55°
    //速度
    HUMAN_ACCEL      = 4.5f,
    HUMAN_TURN_SPEED = 0.5f;

    extern std::string user_name;
    extern const std::pair<int, int> SOUND_INFO, SOUND_ASK, SOUND_OK;
    extern const std::string AMMO_NAME[AMMO_TYPE_SIZE];
    extern const std::string WEBSITE, GAME_LINK, API, DOWNLOAD_LINK, DATA_PATH, TEMP_PATH, MAPS_PATH, WEAPONS_PATH, GAME_NAME, QQ_ID;
    
}

#endif

