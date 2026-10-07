/*
----------------------------------------
我的网站：https://alien547.pages.dev
QQ群：158663617
----------------------------------------
游戏中文名：清除感染者
游戏英文名：Purge
类型：编辑器
作者：alien547
版本：1.0.2
编译环境：ISO C++11
Copyright (c) 2026 alien547
使用 MIT 许可证授权，详见项目根目录下的 LICENSE.txt 文件
----------------------------------------
*/
//Use GBK if Chinese looks wrong.
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <functional>
#include <chrono>
#include <algorithm>
#include <cmath>
#include "World.h"
#include "ParticleSystem.h"
#include "Tool/Tool.h"
#include "Settings.h"
using namespace std;
using namespace Settings;

World world;
pair<float, float> camera={0, 0};
int spawn_x=-1, spawn_y=-1;
bool map_light=true, need_save=false;

struct LightPreset{
    string name;
    Light light;
};

vector<LightPreset> light_presets={
    {"路灯", 200, 200, 200, -1, -1, 5.5f, 0.8f, 0, 3.14159f, -1},
    {"篝火", 255, 150, 50, -1, -1, 4.0f, 1.2f, 0, 3.14159f, -1}
};

struct ExitPreset{
    string name;
    Exit exit;
};

vector<ExitPreset> exit_presets;

struct EditOption{
    int key;
    string name;
    function<void(int x1, int y1, int x2, int y2, bool set)> apply;
};

vector<EditOption> edit_options={
    {1, "墙", [](int x1, int y1, int x2, int y2, bool set){
        for(int y=y1; y<=y2; ++y){
            for(int x=x1; x<=x2; ++x){
                if(set){
                    world.get_cell(x, y).info|=1<<CELL_WALL;
                }else{
                    world.get_cell(x, y).info&=~(1<<CELL_WALL);
                }
            }
        }
    }},
    {2, "水", [](int x1, int y1, int x2, int y2, bool set){
        for(int y=y1; y<=y2; ++y){
            for(int x=x1; x<=x2; ++x){
                if(set){
                    world.get_cell(x, y).info|=1<<CELL_WATER;
                }else{
                    world.get_cell(x, y).info&=~(1<<CELL_WATER);
                }
            }
        }
    }},
    {3, "出口", [](int x1, int y1, int x2, int y2, bool set){
        auto solve=[&](Exit& ex){
            for(int y=y1; y<=y2; ++y){
                for(int x=x1; x<=x2; ++x){
                    ex.x=x;
                    ex.y=y;
                    world.exits.push_back(ex);
                }
            }
        };
        if(set){
            draw_text(5, 10, "1.使用预设 2.新建但不放入预设 3.新建且放入预设");
            int choice=prompt_choice(3);
            if(choice==1){
                if(exit_presets.empty()){
                    draw_text(5, 30, "暂无预设");
                    SDL_RenderPresent(renderer);
                    SDL_Delay(MID_TIME);
                    return;
                }
                string choices;
                for(int i=0; i<exit_presets.size(); ++i){
                    choices+=to_string(i+1)+"."+exit_presets[i].name+" ";
                }
                draw_text(5, 30, choices);
                int choice=prompt_choice(exit_presets.size());
                solve(exit_presets[choice-1].exit);
            }else{
                draw_text(5, 30, "目标地图文件名（不要加 .map 后缀）：");
                string go_name=read_input(5, 50, 20);
                draw_text(5, 70, "目标 X 坐标：");
                int go_x=safe_stoi(read_input(5, 90, 3), 0);
                draw_text(5, 110, "目标 Y 坐标：");
                int go_y=safe_stoi(read_input(5, 130, 3), 0);
                Exit ex;
                ex.go_name=go_name;
                ex.go_x=go_x;
                ex.go_y=go_y;
                solve(ex);
                if(choice==3){
                    draw_text(5, 150, "预设名称：");
                    string name=read_input(5, 170, 20);
                    exit_presets.push_back({name, ex});
                }
            }
        }else{
            for(int i=0; i<world.exits.size(); ++i){
                Exit& ex=world.exits[i];
                if(x1<=ex.x&&ex.x<=x2&&y1<=ex.y&&ex.y<=y2){
                    world.exits.erase(world.exits.begin()+i--);
                }
            }
        }
    }},
    {4, "光源", [](int x1, int y1, int x2, int y2, bool set){
        auto solve=[&](Light& l){
            for(int y=y1; y<=y2; ++y){
                for(int x=x1; x<=x2; ++x){
                    l.x=x;
                    l.y=y;
                    world.lights.push_back(l);
                }
            }
        };
        if(set){
            draw_text(5, 10, "1.使用预设 2.新建但不放入预设 3.新建且放入预设");
            int choice=prompt_choice(3);
            if(choice==1){
                if(light_presets.empty()){
                    draw_text(5, 30, "暂无预设");
                    SDL_RenderPresent(renderer);
                    SDL_Delay(MID_TIME);
                    return;
                }
                string choices;
                for(int i=0; i<light_presets.size(); ++i){
                    choices+=to_string(i+1)+"."+light_presets[i].name+" ";
                }
                draw_text(5, 30, choices);
                int choice=prompt_choice(light_presets.size());
                solve(light_presets[choice-1].light);
            }else{
                draw_text(5, 30, "R (0-255)：");
                Uint8 r=safe_stoi(read_input(5, 50, 3), 0);
                draw_text(5, 70, "G (0-255)：");
                Uint8 g=safe_stoi(read_input(5, 90, 3), 0);
                draw_text(5, 110, "B (0-255)：");
                Uint8 b=safe_stoi(read_input(5, 130, 3), 0);
                draw_text(5, 150, "半径：");
                float radius=safe_stof(read_input(5, 170, 7), 1.0f);
                draw_text(5, 190, "强度：");
                float intensity=safe_stof(read_input(5, 210, 7), 1.0f);
                draw_text(5, 230, "起始角度（弧度，默认0）：");
                float dir_angle=safe_stof(read_input(5, 250, 7), 0.0f);
                draw_text(5, 270, "锥角（弧度，默认π）：");
                float cone_angle=safe_stof(read_input(5, 290, 7), 3.14159f);
                Light l;
                l.color={r, g, b};
                l.r=radius;
                l.intensity=intensity;
                l.dir_angle=dir_angle;
                l.cone_angle=cone_angle;
                l.display=-1;
                solve(l);
                if(choice==3){
                    draw_text(5, 310, "预设名称：");
                    string name=read_input(5, 330, 20);
                    light_presets.push_back({name, l});
                }
            }
        }else{
            for(int i=0; i<world.lights.size(); ++i){
                Light& l=world.lights[i];
                if(x1<=l.x&&l.x<=x2&&y1<=l.y&&l.y<=y2){
                    world.lights.erase(world.lights.begin()+i--);
                }
            }
        }
    }}
};

