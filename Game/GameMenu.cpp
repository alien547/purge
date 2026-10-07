#include <sstream>
#include <fstream>
#include "Tool/Tool.h"
#include "Settings.h"
#include "GameState.h"
using namespace std;
using namespace chrono;
using namespace Settings;

#ifndef WEB_BUILD
#include <future>
#include <json.hpp>
using json=nlohmann::json;
#endif

void GameState::init_menu(){
    menu.root=unique_ptr<TreeNode>(new TreeNode("主菜单"));//初始化结构

    TreeNode* _0=menu.add_child(menu.root.get(), "开始游戏");
    menu.TN_node["开始游戏"]=_0;
    menu.TN_node["开始游戏_单机模式"]=menu.add_child(_0, "单机模式");
    #ifdef SERVER_BUILD
    menu.TN_node["开始游戏_联机模式"]=menu.add_child(_0, "联机模式");
    menu.TN_node["开始游戏_聊天大厅"]=menu.add_child(_0, "聊天大厅");

    TreeNode* _1=menu.add_child(menu.root.get(), "创意工坊");
    menu.TN_node["创意工坊"]=_1;
    #endif

    TreeNode* _2=menu.add_child(menu.root.get(), "设置");
    menu.TN_node["设置"]=_2;

    TreeNode* _2_0=menu.add_child(_2, "音效");
    menu.TN_node["设置_音效"]=_2_0;
    menu.TN_node["设置_音效_修改状态"]=menu.add_child(_2_0, "修改状态");

    TreeNode* _2_1=menu.add_child(_2, "最高帧率");
    menu.TN_node["设置_最高帧率"]=_2_1;
    menu.TN_node["设置_最高帧率_修改状态"]=menu.add_child(_2_1, "修改状态");

    TreeNode* _2_2=menu.add_child(_2, "背景图");
    menu.TN_node["设置_背景图"]=_2_2;
    menu.TN_node["设置_背景图_修改状态"]=menu.add_child(_2_2, "修改状态");

    menu.TN_node["设置_游戏信息"]=menu.add_child(_2, "游戏信息");
    menu.TN_node["设置_成就"]=menu.add_child(_2, "成就");
    menu.TN_node["设置_帮助"]=menu.add_child(_2, "帮助");
    menu.TN_node["设置_关于"]=menu.add_child(_2, "关于");
    #ifdef SERVER_BUILD
    menu.TN_node["设置_文件"]=menu.add_child(_2, "文件");
    menu.TN_node["设置_前往官网"]=menu.add_child(_2, "前往官网");
    #endif

    menu.now_node=menu.root.get();

    menu.TN_node["开始游戏_单机模式"]->on_enter=[this](){
        run=true;
        world.humans.clear();
        world.humans.push_back(move(start_human));
        self=0;
        world.load_map(start_map_file_name, start_map_x, start_map_y);
        Human* p=get_now_player();
        p->on_supply_open=[&](Supply* s){
            open_inventory(s);
        };
        p->on_death=[&](){
            if(is_inventory_open)close_inventory();
        };
        clear_renderer();
        SDL_RenderPresent(renderer);
        debug("开始游戏", DEBUG_INFO);
        start_game();
        start_human=move(world.humans.back());
        save_data(&start_human);
        debug("退出游戏", DEBUG_INFO);
    };
    #ifndef WEB_BUILD
    menu.TN_node["开始游戏_联机模式"]->on_enter=[this](){
        draw_text(5, 20, "1.创建房间 2.加入房间");
        int choice=prompt_choice(2);
        if(choice==1){
            string server_bin;
            #ifdef _WIN32
            server_bin="Server.exe";
            #else
            server_bin="Server";
            #endif
            start_server_process(server_bin, "7777 "+start_map_file_name+" "+to_string(start_map_x)+" "+to_string(start_map_y));
        }else if(choice==2){
            draw_text(5, 40, "输入服务器 IP：");
            SDL_RenderPresent(renderer);
            string ip;
            ip=read_input(120, 40, 20);
            if(ip.empty())return;
            draw_text(5, 60, "输入端口号：");
            SDL_RenderPresent(renderer);
            string port;
            port=read_input(100, 60, 10);
            if(port.empty())return;
            draw_text(5, 80, "连接中...");
            SDL_RenderPresent(renderer);
            if(!connect_to_server(ip, safe_stoi(port))){
                return;
            }
            run=true;
            world.file_name="";
            clear_renderer();
            SDL_RenderPresent(renderer);
            debug("开始联机游戏", DEBUG_INFO);
            time_point<steady_clock> last_start=steady_clock::now();
            start_game();
            disconnect_from_server();
            debug("退出联机游戏", DEBUG_INFO);
        }
    };
    menu.TN_node["开始游戏_聊天大厅"]->on_enter=[this](){
        const int REFRESH_INTERVAL=20000, MAX_LEN=60;
        int start=0, show_tip=0;
        bool done=false, chat=false, is_refreshing=false, is_sending=false, ignore_text_input=false;
        Uint32 last_refresh=SDL_GetTicks();
        string chat_history_path="../"+DATA_PATH+"chat_history.dat", chat_message, chat_history, tip;
        json root;
        future<string> refresh_future;
        future<HttpResponse> send_future;
        ifstream chat_history_file(chat_history_path, ios::binary);
        if(chat_history_file.is_open()){
            vector<char> packet(istreambuf_iterator<char>(chat_history_file), {});
            string raw;
            if(decompress_data(packet, raw)){
                istringstream mem(raw, ios::binary);
                load_string(mem, chat_history);
                if(!chat_history.empty()){
                    try{
                        root=json::parse(chat_history);
                        start=SCREEN_HEIGHT-100-int(root.size())*20;
                    }catch(const exception&){
                        root=json::array();
                    }
                }
            }
            chat_history_file.close();
        }
        auto refresh_messages=[&](){
            if(is_refreshing)return;
            is_refreshing=true;
            refresh_future=async(launch::async, []()->string{
                return http_get(API+"/messages");
            });
        };
        auto timestamp_to_string=[](long long ms)->string{
            time_t sec=ms/1000;
            struct tm* tm_info=localtime(&sec);
            char buffer[64];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
            return string(buffer);
        };
        refresh_messages();
        SDL_Event e;
        while(!done){
            const Uint8* keys=SDL_GetKeyboardState(NULL);
            if(!chat){
                int speed=keys[SDL_SCANCODE_K]?25:5;
                if(keys[SDL_SCANCODE_W])start=min(0, start+speed);
                if(keys[SDL_SCANCODE_S])start=max(SCREEN_HEIGHT-100-int(root.size())*20, start-speed);
            }
            while(SDL_PollEvent(&e)){
                switch(e.type){
                    case SDL_QUIT:
                        done=true;
                        break;
                    case SDL_WINDOWEVENT:
                        update_foreground(e);
                        break;
                    case SDL_KEYDOWN:
                        switch(e.key.keysym.sym){
                            case SDLK_ESCAPE:
                                done=true;
                                SDL_StopTextInput();
                                break;
                            case SDLK_y:
                                if(chat)break;
                                chat=true;
                                chat_message.clear();
                                ignore_text_input=true;
                                SDL_StartTextInput();
                                break;
                            case SDLK_r:
                                refresh_messages();
                                break;
                            case SDLK_v:
                                if(e.key.keysym.mod&KMOD_CTRL){
                                    char* clip=SDL_GetClipboardText();
                                    if(clip){
                                        chat_message+=clip;
                                        while(int(chat_message.size())>MAX_LEN)pop_back_utf8(chat_message);
                                        filter_special_chars(chat_message);
                                        SDL_free(clip);
                                    }
                                }
                                break;
                            case SDLK_RETURN:
                                if(chat&&!chat_message.empty()){
                                    is_sending=true;
                                    string body=json{{"user", user_name},{"text", chat_message}}.dump();
                                    send_future=async(launch::async, [body]()->HttpResponse{
                                        return http_post(API+"/messages", body, "application/json");
                                    });
                                }
                                chat=false;
                                SDL_StopTextInput();
                                break;
                            case SDLK_BACKSPACE:
                                if(chat)pop_back_utf8(chat_message);
                                break;
                        }
                        break;
                    case SDL_TEXTINPUT:
                        if(ignore_text_input){
                            ignore_text_input=false;
                            break;
                        }
                        chat_message+=e.text.text;
                        while(int(chat_message.size())>MAX_LEN)pop_back_utf8(chat_message);
                        filter_special_chars(chat_message);
                        break;
                }
            }

            if(SDL_GetTicks()-last_refresh>=REFRESH_INTERVAL)refresh_messages();
            if(is_refreshing&&refresh_future.wait_for(chrono::milliseconds(0))==future_status::ready){
                string temp=refresh_future.get();
                is_refreshing=false;
                if(temp.empty()){
                    tip="获取消息失败";
                    show_tip=30;
                }else{
                    try{
                        root=json::parse(temp);
                        ofstream chat_history_file(chat_history_path, ios::binary);
                        if(chat_history_file.is_open()){
                            ostringstream mem(ios::binary);
                            save_string(mem, temp);
                            string raw=mem.str();
                            vector<char> compressed=compress_data(raw.data(), int(raw.size()));
                            chat_history_file.write(compressed.data(), compressed.size());
                            chat_history_file.close();
                        }
                    }catch(const exception&){
                        tip="消息格式错误";
                        show_tip=30;
                    }
                }
                last_refresh=SDL_GetTicks();
            }
            if(is_sending&&send_future.wait_for(chrono::milliseconds(0))==future_status::ready){
                HttpResponse resp=send_future.get();
                is_sending=false;
                if(resp.status_code==200){
                    chat_message.clear();
                    refresh_messages();
                }else{
                    if(resp.status_code==403){
                        tip="消息中含有违禁词";
                    }else{
                        tip="发送失败，请重试";
                    }
                    chat=true;
                    SDL_StartTextInput();
                    show_tip=30;
                }
            }

            clear_renderer();
            draw_background_texture();
            draw_text(5, 5, "聊天大厅");
            for(int i=0; i<root.size(); ++i){
                auto& msg=root[i];
                int line=start+40+i*20;
                if(!(40<=line&&line<=SCREEN_HEIGHT-60))continue;
                string user=msg.value("user", "匿名"), text=msg.value("text", "");
                long long ts=msg.value("timestamp", 0LL);
                string display;
                display.reserve(50+user.size()+text.size());
                display="["+timestamp_to_string(ts)+"] "+user+"："+text;
                draw_text(20, line, display);
            }
            DrawTextOptions opts;
            opts.color={255, 255, 180, 255};
            if(chat||!chat_message.empty())draw_text(50, SCREEN_HEIGHT-45, "> "+chat_message+"_", opts);
            opts.cache=true;
            opts.small=true;
            draw_text(20, SCREEN_HEIGHT-20, "发言即代表同意遵守社区规范，内容将经过自动审核", opts);
            if(show_tip>0){
                --show_tip;
                DrawTextOptions opts;
                opts.color={Uint8(170+abs(show_tip%9-4)*20), 30, 30, 255};
                opts.cache=true;
                opts.center=true;
                draw_text(SCREEN_WIDTH/2, SCREEN_HEIGHT-20, tip, opts);
            }
            opts=DrawTextOptions();
            opts.color={255, 255, 180, 255};
            opts.cache=true;
            if(is_refreshing)draw_text(60, 25, "刷新消息...", opts);
            if(is_sending)draw_text(SCREEN_WIDTH-140, SCREEN_HEIGHT-45, "发送消息...", opts);
            if(start!=0)draw_text(20, 25, "......", opts);
            if(start!=SCREEN_HEIGHT-100-int(root.size())*20)draw_text(20, SCREEN_HEIGHT-55, "......", opts);
            SDL_RenderPresent(renderer);
            SDL_Delay(SHORT_TIME);
        }
    };
    menu.TN_node["创意工坊"]->on_enter=[this](){
        string server_bin;
        #ifdef _WIN32
        server_bin="Editor.exe";
        #else
        server_bin="Editor";
        #endif
        start_server_process(server_bin, "");
    };
    #endif
    menu.TN_node["设置_音效_修改状态"]->on_enter=[this](){
        change_state(sound_on);
    };
    menu.TN_node["设置_最高帧率_修改状态"]->on_enter=[this](){
        int fps_type[4]={20, 30, 60, 120};
        draw_text(5, 20, "修改最高帧率为：");
        for(int i=0; i<4; ++i)draw_text(140+i*40, 20, to_string(i+1)+"."+to_string(fps_type[i]));
        int choice=prompt_choice(4);
        if(choice!=-1&&max_fps!=fps_type[choice-1]){
            max_fps=fps_type[choice-1];
            draw_text(5, 60, "已修改为 "+to_string(fps_type[choice-1]));
            SDL_RenderPresent(renderer);
            SDL_Delay(LONG_TIME);
        }
    };
    menu.TN_node["设置_背景图_修改状态"]->on_enter=[this](){
        struct Color{SDL_Color top, bottom; string name;};
        Color color_type[4]={{{20, 10, 40, 255}, {50, 30, 100, 255}, "暗夜紫"}, {{80, 10, 10, 255}, {255, 80, 20, 255}, "熔岩红"},
        {{10, 20, 40, 255}, {80, 200, 220, 255}, "赛博青"}, {{45, 40, 40, 255}, {85, 80, 80, 255}, "极简灰"}};
        draw_text(5, 20, "修改背景图为：");
        for(int i=0; i<4; ++i)draw_text(120+i*80, 20, to_string(i+1)+"."+color_type[i].name);
        int choice=prompt_choice(4);
        if(choice!=-1&&!(color_equal(top_color, color_type[choice-1].top)&&color_equal(bottom_color, color_type[choice-1].bottom))){
            top_color=color_type[choice-1].top;
            bottom_color=color_type[choice-1].bottom;
            create_background_texture(top_color, bottom_color, background_horizon);
            draw_text(5, 60, "已修改为 "+color_type[choice-1].name);
            SDL_RenderPresent(renderer);
            SDL_Delay(LONG_TIME);
        }
    };
    menu.TN_node["设置_游戏信息"]->on_enter=[this](){
        Human* p=&start_human;
        string state_name[2]={"关闭", "开启"};
        draw_text(5, 20, R"(游戏版本号：)"+to_string(VERSION[0])+R"(.)"+to_string(VERSION[1])+R"(.)"+to_string(VERSION[2])+R"(
音效：)"+state_name[sound_on]+R"(
最大帧率：)"+to_string(max_fps)+R"(

经验：)"+to_string(p->experience)+R"(
杀敌数：)"+to_string(p->total_kills));
        SDL_RenderPresent(renderer);
        menu.wait_key_press();
    };
    menu.TN_node["设置_成就"]->on_enter=[this](){
        for(int i=0; i<2; ++i){
            draw_text(5, 20*(i+1), "["+achievements[i].name+"] "+achievements[i].info);
        }
        SDL_RenderPresent(renderer);
        menu.wait_key_press();
    };
    menu.TN_node["设置_帮助"]->on_enter=[this](){
        show_help();
    };
    menu.TN_node["设置_关于"]->on_enter=[this](){
        draw_text(5, 20, R"(作者：alien547
我的网站：)"+WEBSITE+R"(
QQ群：)"+QQ_ID+R"(

致谢每一个开放学习资源的人
这个游戏会持续更新的，敬请期待！
最后一次更新：)"+to_string(LAST_UPDATE[0])+R"(年)"+to_string(LAST_UPDATE[1])+R"(月)"+to_string(LAST_UPDATE[2])+R"(日)");
        SDL_RenderPresent(renderer);
        menu.wait_key_press();
    };
    #ifndef WEB_BUILD
    menu.TN_node["设置_文件"]->on_enter=[this](){
        draw_text(5, 20, "1.上传地图 2.下载地图");
        int choice=prompt_choice(2);
        if(choice==-1)return;
        draw_text(5, 40, "请输入地图文件名（不要加 .map 后缀）：");
        SDL_RenderPresent(renderer);
        string map_name=read_input(5, 60, MAX_MAP_FILE_NAME_SIZE-1);
        string full_name=map_name+".map";
        string local_path="../"+DATA_PATH+MAPS_PATH+full_name;
        if(choice==1){
            ifstream file(local_path, ios::binary);
            if(!file.is_open()){
                draw_text(5, 80, "本地文件不存在！");
                SDL_RenderPresent(renderer);
                SDL_Delay(LONG_TIME);
                return;
            }
            vector<char> buffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            file.close();
            string url=API+"/map/upload?name="+full_name;
            string data_string(buffer.data(), buffer.size());
            HttpResponse resp=http_post(url, data_string, "application/octet-stream");
            if(resp.status_code==200){
                draw_text(5, 80, "上传成功！");
            }else if(resp.status_code==409) {
                draw_text(5, 80, "上传失败：文件已存在");
            }else{
                draw_text(5, 80, "上传失败！");
            }
            SDL_RenderPresent(renderer);
            SDL_Delay(LONG_TIME);
        }else if(choice==2){
            string url=API+"/map/download?name="+full_name;
            string data=http_get(url);
            if(data.empty()){
                draw_text(5, 80, "下载失败！");
            }else{
                ofstream file(local_path, ios::binary);
                file.write(data.data(), data.size());
                file.close();
                draw_text(5, 80, "下载成功！");
            }
            SDL_RenderPresent(renderer);
            SDL_Delay(LONG_TIME);
        }
    };
    menu.TN_node["设置_前往官网"]->on_enter=[this](){
        draw_text(5, 20, "正在前往...");
        SDL_RenderPresent(renderer);
        open_url(WEBSITE+GAME_LINK);
    };
    #endif

    auto func1=[&](){
        draw_text(5, 0, R"(欢迎来到 )"+GAME_NAME+R"(！游戏愉快！
也欢迎你来到我的网站 )"+WEBSITE+R"(。
游戏有 bug 或有改进建议可联系我，QQ群：)"+QQ_ID+R"(。)");
        draw_image("developer_name", 20, 80);
        draw_text(10, 150, "请输入你的游戏昵称：");
        SDL_RenderPresent(renderer);
        user_name=read_input(170, 150, MAX_HUMAN_NAME_SIZE-1, "匿名");
        start_human.name=user_name;
        beep(SOUND_OK);
        create_background_texture(top_color, bottom_color, background_horizon);
    };
    auto func2=[&](){
        draw_text(5, 0, "感谢你的游玩！");
        SDL_RenderPresent(renderer);
        save_data(&start_human);
        cleanup();
    };
    menu.on_start_page=func1;
    menu.on_end_page=func2;
}

