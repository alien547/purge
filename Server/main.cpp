/*
----------------------------------------
我的网站：https://alien547.pages.dev
QQ群：158663617
----------------------------------------
游戏中文名：清除感染者
游戏英文名：Purge
类型：服务器
作者：alien547
版本：1.0.2
编译环境：ISO C++11
Copyright (c) 2026 alien547
使用 MIT 许可证授权，详见项目根目录下的 LICENSE.txt 文件
----------------------------------------
*/
//Use GBK if Chinese looks wrong.
#define ENET_IMPLEMENTATION
#define SDL_MAIN_HANDLED

#include <chrono>
#include <csignal>
#include <unordered_map>
#include <memory>
#include <thread>
#include <cmath>
#include <enet/enet.h>
#include <curl/curl.h>
#include "Network.h"
#include "World.h"
#include "Tool/String.h"
#include "Tool/Log.h"
using namespace std;
using namespace chrono;
using namespace Settings;

volatile sig_atomic_t run=1;

struct Time{
    time_point<steady_clock> last_zombie_update=steady_clock::now(), last_zombie_add=steady_clock::now(),
    last_bullet_update=steady_clock::now(), last_supply_add=steady_clock::now(), last_env_update=steady_clock::now();
}all_time;

unordered_map<ENetPeer*, KeyState> player_inputs;
unordered_map<ENetPeer*, uint64_t> peer_to_id;
unordered_map<uint64_t, int> id_to_index;
unordered_map<ENetPeer*, uint32_t> pending_peers;

World world;

void send_server_message(ENetPeer* peer, const string& content, uint8_t type){
    ServerMessage msg;
    strncpy(msg.content, content.c_str(), MAX_CHAT_MESSAGE_SIZE-1);
    msg.content[MAX_CHAT_MESSAGE_SIZE-1]='\0';
    msg.type=type;
    ENetPacket* packet=enet_packet_create(&msg, sizeof(msg), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 1, packet);
}

void send_supply_update(ENetPeer* peer, Supply* supply){
    if(!supply)return;
    OpenSupply open_supply;
    strncpy(open_supply.name, supply->name.c_str(), MAX_SUPPLY_NAME_SIZE-1);
    open_supply.name[MAX_SUPPLY_NAME_SIZE-1]='\0';
    open_supply.size=supply->items.size();
    open_supply.id=supply->id;
    for(int i=0; i<min(int(supply->items.size()), int(MAX_SUPPLY_ITEM_SIZE)); ++i){
        open_supply.items[i]=world.item_to_net(supply->items[i].get());
    }
    ENetPacket* packet=enet_packet_create(&open_supply, sizeof(open_supply), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 1, packet);
}

void send_player_dead(ENetPeer* peer){
    PlayerDead dead;
    ENetPacket* packet=enet_packet_create(&dead, sizeof(dead), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 1, packet);
}

