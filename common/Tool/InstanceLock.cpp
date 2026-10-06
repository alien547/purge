#include "InstanceLock.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

using namespace std;

bool acquire_single_instance_lock(const string& name){
    bool lock;
    #ifdef _WIN32
    string mutexName="Global\\Purge"+name+"Mutex";
    HANDLE hMutex=CreateMutexA(NULL, FALSE, mutexName.c_str());
    lock=GetLastError()!=ERROR_ALREADY_EXISTS;
    if(!lock)CloseHandle(hMutex);
    #else
    string lock_path="/tmp/purge_"+name+".lock";
    int fd=open(lock_path.c_str(), O_CREAT|O_RDWR, 0666);
    lock=flock(fd, LOCK_EX|LOCK_NB)==0;
    if(!lock)close(fd);
    #endif
    return lock;
}

