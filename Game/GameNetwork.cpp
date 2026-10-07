#include <mutex>
#include "Settings.h"
#include "GameState.h"
#include "Tool/Font.h"
#include "Tool/Render.h"
#include "Tool/Serialize.h"
#include "Tool/Utils.h"

#ifndef WEB_BUILD
#include <thread>
#include <enet/enet.h>
#include "Tool/Http.h"
#endif

using namespace std;
using namespace Settings;

void GameState::send_chat(string chat_message){
    if(!is_multiplayer){//命令
        if(chat_message[0]=='/'){
            chat_message.erase(0, 1);
            int space=chat_message.find(' ');
            string cmd_name=chat_message.substr(0, space);
            string args=(space!=string::npos)?chat_message.substr(space+1):"";

            auto handler=safe_map_find(cmd_handlers, cmd_name);
            if(handler){
                handler(args);
            }else{
                add_info("未知命令", INFO_SYSTEM, 30);
            }
            return;
        }else{
            add_info(chat_message, INFO_CHAT, 30, user_name+"：");
        }
        return;
    }
    #ifndef WEB_BUILD
    if(!server_peer)return;
    Human* p=get_now_player();
    if(!p)return;
    ChatMessage chat;
    strncpy(chat.sender_name, p->name.c_str(), MAX_HUMAN_NAME_SIZE-1);
    chat.sender_name[MAX_HUMAN_NAME_SIZE-1]='\0';
    strncpy(chat.content, chat_message.c_str(), MAX_CHAT_MESSAGE_SIZE-1);
    chat.content[MAX_CHAT_MESSAGE_SIZE-1]='\0';
    ENetPacket* packet=enet_packet_create(&chat, sizeof(chat), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(server_peer, 1, packet);
    #endif
}

#ifndef WEB_BUILD
void GameState::flush_net_tasks(){
    lock_guard<mutex> lock(net_tasks_mutex);
    while(!net_tasks.empty()){
        auto task=move(net_tasks.front());
        net_tasks.pop();
        task();
    }
}

bool GameState::connect_to_server(const string& ip, int port){
    call_once(enet_init_flag, enet_initialize);
    world.humans.clear();
    self=-1;
    id=-1;
    net_client=enet_host_create(NULL, 1, 2, 0, 0);
    ENetAddress addr;
    if(enet_address_set_host(&addr, ip.c_str())<0){
        draw_text(5, 100, "无效的服务器 IP");
        SDL_RenderPresent(renderer);
		enet_host_destroy(net_client);
		net_client=nullptr;
		SDL_Delay(LONG_TIME);
		return false;
	}
    addr.port=port;
    server_peer=enet_host_connect(net_client, &addr, 2, 0);
    if(!server_peer){
        draw_text(5, 100, "连接服务器失败");
        SDL_RenderPresent(renderer);
		enet_host_destroy(net_client);
		net_client=nullptr;
		SDL_Delay(LONG_TIME);
		return false;
	}
	is_multiplayer=true;
    is_connected=false;
    return true;
}

void GameState::disconnect_from_server(){
    if(server_peer){
		enet_peer_disconnect(server_peer, 0);
		ENetEvent event;
		while(enet_host_service(net_client, &event, 3000)>0){
			if(event.type==ENET_EVENT_TYPE_DISCONNECT)break;
		}
		server_peer=nullptr;
	}
	if(net_client){
		enet_host_destroy(net_client);
		net_client=nullptr;
	}
	snap_queue.clear();
	is_multiplayer=false;
	is_connected=false;
}

void GameState::send_input(const KeyState& input){
    if(!server_peer||!is_connected)return;
    ENetPacket* packet=enet_packet_create(&input, sizeof(input), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(server_peer, 1, packet);
}

void GameState::process_network_events(){
    if(!net_client)return;
    ENetEvent event;
    while(enet_host_service(net_client, &event, 0)>0){
        switch(event.type){
            case ENET_EVENT_TYPE_CONNECT:{
                add_info("已进入房间", INFO_SYSTEM, 30);
                last_seq=0;
                is_connected=true;
                Handshake hand;
                hand.protocol_version=PROTOCOL_VERSION;
                ENetPacket* packet=enet_packet_create(&hand, sizeof(hand), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(server_peer, 1, packet);
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:{
                uint8_t pkt_tag=*(uint8_t*)(event.packet->data);
                switch(pkt_tag){
                    case PKT_ASSIGN_ID:{
                        AssignID assign_id;
                        memcpy(&assign_id, event.packet->data, sizeof(assign_id));
                        id=assign_id.id;
                        thread([this, player_name=user_name, player_id=id](){
                            IpLocation loc=query_ip_location();
                            PlayerJoin join;
                            join.make(player_name, loc.country, loc.province, player_id);
                            {
                                lock_guard<std::mutex> lock(net_tasks_mutex);
                                net_tasks.push([this, join](){
                                    if(server_peer){
                                        ENetPacket* packet=enet_packet_create(&join, sizeof(join), ENET_PACKET_FLAG_RELIABLE);
                                        enet_peer_send(server_peer, 1, packet);
                                    }
                                });
                            }
                        }).detach();
                        break;
                    }
                    case PKT_OPEN_SUPPLY:{
                        OpenSupply open;
                        memcpy(&open, event.packet->data, sizeof(open));
                        if(is_inventory_open&&inv_supply&&inv_supply->id==open.id){//更新
                            inv_supply->items.clear();
                            for(int i=0; i<open.size; ++i)inv_supply->items.push_back(world.net_to_item(open.items[i]));
                        }else{//新建
                            Supply* new_supply=new Supply(0, 0, true, false, open.name);
                            new_supply->id=open.id;
                            for(int i=0; i<open.size; ++i)new_supply->items.push_back(world.net_to_item(open.items[i]));
                            open_inventory(new_supply);
                        }
                        break;
                    }
                    case PKT_CHAT_MESSAGE:{
                        ChatMessage chat;
                        memcpy(&chat, event.packet->data, sizeof(chat));
                        add_info(chat.content, INFO_CHAT, 30, string(chat.sender_name)+"：");
                        break;
                    }
                    case PKT_SERVER_MESSAGE:{
                        ServerMessage msg;
                        memcpy(&msg, event.packet->data, sizeof(msg));
                        if(msg.type==SERVER_MESSAGE_INFO){
                            add_info(msg.content, INFO_SYSTEM, 30, "服务器信息：");
                        }else if(msg.type==SERVER_MESSAGE_WARN){
                            add_info(msg.content, INFO_SYSTEM, 40, "服务器警告：");
                        }else if(msg.type==SERVER_MESSAGE_KICK){
                            kick_message=msg.content;
                            run=false;
                        }
                        break;
                    }
                    case PKT_SFX_EVENT:{
                        SfxEvent ev;
                        memcpy(&ev, event.packet->data, sizeof(ev));
                        world.play_sfx_at(ev.name, ev.x, ev.y, ev.base_vol, ev.max_dist);
                        break;
                    }
                    case PKT_MUSIC_EVENT:{
                        MusicEvent ev;
                        memcpy(&ev, event.packet->data, sizeof(ev));
                        if(ev.id!=id)break;
                        world.play_music_at(ev.name, ev.enabled);
                        break;
                    }
                    case PKT_PARTICLE_EVENT:{
                        ParticleEvent ev;
                        memcpy(&ev, event.packet->data, sizeof(ev));
                        world.emit_particle(ev.x, ev.y, {ev.speed_min, ev.speed_max}, {ev.max_life_min, ev.max_life_max}, {ev.size_min, ev.size_max}, ev.count, {ev.r, ev.g, ev.b});
                        break;
                    }
                    case PKT_BLOOD_EVENT:{
                        BloodEvent ev;
                        memcpy(&ev, event.packet->data, sizeof(ev));
                        world.emit_blood(ev.x, ev.y, ev.dir_x, ev.dir_y, ev.count);
                        break;
                    }
                    case PKT_PLAYER_JOIN:{
                        PlayerJoin join;
                        memcpy(&join, event.packet->data, sizeof(join));
                        player_names[join.id]=join.name;
                        IpLocation loc;
                        loc.country=join.country;
                        loc.province=join.province;
                        player_locations[join.id]=loc;
                        if(join.id!=id)add_info(" 进入房间", INFO_SYSTEM, 30, join.name);
                        break;
                    }
                    case PKT_PLAYER_LEAVE:{
                        PlayerLeave leave;
                        memcpy(&leave, event.packet->data, sizeof(leave));
                        add_info(" 离开房间", INFO_SYSTEM, 30, safe_map_find(player_names, leave.id, "未知"));
                        player_names.erase(leave.id);
                        player_locations.erase(leave.id);
                        break;
                    }
                    case PKT_PLAYER_DEAD:{
                        close_inventory();
                        break;
                    }
                    case PKT_WORLD_SNAPSHOT:{
                        vector<char> packet_data((char*)(event.packet->data)+1, (char*)(event.packet->data)+event.packet->dataLength);
                        WorldSnapshot snap;
                        if(!decompress_data(packet_data, snap))break;
                        if(snap.sequence<=last_seq)break;
                        last_seq=snap.sequence;
                        if(snap_queue.size()>=10)snap_queue.pop_front();
                        snap_queue.push_back(snap);
                        break;
                    }
                }
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                is_multiplayer=false;
                is_connected=false;
                run=false;
                break;
        }
    }
}

void GameState::apply_snapshot(const WorldSnapshot& snap){
    if(snap.map_file_name!=world.file_name){
        world.load_map(snap.map_file_name, 0, 0);
        last_seq=0;
    }

    world.day=snap.day;
    world.hour=snap.hour;
    world.minute=snap.minute;
    world.rain_time=snap.rain_time;
    world.sky_flash=snap.sky_flash;
    world.rain_heavy=snap.rain_heavy;
    world.env_light_intensity=snap.env_light_intensity;
    world.env_light_color={snap.env_light_color_r, snap.env_light_color_g, snap.env_light_color_b};

    world.zombies.clear();
    world.bullets.clear();
    world.supplies.clear();
    world.lights.clear();

    world.humans.resize(snap.humans_size);
    for(int i=0; i<snap.humans_size; ++i){
        Human& p=world.humans[i];
        const WorldSnapshot::HumanData& sp=snap.humans[i];
        p.physics_params.x=sp.x;
        p.physics_params.y=sp.y;
        p.direction={sp.dir_x, sp.dir_y};
        p.slash_angle=sp.slash_angle;
        p.weapon_type=sp.weapon_type;
        p.slash_time=sp.slash_time;
        p.id=sp.id;
        p.name=safe_map_find(player_names, p.id, "未知");
        if(p.id==id){
            self=i;
            p.flash=snap.flash;
            p.night_vision=snap.night_vision;
            p.flash_battery=snap.flash_battery;
            p.health=snap.health;
            p.max_health=snap.max_health;
            p.stamina=snap.stamina;
            p.sanity=snap.sanity;
            p.experience=snap.experience;
            p.money=snap.money;
            p.total_kills=snap.total_kills;
            p.upgrade_health=snap.upgrade_health;
            p.search_item=snap.search_item;
            p.go_exit=snap.go_exit;
            p.hurt=snap.hurt;
            p.weapon_switch_time=snap.weapon_switch_time;
            p.stun_time=snap.stun_time;
            p.nv_boot_time=snap.nv_boot_time;
            for(int j=0; j<AMMO_TYPE_SIZE; ++j)p.ammo[j]=snap.ammo[j];
            for(int j=0; j<NOW_USE_SIZE; ++j)p.now_use[j]=world.net_to_item(snap.now_use[j]);
            for(int j=0; j<BAG_SIZE; ++j)p.bag[j]=world.net_to_item(snap.bag[j]);
            Weapon* w=nullptr;
            unique_ptr<Item>& item=p.now_use[p.weapon_type];
            if(item&&item->type.first==ITEM_WEAPON){
                w=(Weapon*)(item.get());
            }
            if(w){
                w->image_path=snap.weapon_image_path;
                w->load=snap.weapon_load;
                w->now_ammo=snap.weapon_now_ammo;
            }
        }
    }

    for(int i=0; i<snap.zombies_size; ++i){
        const WorldSnapshot::ZombieData& sz=snap.zombies[i];
        Zombie z;
        z.physics_params.x=sz.x;
        z.physics_params.y=sz.y;
        z.dead=sz.dead;
        z.type=sz.type;
        world.zombies.push_back(z);
    }

    for(int i=0; i<snap.bullets_size; ++i){
        const WorldSnapshot::BulletData& sb=snap.bullets[i];
        Bullet b;
        b.x=sb.x;
        b.y=sb.y;
        world.bullets.push_back(b);
    }

    for(int i=0; i<snap.supplies_size; ++i){
        const WorldSnapshot::SupplyData& ss=snap.supplies[i];
        world.supplies.push_back(unique_ptr<Supply>(new Supply(ss.x, ss.y, ss.open, false, "")));
    }

    for(int i=0; i<snap.lights_size; ++i){
        const WorldSnapshot::LightData& li=snap.lights[i];
        Light l;
        l.x=li.x;
        l.y=li.y;
        l.r=li.r;
        l.intensity=li.intensity;
        l.dir_angle=li.dir_angle;
        l.cone_angle=li.cone_angle;
        l.color={li.color_r, li.color_g, li.color_b};
        world.lights.push_back(l);
    }

    world.update_persistent_sounds();
}
#endif

