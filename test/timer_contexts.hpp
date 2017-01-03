#pragma once

#include <chrono>
#include "oqpi.hpp"

class timer_task_context
    : public oqpi::task_context_base
{
public:
    timer_task_context(oqpi::task_base *pOwner)
        : oqpi::task_context_base(pOwner)
        , createdAt_(std::chrono::high_resolution_clock::now())
    {}

    inline void onPreExecute()
    {
        startedAt_ = std::chrono::high_resolution_clock::now();
    }

    inline void onPostExecute()
    {
        stoppedAt_ = std::chrono::high_resolution_clock::now();

        std::string msg = owner()->getName();
        msg += " took ";
        msg += std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stoppedAt_ - startedAt_).count());
        msg += "ms\n";
        std::cout << msg;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> createdAt_;
    std::chrono::time_point<std::chrono::high_resolution_clock> startedAt_;
    std::chrono::time_point<std::chrono::high_resolution_clock> stoppedAt_;
};


class timer_group_context
    : public oqpi::group_context_base
{
public:
    timer_group_context(oqpi::task_group_base *pOwner)
        : oqpi::group_context_base(pOwner)
        , createdAt_(std::chrono::high_resolution_clock::now())
        , taskCount_(0)
    {}

    inline void onTaskAdded(const oqpi::task_handle &)
    {
        ++taskCount_;
    }

    inline void onPreExecute()
    {
        startedAt_ = std::chrono::high_resolution_clock::now();
    }

    inline void onPostExecute()
    {
        stoppedAt_ = std::chrono::high_resolution_clock::now();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> createdAt_;
    std::chrono::time_point<std::chrono::high_resolution_clock> startedAt_;
    std::chrono::time_point<std::chrono::high_resolution_clock> stoppedAt_;
    int32_t taskCount_;
};
