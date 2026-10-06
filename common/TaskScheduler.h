#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <vector>
#include <chrono>
#include <functional>

class TaskScheduler{
    public:
        struct DelayedTask{
            std::chrono::steady_clock::time_point trigger_time;
            std::function<void()> func;
        };
        std::vector<DelayedTask> tasks;

        void schedule(int time, std::function<void()> func);
        void update();
        void clear();
};

#endif

