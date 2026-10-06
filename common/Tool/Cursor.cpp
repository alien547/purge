#include <unordered_map>
#include "Cursor.h"
using namespace std;

struct CursorHash{
    size_t operator()(const SDL_SystemCursor& s)const{
        return std::hash<int>()(int(s));
    }
};

unordered_map<SDL_SystemCursor, SDL_Cursor*, CursorHash> cursor_cache;

void cleanup_cursor(){
    for(auto& c : cursor_cache)SDL_FreeCursor(c.second);
}

void change_cursor(SDL_SystemCursor name){
    SDL_Cursor* cur;
    auto it=cursor_cache.find(name);
    if(it==cursor_cache.end()){
        cur=SDL_CreateSystemCursor(name);
        cursor_cache[name]=cur;
    }else{
        cur=it->second;
    }
    SDL_SetCursor(cur);
}

