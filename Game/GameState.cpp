#include <fstream>
#include <algorithm>
#include <cmath>
#include <curl/curl.h>
#include <enet/enet.h>
#include "Tool/Tool.h"
#include "World.h"
#include "Human.h"
#include "GameState.h"
using namespace std;
using namespace chrono;
using namespace Settings;

Human* GameState::get_now_player(){
    if(self<0||self>=world.humans.size())return nullptr;
    return &world.humans[self];
}

void GameState::save_data(Human* p){
    if(!p)return;
    world.save_map();
    string temp_path="../"+TEMP_PATH+"purge_data.tmp", data_path="../"+DATA_PATH+"purge_data.dat";
    ofstream data(temp_path.c_str(), ios::binary);
    if(data.is_open()){
        for(int i=0; i<3; ++i)save_value(data, VERSION[i]);
        save_value(data, p->max_health, p->total_kills, p->experience, p->money, p->upgrade_health, p->weapon_type,
        world.day, world.hour, world.minute, world.rain_time, sound_on, max_fps,
        top_color.r, top_color.b, top_color.g, top_color.a, bottom_color.r, bottom_color.b, bottom_color.g, bottom_color.a);
        for(int i=0; i<2; ++i)save_value(data, achievements[i].unlock);
        for(unique_ptr<Item>& i : p->now_use)world.save_item(data, i.get());
        for(unique_ptr<Item>& i : p->bag)world.save_item(data, i.get());
        for(int i=0; i<AMMO_TYPE_SIZE; ++i)save_value(data, p->ammo[i]);
        data.close();
        remove(data_path.c_str());
        rename(temp_path.c_str(), data_path.c_str());
    }
}

void GameState::init_commands(){
    cmd_handlers["kill"]=[this](const string&){
        world.zombies.clear();
        add_info("已清除所有僵尸", INFO_SYSTEM, 30);
    };
    cmd_handlers["spawn"]=[this](const string& args){
        int count=safe_stoi(args, 0);
        if(count<=0){
            add_info("数量必须大于0", INFO_SYSTEM, 30);
            return;
        }
        if(count>100000){
            add_info("数量过多", INFO_SYSTEM, 30);
            return;
        }
        for(int i=0; i<count; ++i)world.add_zombie();
        add_info("已生成 "+to_string(count)+" 只僵尸", INFO_SYSTEM, 30);
    };
    cmd_handlers["heal"]=[this](const string&){
        Human* p=get_now_player();
        p->respawn();
        add_info("已恢复", INFO_SYSTEM, 30);
    };
    cmd_handlers["ammo"]=[this](const string&){
        Human* p=get_now_player();
        for(int i=0; i<AMMO_TYPE_SIZE; ++i)p->ammo[i]+=1000;
        add_info("已补充弹药", INFO_SYSTEM, 30);
    };
    cmd_handlers["time"]=[this](const string&){
        world.hour+=2;
        add_info("已增加2小时", INFO_SYSTEM, 30);
    };
    cmd_handlers["weapon"]=[this](const string&){
        Human* p=get_now_player();
        int a=random(0, world.weapons.size()-2), pos=-1;
        for(int i=0; i<NOW_USE_SIZE; ++i){
            if(p->now_use[i]->type.first==ITEM_EMPTY){
                pos=i;
                break;
            }
        }
        if(pos!=-1){
            p->now_use[pos]=unique_ptr<Weapon>(new Weapon(world.weapons[a]));
        }else{
            for(int i=0; i<BAG_SIZE; ++i){
                if(p->bag[i]->type.first==ITEM_EMPTY){
                    pos=i;
                    break;
                }
            }
            if(pos!=-1){
                p->bag[pos]=unique_ptr<Weapon>(new Weapon(world.weapons[a]));
            }else{
                add_info("背包无多余空间", INFO_SYSTEM, 30);
                return;
            }
        }
        add_info("已获取随机武器", INFO_SYSTEM, 30);
    };
    cmd_handlers["help"]=[this](const string&){
        add_info("所有命令：", INFO_SYSTEM, 30);
        for(auto i : cmd_handlers)add_info(i.first, INFO_SYSTEM, 30);
    };
}

