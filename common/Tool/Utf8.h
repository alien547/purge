#ifndef UTF8_H
#define UTF8_H

#include <string>
#include "Math.h"

inline int utf8_char_len(unsigned char c){
    if((c&0x80)==0)return 1;
    if((c&0xE0)==0xC0)return 2;
    if((c&0xF0)==0xE0)return 3;
    if((c&0xF8)==0xF0)return 4;
    return 1;
}

inline void filter_special_chars(std::string& str){
    int i=0;
    while(i<str.size()){
        unsigned char c=str[i];
        if(c<0x80){
            if(c<32||c==127){
                str.erase(i, 1);
                continue;
            }
            ++i;
        }else{
            i+=utf8_char_len(c);
        }
    }
}

inline void pop_back_utf8(std::string& str){
    if(str.empty())return;
    int i=str.size()-1;
    unsigned char c=str[i];
    while(i>0&&(c&0xC0)==0x80){
        c=str[--i];
    }
    str.resize(Math::max(int(str.size())-utf8_char_len(c), 0));
}

#endif

