#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <deque>
#include <functional>
#include <unordered_map>
#include "Tool/Menu.h"
#include "Network.h"
#include "World.h"
#include "Settings.h"

#ifndef WEB_BUILD
#include "Tool/Http.h"
struct _ENetHost;
struct _ENetPeer;
typedef struct _ENetHost ENetHost;
typedef struct _ENetPeer ENetPeer;
#endif

//游戏 主逻辑，包含了游戏的 更新、渲染、保存等 逻辑 
class GameState{
    public:
        int smooth_fps=0, max_fps=60;
        uint32_t last_seq=0;
        std::string start_map_file_name;
        int start_map_x=0, start_map_y=0;
        float view_start_x=0, view_start_y=0;
        bool run=false, is_multiplayer=false, is_connected=false, chat_mode=false, tab_held=false;

        #ifndef WEB_BUILD
        ENetHost* net_client=nullptr;
        ENetPeer* server_peer=nullptr;
        std::deque<WorldSnapshot> snap_queue;
        std::once_flag enet_init_flag;
        std::unordered_map<uint64_t, std::string> player_names;
        std::unordered_map<uint64_t, IpLocation> player_locations;
        #endif

        Human start_human;
        Menu menu;

        std::string kick_message;

        bool is_inventory_open=false, inv_waiting_for_action=false;
        int inv_selected=0, inv_swap_target=-1;
        Supply* inv_supply=nullptr;
        std::vector<std::string> inv_options;
        std::unordered_map<std::string, int> inv_option_id={{"切换", Settings::INV_SWAP}, {"拾取", Settings::INV_GET}, {"使用", Settings::INV_USE}, {"丢弃", Settings::INV_AWAY}};

        SDL_Color top_color={45, 40, 40, 255}, bottom_color={85, 80, 80, 255};
        float background_horizon=0.55f;

        std::queue<std::function<void()>> net_tasks;
        std::mutex net_tasks_mutex;

        float slowmo_visual=0.0f;
        bool slowmo_active=false;

        std::chrono::steady_clock::time_point last_frame=std::chrono::steady_clock::now();

        std::unordered_map<std::string, std::function<void(const std::string&)>> cmd_handlers;

        World world;
        int self=0;
        uint64_t id=0;

        struct Time{
            std::chrono::time_point<std::chrono::steady_clock> last_zombie_update=std::chrono::steady_clock::now(), last_zombie_add=std::chrono::steady_clock::now(),
            last_bullet_update=std::chrono::steady_clock::now(), last_supply_add=std::chrono::steady_clock::now(), last_env_update=std::chrono::steady_clock::now(),
            last_use_inventory=std::chrono::steady_clock::now(), last_network_update=std::chrono::steady_clock::now(), last_auto_save=std::chrono::steady_clock::now(),
            last_snapshot_consume=std::chrono::steady_clock::now();
        }all_time;

        struct Info{
            std::string prefix, message;
            int type, display;
        };

        struct Achievement{
            std::string name, info;
            bool unlock;
        };

        std::vector<Info> infos;
        Achievement achievements[2]={{"第一滴血", "杀死 1 个敌人", false}, {"清道夫", "杀死 50 个敌人", false}};

        Human* get_now_player();
        void save_data(Human* p);
        void init_menu();
        void init_commands();
        bool load_settings();
        void change_state(bool& state);
        void show_help();
        void start_game();
        void add_info(const std::string& message, int type, int display, const std::string& prefix="");
        void info_update();
        void draw_info(int x, int y, const Info& info);
        void draw_crosshair(Human* p);
        void draw_scoreboard();
        void draw_screen();
        void open_inventory(Supply* supply=nullptr);
        void close_inventory();
        void draw_inventory();
        void handle_inventory_input();
        void check_achievements();
        KeyState get_input();
        void game_update();
        void send_chat(std::string chat_message);
        void cleanup();

        #ifndef WEB_BUILD
        void flush_net_tasks();
        bool connect_to_server(const std::string& ip, int port);
        void disconnect_from_server();
        void send_input(const KeyState& input);
        void process_network_events();
        void apply_snapshot(const WorldSnapshot& snap);
        #endif
};

#endif

