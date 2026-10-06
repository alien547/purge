/*
----------------------------------------
我的网站：https://alien547.pages.dev
QQ群：158663617
----------------------------------------
游戏中文名：清除感染者
游戏英文名：Purge
类型：游戏
作者：alien547
版本：1.1.3
编译环境：ISO C++11
Copyright (c) 2026 alien547
使用 MIT 许可证授权，详见项目根目录下的 LICENSE.txt 文件
----------------------------------------
*/
//Use GBK if Chinese looks wrong.
#include <SDL.h>
#include "GameState.h"
#include "Tool/Log.h"
#include "Tool/Render.h"
using namespace std;

int main(int argc, char* argv[]){
    GameState state;
    string error_message;
    try{
        if(!state.load_settings())return 1;
        state.menu.start_page();
        state.menu.choose();
        state.menu.end_page();
    }catch(const exception& e){
        error_message=e.what();
    }catch(...){
        error_message="未知异常";
    }
    if(!error_message.empty()){
        debug("游戏："+error_message, DEBUG_ERROR);
        string msg=error_message+"\n游戏即将退出...";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "游戏崩溃", msg.c_str(), window);
        state.cleanup();
        return 1;
    }
    return 0;
}

