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
        startedOnCore_ = oqpi::this_thread::get_current_core();
    }

    inline void onPostExecute()
    {
        stoppedAt_ = std::chrono::high_resolution_clock::now();
        stoppedOnCore_ = oqpi::this_thread::get_current_core();

        std::string msg = owner()->getName();
        msg += " took ";
        msg += std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(stoppedAt_ - startedAt_).count() / 1000.0);
        msg += "ms, it started on core ";
        msg += std::to_string(startedOnCore_);
        msg += ", and stopped on core ";
        msg += std::to_string(stoppedOnCore_);
        msg += "\n";
        std::cout << msg;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> createdAt_;
    std::chrono::time_point<std::chrono::high_resolution_clock> startedAt_;
    std::chrono::time_point<std::chrono::high_resolution_clock> stoppedAt_;
    int32_t startedOnCore_ = -1;
    int32_t stoppedOnCore_ = -1;
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
        startedOnCore_ = oqpi::this_thread::get_current_core();
    }

    inline void onPostExecute()
    {
        stoppedAt_ = std::chrono::high_resolution_clock::now();
        stoppedOnCore_ = oqpi::this_thread::get_current_core();


        std::string msg = "Group ";
        msg += owner()->getName();
        msg += " took ";
        msg += std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(stoppedAt_ - startedAt_).count()/1000.0);
        msg += "ms, it started on core ";
        msg += std::to_string(startedOnCore_);
        msg += ", and stopped on core ";
        msg += std::to_string(stoppedOnCore_);
        msg +="\n";
        std::cout << msg;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> createdAt_;
    std::chrono::time_point<std::chrono::high_resolution_clock> startedAt_;
    std::chrono::time_point<std::chrono::high_resolution_clock> stoppedAt_;
    int32_t taskCount_;
    int32_t startedOnCore_ = -1;
    int32_t stoppedOnCore_ = -1;
};