bool GameState::load_settings(){
    set_log_path("../"+DATA_PATH+"log.txt");
    debug("加载程序", DEBUG_INFO);

    //SDL
    init_render("Game");
    //SDL_mixer
    init_audio("../assets/sfx/", "../assets/music/");
    //SDL_ttf
    init_font("../assets/fonts/Font.otf");
    //SDL_image
    init_image("../assets/images/");

    SDL_ShowWindow(window);

    if(!acquire_single_instance_lock("game")){
        debug("游戏已在运行", DEBUG_ERROR);
        DrawTextOptions opts;
        opts.color={255, 200, 0, 255};
        draw_text(SCREEN_WIDTH/3, SCREEN_HEIGHT/4, "游戏已在运行！", opts);
        SDL_RenderPresent(renderer);
        SDL_Delay(LONG_TIME);
        cleanup();
        return false;
    }

    draw_text(5, 5, "加载存档...");
    SDL_RenderPresent(renderer);
    ifstream data("../"+DATA_PATH+"purge_data.dat", ios::binary);
    Human* p=&start_human;
    if(data.is_open()){
        short now_version[3];
        bool version_same=true;
        for(int i=0; i<3; ++i){
            load_value(data, now_version[i]);
            if(now_version[i]!=VERSION[i]){
                version_same=false;
            }
        }
        if(!version_same){
            data.close();
            clear_renderer();
            draw_text(5, 5, "当前游戏版本号与存档版本号不同，可能导致错误。\n游戏版本号："+to_string(VERSION[0])+"."+to_string(VERSION[1])+"."+
            to_string(VERSION[2])+"  存档版本号："+to_string(now_version[0])+"."+to_string(now_version[1])+"."+to_string(now_version[2]));
            draw_text(10, 25, "下载最新版？（按 Y 打开官网，按 N 重置存档）");
            int8_t k=prompt_yes_no();
            if(k==1){
                open_url(WEBSITE+GAME_LINK);
            }else if(k==0){
                remove(("../"+DATA_PATH+"purge_data.dat").c_str());
            }
            cleanup();
            return false;
        }
        load_value(data, p->max_health, p->total_kills, p->experience, p->money, p->upgrade_health, p->weapon_type,
        world.day, world.hour, world.minute, world.rain_time, sound_on, max_fps,
        top_color.r, top_color.b, top_color.g, top_color.a, bottom_color.r, bottom_color.b, bottom_color.g, bottom_color.a);
        p->health=p->max_health;
        start_human.id=1;
        for(int i=0; i<2; ++i)load_value(data, achievements[i].unlock);
        for(int i=0; i<NOW_USE_SIZE; ++i)p->now_use[i]=world.load_item(data);
        for(int i=0; i<BAG_SIZE; ++i)p->bag[i]=world.load_item(data);
        for(int i=0; i<AMMO_TYPE_SIZE; ++i)load_value(data, p->ammo[i]);
        data.close();
    }
    ifstream start_map("../"+DATA_PATH+"start_map.dat", ios::binary);
    if(start_map.is_open()){
        load_string(start_map, start_map_file_name);
        load_value(start_map, start_map_x, start_map_y);
        start_map.close();
    }
    draw_text(120, 5, "完成");
    draw_text(5, 25, "加载页面...");
    SDL_RenderPresent(renderer);
    init_menu();
    init_commands();
    create_background_texture(top_color, bottom_color, background_horizon);
    init_char_atlas();
    draw_text(120, 25, "完成");
    SDL_RenderPresent(renderer);
    return true;
}

void GameState::add_info(const string& message, int type, int display, const string& prefix){
    infos.push_back(Info{prefix, message, type, display});
    beep(SOUND_INFO);
}

void GameState::info_update(){
    for(Info& i : infos){
        --i.display;
    }
    infos.erase(remove_if(infos.begin(), infos.end(), [](const Info& i){return i.display<=0;}), infos.end());
}

void GameState::open_inventory(Supply* supply){
    if(is_inventory_open)close_inventory();
    inv_supply=supply;
    is_inventory_open=true;
    inv_selected=0;
    inv_swap_target=-1;
    SDL_ShowCursor(SDL_ENABLE);
}

