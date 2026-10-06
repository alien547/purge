#include "TaskScheduler.h"
using namespace std;
using namespace chrono;

void TaskScheduler::schedule(int time, function<void()> func){
    steady_clock::time_point now=steady_clock::now();
    milliseconds duration=milliseconds(time);
    tasks.push_back({now+duration, func});
}

void TaskScheduler::update(){
    steady_clock::time_point now=steady_clock::now();
    vector<DelayedTask> due;
    for(int i=0; i<(int)tasks.size(); ){
        if(now>=tasks[i].trigger_time){
            due.push_back(std::move(tasks[i]));
            swap(tasks[i], tasks.back());
            tasks.pop_back();
        }else{
            ++i;
        }
    }
    for(auto& t : due){
        t.func();
    }
}

void TaskScheduler::clear(){
    tasks.clear();
}