void GameState::change_state(bool& state){
    beep(SOUND_ASK);
    string state_name[2]={"关闭", "开启"};
    draw_text(5, 20, "当前状态："+state_name[state]+"\n按 Y "+state_name[!state]+"，按 N 退出");
    int8_t k=prompt_yes_no();
    if(k==1){
        state=!state;
        draw_text(5, 70, "修改成功！");
        SDL_RenderPresent(renderer);
        SDL_Delay(LONG_TIME);
    }
    return;
}

void GameState::show_help(){
    draw_text(5, 20, R"(菜单操作：
按 wa 在菜单中移动光标，Enter 选择当前光标指定选项，Esc 返回上一级菜单。

游戏操作：
Esc 退出并保存，wasd 移动，
鼠标左键 瞄准/攻击（数字键切换武器），Shift 奔跑，Tab 缓慢时间（单机）/查看玩家信息（联机）
e 进入出口，f 打开补给，t 手电筒，n 夜视仪，r 换弹，q 切换到上一个武器，b 打开背包，y 对话。

基础图例：
I 玩家 H 队友 D 尸体 Q 出口 ' 子弹 . 空地 + 补给 # 墙 ~ 水。
敌人图例：
E 普通 F 快速 T 重装 V 隐身。

游戏玩法：
用武器击杀感染者，枪械需要子弹，子弹可通过捡补给获得。
)");
    SDL_RenderPresent(renderer);
    menu.wait_key_press();
}

