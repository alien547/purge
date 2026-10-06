#include <vector>
#include <mutex>
#include <fstream>
#include <ctime>
#include "Log.h"
using namespace std;

constexpr bool DEBUG_MODE = false;

static vector<string> log_buffer;
static mutex log_mutex;

string log_path;

void set_log_path(const string& path){
    log_path=path;
}

void flush_log_to_file(){
    if(log_buffer.empty())return;
    ofstream log(log_path, ios::app);
    for(string& s : log_buffer)log << s << '\n';
    log_buffer.clear();
    log.close();
}

void debug(const string& message, int type){
    if(type==DEBUG_INFO&&!DEBUG_MODE)return;
    char time_buf[100];
    time_t t=time(nullptr);
    strftime(time_buf, sizeof(time_buf), "[%Y-%m-%d %H:%M:%S][", localtime(&t));
    string debug_text=string(time_buf);
    if(type==DEBUG_INFO){
        debug_text+="ÌáÊ¾";
    }else if(type==DEBUG_ERROR){
        debug_text+="´íÎó";
    }
    debug_text+="] "+message;
    lock_guard<mutex> lock(log_mutex);
    log_buffer.push_back(debug_text);
    if(log_buffer.size()>=10||type==DEBUG_ERROR)flush_log_to_file();
}