void send_snapshot_to_peer(ENetPeer* peer){
    auto peer_it=peer_to_id.find(peer);
    if(peer_it==peer_to_id.end())return;
    uint64_t player_id=peer_it->second;
    auto id_it=id_to_index.find(player_id);
    if(id_it==id_to_index.end())return;
    Human& p=world.humans[id_it->second];

    float k=p.run?0.94:1;
    auto is_interested=[&](float x, float y)->bool{
        return (fabs(p.physics_params.x-x)<1+EPSILON&&fabs(p.physics_params.y-y)<1+EPSILON)||
        world.is_point_visible(p.physics_params.x, p.physics_params.y, x, y, HUMAN_SEE_LEN*k, p.direction.first, p.direction.second, HUMAN_SEE_ANGLE*k);
    };

    WorldSnapshot snap;
    static int32_t seq=0;
    snap.sequence=++seq;

    strncpy(snap.map_file_name, world.file_name.c_str(), MAX_MAP_FILE_NAME_SIZE-1);
    snap.map_file_name[MAX_MAP_FILE_NAME_SIZE-1]='\0';

    Weapon* w=p.get_now_weapon(world);
    strncpy(snap.weapon_image_path, w->image_path.c_str(), MAX_WEAPON_IMAGE_PATH_SIZE-1);
    snap.weapon_image_path[MAX_WEAPON_IMAGE_PATH_SIZE-1]='\0';
    snap.flash=p.flash;
    snap.night_vision=p.night_vision;
    snap.flash_battery=p.flash_battery;
    snap.health=p.health;
    snap.max_health=p.max_health;
    snap.stamina=p.stamina;
    snap.sanity=p.sanity;
    snap.weapon_load=w->load;
    snap.weapon_now_ammo=w->now_ammo;
    snap.experience=p.experience;
    snap.money=p.money;
    snap.total_kills=p.total_kills;
    snap.upgrade_health=p.upgrade_health;
    snap.search_item=p.search_item;
    snap.go_exit=p.go_exit;
    snap.hurt=p.hurt;
    snap.weapon_switch_time=p.weapon_switch_time;
    snap.stun_time=p.stun_time;
    snap.nv_boot_time=p.nv_boot_time;
    for(int j=0; j<AMMO_TYPE_SIZE; ++j)snap.ammo[j]=p.ammo[j];
    for(int j=0; j<NOW_USE_SIZE; ++j)snap.now_use[j]=world.item_to_net(p.now_use[j].get());
    for(int j=0; j<BAG_SIZE; ++j)snap.bag[j]=world.item_to_net(p.bag[j].get());

    int now_idx;

    now_idx=0;
    for(Human& h : world.humans){
        if(now_idx>=MAX_HUMAN_SIZE)break;
        if(!is_interested(h.physics_params.x, h.physics_params.y))continue;
        snap.humans[now_idx].x=h.physics_params.x;
        snap.humans[now_idx].y=h.physics_params.y;
        snap.humans[now_idx].dir_x=h.direction.first;
        snap.humans[now_idx].dir_y=h.direction.second;
        snap.humans[now_idx].slash_angle=h.slash_angle;
        snap.humans[now_idx].weapon_type=h.weapon_type;
        snap.humans[now_idx].slash_time=h.slash_time;
        snap.humans[now_idx].id=h.id;
        ++now_idx;
    }
    snap.humans_size=now_idx;

    now_idx=0;
    for(Zombie& z : world.zombies){
        if(now_idx>=MAX_ZOMBIE_SIZE)break;
        if(!is_interested(z.physics_params.x, z.physics_params.y))continue;
        snap.zombies[now_idx].x=z.physics_params.x;
        snap.zombies[now_idx].y=z.physics_params.y;
        snap.zombies[now_idx].dead=z.dead;
        snap.zombies[now_idx].type=z.type;
        ++now_idx;
    }
    snap.zombies_size=now_idx;

    now_idx=0;
    for(Bullet& b : world.bullets){
        if(now_idx>=MAX_BULLET_SIZE)break;
        if(!is_interested(b.x, b.y))continue;
        snap.bullets[now_idx].x=b.x;
        snap.bullets[now_idx].y=b.y;
        ++now_idx;
    }
    snap.bullets_size=now_idx;

    now_idx=0;
    for(unique_ptr<Supply>& s : world.supplies){
        if(now_idx>=MAX_SUPPLY_SIZE)break;
        if(!is_interested(s->x, s->y))continue;
        snap.supplies[now_idx].x=s->x;
        snap.supplies[now_idx].y=s->y;
        snap.supplies[now_idx].open=s->open;
        ++now_idx;
    }
    snap.supplies_size=now_idx;

    now_idx=0;
    for(Light& l : world.lights){
        if(now_idx>=MAX_LIGHT_SIZE)break;
        float u=l.x-p.physics_params.x, v=l.y-p.physics_params.y, dist=HUMAN_SEE_LEN*k+l.r;
        if(u*u+v*v>dist*dist)continue;
        snap.lights[now_idx].x=l.x;
        snap.lights[now_idx].y=l.y;
        snap.lights[now_idx].r=l.r;
        snap.lights[now_idx].intensity=l.intensity;
        snap.lights[now_idx].dir_angle=l.dir_angle;
        snap.lights[now_idx].cone_angle=l.cone_angle;
        snap.lights[now_idx].color_r=l.color.r;
        snap.lights[now_idx].color_g=l.color.g;
        snap.lights[now_idx].color_b=l.color.b;
        ++now_idx;
    }
    snap.lights_size=now_idx;

    snap.day=world.day;
    snap.hour=world.hour;
    snap.minute=world.minute;
    snap.rain_time=world.rain_time;
    snap.sky_flash=world.sky_flash;
    snap.env_play_sound_info=world.play_sound_info;
    snap.rain_heavy=world.rain_heavy;
    snap.env_light_intensity=world.env_light_intensity;
    snap.env_light_color_r=world.env_light_color.r;
    snap.env_light_color_g=world.env_light_color.g;
    snap.env_light_color_b=world.env_light_color.b;

    vector<char> compressed_data=compress_data(snap);
    vector<char> packet_data;
    packet_data.reserve(1+compressed_data.size());
    packet_data.push_back(PKT_WORLD_SNAPSHOT);
    packet_data.insert(packet_data.end(), compressed_data.begin(), compressed_data.end());
    ENetPacket* packet=enet_packet_create(packet_data.data(), packet_data.size(), ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer, 0, packet);
}