void GameState::close_inventory(){
    is_inventory_open=false;
    inv_swap_target=-1;
    if(inv_supply){
        if(is_multiplayer){
            CloseSupply close_supply;
            close_supply.id=inv_supply->id;
            ENetPacket* packet=enet_packet_create(&close_supply, sizeof(close_supply), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(server_peer, 1, packet);
            delete inv_supply;
        }else{
            inv_supply->is_opening=false;
        }
        inv_supply=nullptr;
    }
    SDL_ShowCursor(SDL_DISABLE);
}

void GameState::handle_inventory_input(){
    Human* p=get_now_player();
    if(!p)return;
    int total_size=NOW_USE_SIZE+BAG_SIZE+(inv_supply?inv_supply->items.size():0);
    auto send_inventory_action=[&](uint8_t action){
        InventoryAction act;
        act.action=action;
        act.src_slot=inv_selected;
        act.dst_slot=inv_swap_target;
        act.supply_id=inv_supply?inv_supply->id:0;
        if(is_connected){
            ENetPacket* packet=enet_packet_create(&act, sizeof(act), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(server_peer, 1, packet);
        }else{
            world.execute_inventory_action(p->id, act);
        }
        inv_swap_target=-1;
        inv_waiting_for_action=false;
    };
    const Uint8* keys=SDL_GetKeyboardState(NULL);
    if(inv_selected>=total_size)inv_selected=total_size-1;
    if(keys[SDL_SCANCODE_ESCAPE]){
        if(inv_waiting_for_action){
            inv_waiting_for_action=false;
        }else{
            close_inventory();
        }
        return;
    }
    if(inv_waiting_for_action){
        for(int i=0; i<inv_options.size(); ++i){
            if(keys[SDL_SCANCODE_1+i]){
                int action=safe_map_find(inv_option_id, inv_options[i], -1);
                if(action==INV_SWAP){
                    inv_swap_target=inv_selected;
                    inv_waiting_for_action=false;
                }else if(action!=-1){
                    send_inventory_action(action);
                }
            }
        }
    }

    if(!inv_waiting_for_action){
        int& now_point=inv_swap_target==-1?inv_selected:inv_swap_target;
        if(keys[SDL_SCANCODE_W])now_point=(now_point-1+total_size)%total_size;
        if(keys[SDL_SCANCODE_S])now_point=(now_point+1)%total_size;
    }
    if(keys[SDL_SCANCODE_RETURN]){
        if(inv_swap_target!=-1){
            send_inventory_action(INV_SWAP);
            inv_swap_target=-1;
        }else if(!inv_waiting_for_action){
            unique_ptr<Item>& temp=world.get_inv_item(inv_selected, p, inv_supply);
            bool need[3]={temp->type.first==ITEM_AMMO, !inv_supply&&temp->type.first!=ITEM_EMPTY, temp->type.first==ITEM_CONS};
            inv_options.clear();
            inv_options.push_back(need[0]?"拾取":"切换");
            if(need[2])inv_options.push_back("使用");
            if(need[1])inv_options.push_back("丢弃");
            inv_waiting_for_action=true;
        }
    }
}

void GameState::check_achievements(){
    Human* p=get_now_player();
    if(!p)return;
    if(p->total_kills>=1&&!achievements[0].unlock){
        achievements[0].unlock=true;
        add_info("解锁 ["+achievements[0].name+"] 成就！", INFO_ACHIEVE, 45);
    }
    if(p->total_kills>=50&&!achievements[1].unlock){
        achievements[1].unlock=true;
        add_info("解锁 ["+achievements[1].name+"] 成就！", INFO_ACHIEVE, 45);
    }
}

KeyState GameState::get_input(){
    KeyState input;
    const Uint8* keys=SDL_GetKeyboardState(NULL);
    bool k=chat_mode||is_inventory_open;
    for(int i=0; i<min(int(KEYS_SIZE), int(SDL_NUM_SCANCODES)); ++i)set_key(input, i, k?false:keys[i]);
    int m_x, m_y;
    Uint32 mouse_state=SDL_GetMouseState(&m_x, &m_y);
    input.mouse_x=(float(m_x)/FONT_SIZE+view_start_x);
    input.mouse_y=(float(m_y)/FONT_SIZE+view_start_y);
    set_key(input, KEY_MOUSE_LEFT, k?false:mouse_state&SDL_BUTTON(SDL_BUTTON_LEFT));
    set_key(input, KEY_MOUSE_RIGHT, k?false:mouse_state&SDL_BUTTON(SDL_BUTTON_RIGHT));
    return input;
}

void GameState::game_update(){
    auto d_now_time=[&](time_point<steady_clock>& time, double max_d_time)->bool{
        if(duration<double>(steady_clock::now()-time).count()>=max_d_time){
            time+=duration_cast<steady_clock::duration>(duration<double>(max_d_time));
            return true;
        }
        return false;
    };
    Human* p=get_now_player();
    ++world.now_frame;

    steady_clock::time_point now=steady_clock::now();
    float raw_dt=duration<float>(now-last_frame).count();
    last_frame=now;
    if(world.slowmo_time>0){
        world.slowmo_time-=raw_dt;
        if(world.slowmo_time<=0)world.time_scale_target=1.0f;
    }
    bool entering=world.time_scale_target<world.time_scale;
    float speed=entering?9.0f:4.5f;
    float max_step=speed*raw_dt;
    world.time_scale+=Math::clamp(world.time_scale_target-world.time_scale, -max_step, max_step);
    float vs_target=Math::clamp((1.0f-world.time_scale)/0.75f, 0.0f, 1.0f);
    slowmo_visual+=(vs_target-slowmo_visual)*(1.0f-expf(-10.0f*raw_dt));
    if(slowmo_active){
        world.trigger_slowmo(0.2f, 0.1f);
    }
    if(is_multiplayer){
        while(d_now_time(all_time.last_network_update, 0.03)){
            if(is_connected&&is_foreground){
                send_input(get_input());
            }
            flush_net_tasks();
            process_network_events();
            world.rebuild_grid();
        }
        while(d_now_time(all_time.last_snapshot_consume, 0.03)){
            if(!snap_queue.empty()){
                apply_snapshot(snap_queue.front());
                snap_queue.pop_front();
            }
        }
        while(d_now_time(all_time.last_env_update, 0.1)){
            info_update();
            check_achievements();
            world.particle_system.update(world.width, world.height);
        }
    }else{
        if(duration<double>(steady_clock::now()-all_time.last_auto_save).count()>=120){
            all_time.last_auto_save=steady_clock::now();
            save_data(p);
            add_info("自动保存成功！", INFO_SYSTEM, 20);
        }
        while(d_now_time(all_time.last_env_update, 0.1)){
            world.env_update();
            p->apply_input(world, get_input());
            if(p->flash)world.add_light(p->physics_params.x, p->physics_params.y, FLASH_LEN, 1.0f, atan2(p->direction.second, p->direction.first), FLASH_ANGLE, 0, {230, 235, 255});
            info_update();
            check_achievements();
            if(!world.safe){
                while(d_now_time(all_time.last_supply_add, 10.0)){
                    pair<float, float> pos=world.select_weighted_position();
                    if(pos.first!=-1){
                        int t=random(2, 3);
                        while(t--){
                            world.add_supply(pos.first, pos.second);
                        }
                    }
                }
                while(d_now_time(all_time.last_zombie_update, 0.08))world.zombie_update();
                while(d_now_time(all_time.last_zombie_add, 15.0))world.add_zombie();
            }
            world.update_persistent_sounds();
            world.particle_system.update(world.width, world.height);
        }
        while(d_now_time(all_time.last_bullet_update, 0.025))world.bullet_update();
    }
    if(is_inventory_open){
        while(d_now_time(all_time.last_use_inventory, 0.1))handle_inventory_input();
    }
}

void GameState::cleanup(){
    flush_log_to_file();
    disconnect_from_server();
    cleanup_font();
    cleanup_image();
    cleanup_audio();
    cleanup_cursor();
    cleanup_render();
    curl_global_cleanup();
    enet_deinitialize();
}