void wait_key_press(){
    SDL_Event e;
    while(true){
        while(SDL_PollEvent(&e)){
            switch(e.type){
                case SDL_KEYDOWN:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_QUIT:
                    return;
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(SHORT_TIME);
    }
}

void show_cells(){
    enum EditState{IDLE, WAIT_FIRST, WAIT_SECOND, BRUSH, WAIT_SPAWN_POS};
    EditState edit_state=IDLE;
    int from_x=-1, from_y=-1, to_x=-1, to_y=-1, type=edit_options[0].key, last_brush_x=-1, last_brush_y=-1;
    bool has=false;
    auto solve=[&](){
        int start_x=from_x, start_y=from_y, end_x=to_x, end_y=to_y;
        if(start_x>end_x)swap(start_x, end_x);
        if(start_y>end_y)swap(start_y, end_y);
        for(EditOption& opt : edit_options){
            if(type==opt.key){
                opt.apply(start_x, start_y, end_x, end_y, has);
                break;
            }
        }
        need_save=true;
    };
    auto get_pos=[&](int x, int y)->pair<float, float>{
        float map_x=camera.first+x/FONT_SIZE-3, map_y=camera.second+y/FONT_SIZE-2;
        map_x=Math::clamp(map_x, 0.0f, float(world.width-1)), map_y=Math::clamp(map_y, 0.0f, float(world.height-1));
        return {map_x, map_y};
    };
    auto format_float=[](float v)->string{
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", v);
        return buf;
    };
    SDL_Event e;
    while(true){
        const Uint8* keys=SDL_GetKeyboardState(NULL);
        float scale=(keys[SDL_SCANCODE_LSHIFT]||keys[SDL_SCANCODE_RSHIFT])?3.2f:0.8f;
        int dx=(keys[SDL_SCANCODE_D]||keys[SDL_SCANCODE_RIGHT])-(keys[SDL_SCANCODE_A]||keys[SDL_SCANCODE_LEFT]);
        int dy=(keys[SDL_SCANCODE_S]||keys[SDL_SCANCODE_DOWN])-(keys[SDL_SCANCODE_W]||keys[SDL_SCANCODE_UP]);
        camera={Math::clamp(camera.first+dx*scale, 0.0f, float(world.width-1)), Math::clamp(camera.second+dy*scale, 0.0f, float(world.height-1))};
        int end_x=min(camera.first+(SCREEN_WIDTH-350)/FONT_SIZE, float(world.width)), end_y=min(camera.second+(SCREEN_HEIGHT-200)/FONT_SIZE, float(world.height));

        while(SDL_PollEvent(&e)){
            switch(e.type){
                case SDL_QUIT:
                    return;
                case SDL_KEYDOWN:{
                    if(e.key.repeat!=0)break;
                    SDL_Keycode key=e.key.keysym.sym;
                    switch(key){
                        case SDLK_ESCAPE:
                            if(edit_state!=IDLE){
                                edit_state=IDLE;
                            }else{
                                return;
                            }
                            break;
                        case SDLK_b:
                            if(edit_state==IDLE){
                                edit_state=WAIT_SPAWN_POS;
                            }
                            break;
                        case SDLK_t:
                            map_light=!map_light;
                            break;
                        case SDLK_g:{
                            draw_text(5, 500, "前往坐标：");
                            int go_x=safe_stoi(read_input(80, 500, 5)), go_y=safe_stoi(read_input(140, 500, 5));
                            camera={Math::clamp(go_x, 0, world.width-1), Math::clamp(go_y, 0, world.height-1)};
                            break;
                        }
                        case SDLK_e:
                            if(edit_state==IDLE){
                                edit_state=WAIT_FIRST;
                            }
                            break;
                        case SDLK_k:
                            if(edit_state==IDLE){
                                edit_state=BRUSH;
                            }
                            break;
                        case SDLK_l:
                            has=!has;
                            break;
                        case SDLK_p:
                            world.generate_level();
                            need_save=true;
                            break;
                    }
                    Uint16 mod=e.key.keysym.mod;
                    if(mod&KMOD_CTRL){
                        for(EditOption& opt : edit_options){
                            if(key==opt.key+SDLK_0){
                                type=opt.key;
                                break;
                            }
                        }
                    }
                    break;
                }
                case SDL_MOUSEBUTTONDOWN:
                    if(e.button.button==SDL_BUTTON_LEFT){
                        pair<int, int> pos=get_pos(e.button.x, e.button.y);
                        if(edit_state==WAIT_FIRST){
                            from_x=pos.first;
                            from_y=pos.second;
                            edit_state=WAIT_SECOND;
                        }else if(edit_state==WAIT_SECOND){
                            to_x=pos.first;
                            to_y=pos.second;
                            solve();
                            edit_state=IDLE;
                        }else if(edit_state==WAIT_SPAWN_POS){
                            spawn_x=pos.first, spawn_y=pos.second;
                            ofstream start_map("../"+DATA_PATH+"start_map.dat", ios::binary);
                            save_string(start_map, world.file_name);
                            save_value(start_map, spawn_x, spawn_y);
                            edit_state=IDLE;
                        }
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if(edit_state==BRUSH&&(SDL_GetMouseState(NULL, NULL)&SDL_BUTTON_LMASK)){
                        pair<int, int> pos=get_pos(e.motion.x, e.motion.y);
                        if(!(pos.first==last_brush_x&&pos.second==last_brush_y)){
                            from_x=pos.first;
                            from_y=pos.second;
                            to_x=pos.first;
                            to_y=pos.second;
                            solve();
                            last_brush_x=pos.first;
                            last_brush_y=pos.second;
                        }
                    }else{
                        last_brush_x=-1;
                        last_brush_y=-1;
                    }
                    break;
            }
        }

        clear_renderer();
        int mouse_x, mouse_y;
        Uint32 buttons=SDL_GetMouseState(&mouse_x, &mouse_y);
        pair<float, float> pos=get_pos(mouse_x, mouse_y);
        if(camera.first<=pos.first&&pos.first<end_x&&camera.second<=pos.second&&pos.second<end_y){
            SDL_Rect rect={lround((pos.first-camera.first+3)*FONT_SIZE), lround((pos.second-camera.second+2)*FONT_SIZE), FONT_SIZE, FONT_SIZE};
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
            SDL_RenderFillRect(renderer, &rect);
        }
        world.draw_map(camera.first, end_x, camera.second, end_y, camera.first-3, camera.second-2, map_light);
        if(spawn_x!=-1&&spawn_y!=-1){
            SDL_SetRenderDrawColor(renderer, 220, 235, 255, 100);
            draw_circle_outline(lround((spawn_x-camera.first+3)*FONT_SIZE), lround((spawn_y-camera.second+2)*FONT_SIZE), 5);
        }
        string tip;
        switch(edit_state){
            case WAIT_FIRST:
                tip="点击起始坐标";
                break;
            case WAIT_SECOND:
                tip="点击终止坐标";
                break;
            case BRUSH:
                tip="笔刷-按住左键拖动";
                break;
            case WAIT_SPAWN_POS:
                tip="点击出生点";
                break;
        }
        draw_text(5, 500, tip);
        if(edit_state==WAIT_SECOND)draw_char(lround(from_x-camera.first+3)*FONT_SIZE, lround(from_y-camera.second+2)*FONT_SIZE, 'F');
        draw_text(5, 480, "视角坐标："+format_float(camera.first)+"，"+format_float(camera.second));
        for(EditOption& opt : edit_options){
            if(type==opt.key){
                draw_text(200, 480, "类型："+opt.name+" "+(has?"增":"删"));
                break;
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(SHORT_TIME);
    }
}

void init(){
    set_log_path("../"+DATA_PATH+"log.txt");

    //SDL
    init_render("Editor");
    //SDL_mixer
    init_audio("../assets/sfx/", "../assets/music/");
    //SDL_ttf
    init_font("../assets/fonts/Font.otf");
    //SDL_image
    init_image("../assets/images/");

    SDL_ShowWindow(window);

    init_char_atlas();
    create_background_texture();
}

void cleanup(){
    flush_log_to_file();
    cleanup_font();
    cleanup_image();
    cleanup_audio();
    cleanup_render();
}

int main(int argc, char* argv[]){
    Menu menu;
    init();
    string edit_map_history_path="../"+DATA_PATH+"edit_map_history.dat";
    vector<string> edit_map_history_name;
    ifstream edit_map_history_read(edit_map_history_path, ios::binary);
    if(edit_map_history_read.is_open()){
        int8_t size;
        load_value(edit_map_history_read, size);
        edit_map_history_name.resize(size);
        for(int i=0; i<size; ++i){
            load_string(edit_map_history_read, edit_map_history_name[i]);
        }
        edit_map_history_read.close();
    }

    menu.root=unique_ptr<TreeNode>(new TreeNode("主菜单"));

    menu.TN_node["修改地图属性"]=menu.add_child(menu.root.get(), "修改地图属性");
    menu.TN_node["地图详情"]=menu.add_child(menu.root.get(), "地图详情");
    menu.TN_node["保存"]=menu.add_child(menu.root.get(), "保存");
    menu.TN_node["帮助"]=menu.add_child(menu.root.get(), "帮助");

    menu.now_node=menu.root.get();

    menu.TN_node["修改地图属性"]->on_enter=[&](){
        Form form("地图属性");
        form.add("名称", FORM_TEXT, world.name).add("宽度", FORM_NUM, to_string(world.width), 1, 10000).add("高度", FORM_NUM, to_string(world.height), 1, 10000)
        .add("安全性", FORM_SELECT, "", 0, 0, {"不安全", "安全"}, world.safe?1:0).add("生成草地", FORM_CHECK, "1");
        form.on_submit=[&](Form& f){
            world.name=f.get("名称");
            world.width=f.int_get("宽度");
            world.height=f.int_get("高度");
            world.safe=f.idx("安全性")==1;
            world.resize(world.width, world.height);
            if(f.check_get("生成草地"))world.generate_grass();
        };
        form.run(80, 60);
    };
    menu.TN_node["地图详情"]->on_enter=[&](){
        show_cells();
    };
    menu.TN_node["保存"]->on_enter=[&](){
        world.save_map();
        need_save=false;
        clear_renderer();
        draw_background_texture();
        draw_text(5, 0, "保存成功！");
        beep(SOUND_OK);
        SDL_RenderPresent(renderer);
        SDL_Delay(MID_TIME);
    };
    menu.TN_node["帮助"]->on_enter=[&](){
        clear_renderer();
        draw_background_texture();
        draw_text(5, 0, R"(t 切换光照，g 前往指定坐标，e 修改范围格子，k 笔刷，l 切换增删，ctrl+数字键 切换物品，b 选择出生点，p 随机生成地形。)");
        SDL_RenderPresent(renderer);
        SDL_Delay(MID_TIME);
        wait_key_press();
    };

    int state=-1;
    auto func1=[&](){
        draw_text(5, 0, "欢迎来到 "+GAME_NAME+" 编辑器！\n地图文件名（不要加 .map 后缀）：");
        draw_text(5, 45, "最近修改：");
        int now_x=85;
        for(int i=0; i<edit_map_history_name.size(); ++i){
            now_x+=draw_text(now_x, 45, to_string(i+1)+"."+edit_map_history_name[i]+" ");
        }
        SDL_RenderPresent(renderer);
        auto func=[&](SDL_KeyboardEvent& k, string& out)->bool{
            if(k.keysym.mod&KMOD_CTRL){
                int idx=k.keysym.sym-SDLK_1;
                if(idx<edit_map_history_name.size()){
                    out=edit_map_history_name[idx];
                    return true;
                }
            }
            return false;
        };
        world.file_name=read_input(260, 25, MAX_MAP_FILE_NAME_SIZE, "", func);
        if(world.file_name.empty()){
            cleanup();
            state=0;
        }
        clear_renderer();
        if(!world.load_map(world.file_name, 0, 0)){
            draw_background_texture();
            draw_text(5, 0, "无法打开 "+world.file_name+"\n新建文件？\n按 Y 新建，按 N 退出");
            beep(SOUND_ASK);
            if(prompt_yes_no()!=1){
                cleanup();
                state=1;
            }
        }
        string file_name;
        ifstream start_map("../"+DATA_PATH+"start_map.dat", ios::binary);
        if(start_map.is_open()){
            load_string(start_map, file_name);
            if(file_name==world.file_name)load_value(start_map, spawn_x, spawn_y);
            start_map.close();
        }
        beep(SOUND_OK);
    };
    auto func2=[&](){
        if(need_save){
            draw_text(5, 0, "可能还有数据尚未保存，是否保存？\n按 Y 保存，按 N 退出");
            beep(SOUND_ASK);
            if(prompt_yes_no()==1){
                world.save_map();
                clear_renderer();
                draw_background_texture();
                draw_text(5, 0, "保存成功！");
                beep(SOUND_OK);
                SDL_RenderPresent(renderer);
                SDL_Delay(MID_TIME);
            }
        }
        auto it=find(edit_map_history_name.begin(), edit_map_history_name.end(), world.file_name);
        if(it!=edit_map_history_name.end())edit_map_history_name.erase(it);
        edit_map_history_name.insert(edit_map_history_name.begin(), world.file_name);
        if(edit_map_history_name.size()>5)edit_map_history_name.resize(5);
        ofstream edit_map_history_write(edit_map_history_path, ios::binary);
        save_value(edit_map_history_write, int8_t(edit_map_history_name.size()));
        for(string& i : edit_map_history_name){
            save_string(edit_map_history_write, i);
        }
        edit_map_history_write.close();
        clear_renderer();
        draw_background_texture();
        draw_text(5, 0, "即将退出程序...");
        SDL_RenderPresent(renderer);
        cleanup();
    };
    menu.on_start_page=func1;
    menu.on_end_page=func2;

    menu.start_page();
    if(state!=-1)return state;
    menu.choose();
    menu.end_page();
    return 0;
}