void GameState::start_game(){
    auto exit_game=[&]()->bool{
        int t=text_anomaly_level;
        text_anomaly_level=0;
        beep(SOUND_ASK);
        clear_renderer();
        draw_background_texture();
        draw_text(5, 0, "按 Y 退出，按 N 返回");
        int8_t k=prompt_yes_no();
        if(k==1){
            beep(SOUND_OK);
            run=false;
            return true;
        }
        text_anomaly_level=t;
        SDL_Delay(SHORT_TIME);
        return false;
    };
    double sum_time=1000.0/max(1, max_fps);
    time_point<steady_clock> last_start=steady_clock::now();
    string chat_message;
    SDL_Event e;
    SDL_ShowCursor(SDL_DISABLE);
    while(run){
        time_point<steady_clock> start=steady_clock::now();
        while(SDL_PollEvent(&e)){
            switch(e.type){
                case SDL_QUIT:
                    run=false;
                    SDL_StopTextInput();
                    break;
                case SDL_WINDOWEVENT:
                    update_foreground(e);
                    break;
                case SDL_KEYDOWN:
                    switch(e.key.keysym.sym){
                        case SDLK_ESCAPE:
                            if(!is_inventory_open&&exit_game())run=false;
                            break;
                        case SDLK_b:
                            open_inventory();
                            break;
                        case SDLK_y:
                            if(chat_mode)break;
                            chat_mode=true;
                            chat_message.clear();
                            SDL_StartTextInput();
                            break;
                        case SDLK_TAB:
                            tab_held=true;
                            if(!is_multiplayer)slowmo_active=true;
                            break;
                        case SDLK_RETURN:
                            if(chat_mode&&!chat_message.empty())send_chat(chat_message);
                            chat_mode=false;
                            SDL_StopTextInput();
                            break;
                        case SDLK_BACKSPACE:
                            if(chat_mode)pop_back_utf8(chat_message);
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    if(e.key.keysym.sym==SDLK_TAB){
                        tab_held=false;
                        if(!is_multiplayer)slowmo_active=false;
                    }
                    break;
                case SDL_TEXTINPUT:
                    chat_message+=e.text.text;
                    if(chat_message.size()>=MAX_CHAT_MESSAGE_SIZE)pop_back_utf8(chat_message);
                    filter_special_chars(chat_message);
                    break;
            }
        }
        if(!run)break;
        if(is_multiplayer||is_foreground){
            float instant_fps=1.0f/duration<double>(start-last_start).count();
            smooth_fps=smooth_fps*0.9f+instant_fps*0.1f;
            game_update();
            clear_renderer();
            draw_screen();
            if(chat_mode)draw_text(SCREEN_WIDTH/3, SCREEN_HEIGHT*2/3+40, "> "+chat_message);
            SDL_RenderPresent(renderer);
        }
        last_start=start;
        double elapsed=duration<double>(steady_clock::now()-start).count()*1000;
        if(elapsed<sum_time)SDL_Delay(sum_time-elapsed);
    }
    stop_sfx(-1);
    SDL_ShowCursor(SDL_ENABLE);
    text_anomaly_level=0;
    if(!kick_message.empty()){
        clear_renderer();
        draw_background_texture();
        draw_rounded_rect_texture(SCREEN_WIDTH/2-100, SCREEN_HEIGHT/2-70, 200, 40, 200);
        DrawTextOptions opts;
        opts.color={255, 100, 100, 255};
        opts.center=true;
        draw_text(SCREEN_WIDTH/2, SCREEN_HEIGHT/2-50, kick_message, opts);
        SDL_RenderPresent(renderer);
        SDL_Delay(LONG_TIME);
        kick_message.clear();
    }
}

