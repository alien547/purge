#include "Render.h"
#include "Log.h"
#include "Font.h"
#include "Image.h"
#include "Random.h"
#include "Audio.h"
#include "TextInput.h"
#include "String.h"
#include "Menu.h"
#include "Settings.h"
using namespace std;
using namespace Settings;

void Menu::wait_key_press(){
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

TreeNode* Menu::add_child(TreeNode* parent, const string& child_data){
    unique_ptr<TreeNode> child(new TreeNode(child_data));
    child->parent=parent;
    TreeNode* raw=child.get();
    parent->children.push_back(move(child));
    return raw;
}

void Menu::choose(){
    int pos=0;
    vector<int> choice_text_width;
    auto on_choice=[&](int m_x, int m_y, int text_x, int base_y)->int{
        for(int i=0; i<choice_text_width.size(); ++i){
            int item_y=base_y+i*20, item_x=text_x;
            if(item_x<=m_x&&m_x<=item_x+choice_text_width[i]&&item_y<=m_y&&m_y<=item_y+FONT_SIZE){
                return i;
            }
        }
        return -1;
    };
    auto exit_node=[&]()->bool{
        if(now_node->parent==nullptr){
            return can_exit;
        }else{
            now_node=now_node->parent;
            pos=path.top();
            path.pop();
            return false;
        }
    };
    auto enter_node=[&](int k){
        beep(SOUND_OK);
        now_node=now_node->children[k].get();
        path.push(k);
        pos=0;
    };
    SDL_Event e;
    while(true){
        int mouse_x, mouse_y;
        Uint32 buttons=SDL_GetMouseState(&mouse_x, &mouse_y);
        while(SDL_PollEvent(&e)){
            switch(e.type){
                case SDL_QUIT:
                    return;
                case SDL_WINDOWEVENT:
                    update_foreground(e);
                    break;
                case SDL_KEYDOWN:
                    if(e.key.repeat==0){
                        switch(e.key.keysym.sym){
                            case SDLK_ESCAPE:
                                if(exit_node())return;
                                break;
                            case SDLK_RETURN:
                                enter_node(pos);
                                break;
                        }
                    }
                    switch(e.key.keysym.sym){
                        case SDLK_w:
                            pos=max(0, pos-1);
                            break;
                        case SDLK_s:
                            pos=min(int(now_node->children.size()-1), pos+1);
                            break;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    switch(e.button.button){
                        case SDL_BUTTON_LEFT:{
                            int m_x=e.button.x, m_y=e.button.y;
                            int hit_index=on_choice(m_x, m_y, 20, 20);
                            particle_system.emit_explo(float(m_x)/FONT_SIZE, float(m_y)/FONT_SIZE, {5, 13}, {5, 13}, {5, 13}, 30,
                            {uint8_t(250+random(-5, 5)), uint8_t(200+random(-5, 5)), uint8_t(5+random(-5, 5))});
                            play_sfx("mouse_click", 0);
                            if(hit_index!=-1)enter_node(hit_index);
                            break;
                        }
                        case SDL_BUTTON_RIGHT:
                            if(exit_node())return;
                            break;
                    }
                    break;
            }
        }

        clear_renderer();
        draw_background_texture();
        DrawTextOptions opts;
        opts.cache=true;
        draw_text(5, 0, "== "+now_node->data+" ==", opts);
        opts.color={255, 255, 0, 255};
        draw_text(5, 20*(pos+1), "*", opts);
        int on_choice_pos=on_choice(mouse_x, mouse_y, 20, 20);
        if(choice_text_width.size()!=now_node->children.size())choice_text_width.resize(now_node->children.size());
        for(int i=0; i<now_node->children.size(); ++i){
            DrawTextOptions opts;
            opts.cache=true;
            if(i==on_choice_pos){
                opts.color={200, 230, 255, 255};
                choice_text_width[i]=draw_text(20, 20*(i+1)+1, now_node->children[i]->data, opts);
            }else{
                choice_text_width[i]=draw_text(20, 20*(i+1), now_node->children[i]->data, opts);
            }
        }
        particle_system.emit_explo(float(mouse_x)/FONT_SIZE, float(mouse_y)/FONT_SIZE, {1, 2}, {8, 13}, {8, 15}, 2,
        {uint8_t(250+random(-5, 5)), uint8_t(200+random(-5, 5)), uint8_t(5+random(-5, 5))});
        particle_system.update(SCREEN_WIDTH, SCREEN_HEIGHT);
        particle_system.render(0, 0);
        draw_noise_texture(2);
        SDL_RenderPresent(renderer);

        if(now_node->children.empty()){
            if(now_node->on_enter)now_node->on_enter();
            now_node=now_node->parent;
            pos=path.top();
            path.pop();
            continue;
        }
        SDL_Delay(SHORT_TIME);
    }
}

void Menu::start_page(){
    debug("启动程序", DEBUG_INFO);
    clear_renderer();
    draw_background_texture();
    on_start_page();
}

void Menu::end_page(){
    debug("退出程序", DEBUG_INFO);
    clear_renderer();
    draw_background_texture();
    on_end_page();
}