void update_world(ENetHost* server){
    auto d_now_time=[](time_point<steady_clock>& time, double max_d_time)->bool{
        if(duration<double>(steady_clock::now()-time).count()>=max_d_time){
            time+=duration_cast<steady_clock::duration>(duration<double>(max_d_time));
            return true;
        }
        return false;
    };
    while(d_now_time(all_time.last_env_update, 0.1)){
        world.env_update();
        for(const pair<ENetPeer*, uint64_t>& i : peer_to_id){
            uint64_t player_id=i.second;
            auto idx_it=id_to_index.find(player_id);
            if(idx_it==id_to_index.end())continue;
            Human& player=world.humans[idx_it->second];
            auto in_it=player_inputs.find(i.first);
            if(in_it==player_inputs.end())continue;
            player.apply_input(world, in_it->second);
            if(player.flash){
                float dir_angle=atan2(player.direction.second, player.direction.first);
                world.add_light(player.physics_params.x, player.physics_params.y, FLASH_LEN, 1, dir_angle, FLASH_ANGLE, 0, {230, 235, 255});
            }
        }
        if(!world.safe){
            while(d_now_time(all_time.last_supply_add, 10.0))world.add_supply();
            while(d_now_time(all_time.last_zombie_update, 0.08))world.zombie_update();
            while(d_now_time(all_time.last_zombie_add, 15.0))world.add_zombie();
        }
    }
    while(d_now_time(all_time.last_bullet_update, 0.025))world.bullet_update();
}

void send_human_id(ENetPeer* peer, uint64_t id){
    AssignID assign;
    assign.id=id;
    ENetPacket* packet=enet_packet_create(&assign, sizeof(assign), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 1, packet);
}

void send_player_join(ENetHost* server, const string& name, const string& country, const string& province, uint64_t id){
    if(!server)return;
    PlayerJoin join;
    join.make(name, country, province, id);
    ENetPacket* packet=enet_packet_create(&join, sizeof(join), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 1, packet);
};

void signal_handler(int){
    run=0;
}

