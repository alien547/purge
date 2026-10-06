#include "Process.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>
#endif

using namespace std;

void open_url(const string& url){
    #ifdef _WIN32
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    #elif __APPLE__
    system(("open \""+url+"\"").c_str());
    #else
    system(("xdg-open \""+url+"\" &").c_str());
    #endif
}

bool start_server_process(const string& server_path, const string& args){
    string cmd="\""+server_path+"\" "+args;

    #ifdef _WIN32
    return system(("start \"\" "+cmd).c_str())==0;
    #else
    return system((cmd+" &").c_str())==0;
    #endif
}