int main(int argc, char* argv[]){
    set_log_path("../"+DATA_PATH+"log.txt");
    const int MAX_TRY=100, MAX_CONNECT_SIZE=32;
    const duration<double> TICK_DURATION=duration<double>(1/30.0);
    const char spinner[5]="|/-\\";
    int spin_idx=0;
    uint64_t next_player_id=1;
    printf("Purge Server\n");
    for(int i=0; i<35; ++i)printf("-");
    printf("\n");
    int port=7777, x=0, y=0;
    string map_file_name="base";
    if(argc>=2)port=safe_stoi(argv[1], 7777);
    if(argc>=3)map_file_name=argv[2];
    if(argc>=4)x=safe_stoi(argv[3], 3);
    if(argc>=5)y=safe_stoi(argv[4], 2);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    enet_initialize();
    ENetHost* server=nullptr;
    for(int i=0; i<MAX_TRY; ++i){
        ENetAddress address={ENET_HOST_ANY, enet_uint16(port+i)};
        server=enet_host_create(&address, MAX_CONNECT_SIZE, 2, 0, 0);
        if(server){
            port+=i;
            break;
        }
    }
    if(!server){
        printf("Cannot find available port between %d and %d.\n", port, port+MAX_TRY-1);
        debug("无法在 "+to_string(port)+" ~ "+to_string(port+MAX_TRY-1)+" 之间找到可用端口", DEBUG_ERROR);
        enet_deinitialize();
        return 1;
    }
    steady_clock::time_point server_start=steady_clock::now();
    printf("Server started on port %d.\nPlease press Ctrl+C to exit.Do not close the window directly.\n", port);
    debug("服务器启动，端口 "+to_string(port), DEBUG_INFO);
    world.is_server=true;
    world.server_host=server;
    curl_global_init(CURL_GLOBAL_ALL);
    world.load_map(map_file_name, x, y);
    steady_clock::time_point last_start=steady_clock::now();
    string error_message;
    auto cleanup=[&](){
        world.save_map();
        enet_host_destroy(server);
        enet_deinitialize();
        curl_global_cleanup();
    };
    ENetEvent event;
    try{
        while(run){
            time_point<steady_clock> start=steady_clock::now();
            printf("\r");
            while(enet_host_service(server, &event, 0)>0){
                switch(event.type){
                    case ENET_EVENT_TYPE_CONNECT:
                        pending_peers[event.peer]=enet_time_get();
                        break;
                    case ENET_EVENT_TYPE_RECEIVE:{
                        uint8_t pkt_tag=*(uint8_t*)(event.packet->data);
                        switch(pkt_tag){
                            case PKT_HANDSHAKE:{
                                Handshake hand;
                                memcpy(&hand, event.packet->data, sizeof(hand));
                                auto it=pending_peers.find(event.peer);
                                if(it!=pending_peers.end()){
                                    if(PROTOCOL_VERSION==hand.protocol_version){
                                        uint64_t id=next_player_id++, index=world.humans.size();
                                        world.humans.push_back(Human());
                                        Human& human=world.humans.back();
                                        human.physics_params.x=x;
                                        human.physics_params.y=y;
                                        human.id=id;
                                        human.on_supply_open=[peer=event.peer](Supply* s){
                                            send_supply_update(peer, s);
                                        };
                                        human.on_death=[peer=event.peer](){
                                            send_player_dead(peer);
                                        };
                                        peer_to_id[event.peer]=id;
                                        id_to_index[id]=index;
                                        send_human_id(event.peer, id);
                                        printf("New player connected, assigned ID %llu.\n", (unsigned long long)(id));
                                    }else{
                                        send_server_message(it->first, "网络协议版本不同", SERVER_MESSAGE_KICK);
                                        enet_peer_disconnect_later(event.peer, 0);
                                    }
                                    pending_peers.erase(it);
                                }
                                break;
                            }
                            case PKT_CHAT_MESSAGE:{
                                ChatMessage chat;
                                memcpy(&chat, event.packet->data, sizeof(chat));
                                ENetPacket* packet=enet_packet_create(&chat, sizeof(chat), ENET_PACKET_FLAG_RELIABLE);
                                enet_host_broadcast(server, 1, packet);
                                break;
                            }
                            case PKT_PLAYER_JOIN:{
                                PlayerJoin join;
                                memcpy(&join, event.packet->data, sizeof(join));
                                auto peer_it=peer_to_id.find(event.peer);
                                if(peer_it==peer_to_id.end())break;
                                auto idx_it=id_to_index.find(peer_it->second);
                                if(idx_it==id_to_index.end())break;
                                Human& human=world.humans[idx_it->second];
                                human.name=join.name;
                                send_player_join(server, human.name, join.country, join.province, human.id);
                                break;
                            }
                            case PKT_CLOSE_SUPPLY:{
                                CloseSupply close;
                                memcpy(&close, event.packet->data, sizeof(close));
                                Supply* s=world.get_supply(close.id);
                                if(s)s->is_opening=false;
                                break;
                            }
                            case PKT_INVENTORY_ACTION:{
                                InventoryAction inv;
                                memcpy(&inv, event.packet->data, sizeof(inv));
                                auto it=peer_to_id.find(event.peer);
                                if(it==peer_to_id.end())break;
                                InventoryResult result=world.execute_inventory_action(it->second, inv);
                                if(result.changed_supply)send_supply_update(event.peer, result.supply);
                                break;
                            }
                            case PKT_KEY_STATE:{
                                KeyState input;
                                memcpy(&input, event.packet->data, sizeof(input));
                                player_inputs[event.peer]=input;
                                break;
                            }
                        }
                        enet_packet_destroy(event.packet);
                        break;
                    }
                    case ENET_EVENT_TYPE_DISCONNECT:{
                        auto pending_it=pending_peers.find(event.peer);
                        if(pending_it!=pending_peers.end())pending_peers.erase(pending_it);
                        auto peer_it=peer_to_id.find(event.peer);
                        if(peer_it==peer_to_id.end())break;
                        uint64_t id=peer_it->second;
                        auto idx_it=id_to_index.find(id);
                        if(idx_it==id_to_index.end()){
                            peer_to_id.erase(event.peer);
                            break;
                        }
                        int index=idx_it->second;
                        swap(world.humans[index], world.humans.back());
                        id_to_index[world.humans[index].id]=index;
                        PlayerLeave leave;
                        leave.id=world.humans.back().id;
                        world.humans.pop_back();
                        player_inputs.erase(event.peer);
                        peer_to_id.erase(event.peer);
                        id_to_index.erase(id);
                        ENetPacket* packet=enet_packet_create(&leave, sizeof(leave), ENET_PACKET_FLAG_RELIABLE);
                        enet_host_broadcast(server, 1, packet);
                        printf("Player disconnected, ID %llu.\n", (unsigned long long)(id));
                        break;
                    }
                }
            }

            update_world(server);
            uint32_t now=enet_time_get();
            for(auto it=pending_peers.begin(); it!=pending_peers.end(); ){//踢出未发送握手包的连接
                if(now-it->second>3000){
                    send_server_message(it->first, "网络协议版本不同", SERVER_MESSAGE_KICK);
                    enet_peer_disconnect_later(it->first, 0);
                    it=pending_peers.erase(it);
                }else{
                    ++it;
                }
            }
            for(const pair<ENetPeer*, uint64_t>& i : peer_to_id){
                send_snapshot_to_peer(i.first);
            }

            printf("%c Tick Rate:%-4dUptime:%-10d", spinner[spin_idx/10],
            int(1.0/max(duration<double>(start-last_start).count(), 0.001)), int(duration_cast<seconds>(steady_clock::now()-server_start).count()));
            fflush(stdout);

            last_start=start;
            spin_idx=(spin_idx+1)%40;
            duration<double> elapsed=duration<double>(steady_clock::now()-start);
            if(elapsed<TICK_DURATION){
                this_thread::sleep_for(TICK_DURATION-elapsed);
            }
        }
        int remaining=0;
        for(int i=0; i<server->peerCount; ++i){
            ENetPeer* peer=&server->peers[i];
            if(peer->state!=ENET_PEER_STATE_CONNECTED)continue;
            ++remaining;
            send_server_message(peer, "服务器关闭", SERVER_MESSAGE_KICK);
            enet_peer_disconnect_later(peer, 0);
        }
        uint32_t deadline=enet_time_get()+3000;
        ENetEvent ev;
        while(remaining>0&&enet_time_get()<deadline){
            if(enet_host_service(server, &ev, 30)<=0)continue;
            if(ev.type==ENET_EVENT_TYPE_RECEIVE){
                enet_packet_destroy(ev.packet);
            }else if(ev.type==ENET_EVENT_TYPE_DISCONNECT){
                --remaining;
            }
        }
    }catch(const exception& e){
        error_message=e.what();
    }catch(...){
        error_message="Unknown exception";
    }
    if(!error_message.empty()){
        debug("服务器崩溃："+error_message, DEBUG_ERROR);
        string msg="Server crashed.\n"+error_message+"\nServer is about to exit...";
        printf("\n\n%s", msg.c_str());
        this_thread::sleep_for(milliseconds(LONG_TIME));
        cleanup();
        return 1;
    }
    cleanup();
    return 0;
}

