#pragma once

#include <chrono>
#include <unordered_map>
#include "oqpi.hpp"

int64_t query_performance_counter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

int64_t query_performance_frequency()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}

double duration(int64_t s, int64_t e)
{
    static const auto F = query_performance_frequency();
    auto dt = e - s;
    return (dt / (F*1.0)) * 1000.0;
}


class timing_registry
{
private:
    struct task_info
    {
        std::string name;
        int64_t createdAt = 0;
        int64_t startedAt_ = 0;
        int64_t stoppedAt_ = 0;
        double duration_ = 0;
        int32_t startedOnCore_ = -1;
        int32_t stoppedOnCore_ = -1;
        std::vector<uint64_t> children;
        bool isGroup = false;
        oqpi::task_uid parentUID = oqpi::invalid_task_uid;

        void print() const
        {
            std::string msg = name;
            msg += " took ";
            msg += std::to_string(duration_);
            msg += "ms, it started on core ";
            msg += std::to_string(startedOnCore_);
            msg += ", and stopped on core ";
            msg += std::to_string(stoppedOnCore_);
            msg += "\n";
            std::cout << msg;
        }
    };

public:
    static timing_registry& get()
    {
        static timing_registry instance;
        return instance;
    }

    void registerTask(uint64_t uid, const std::string &name, bool isGroup = false)
    {
        auto t = query_performance_counter();
        std::lock_guard<std::mutex> __l(m);
        auto &h = tasks[uid];
        h.name = name;
        h.createdAt = t;
        h.isGroup = isGroup;
    }

    void registerGroup(uint64_t uid, const std::string &name)
    {
        registerTask(uid, name, true);
    }

    void startTask(uint64_t uid)
    {
        auto t = query_performance_counter();
        std::lock_guard<std::mutex> __l(m);
        //oqpi_check(tasks.find(name) == tasks.end());
        auto &h = tasks[uid];
        h.startedOnCore_ = oqpi::this_thread::get_current_core();
        h.startedAt_ = t;
    }

    void endTask(uint64_t uid)
    {
        auto t = query_performance_counter();
        std::lock_guard<std::mutex> __l(m);
        auto it = tasks.find(uid);
        //oqpi_check(it != tasks.end());
        it->second.stoppedAt_ = t;
        it->second.duration_ = duration(it->second.startedAt_, it->second.stoppedAt_);
        it->second.stoppedOnCore_ = oqpi::this_thread::get_current_core();
    }

    void addToGroup(uint64_t guid, uint64_t tuid)
    {
        std::lock_guard<std::mutex> __l(m);
        // Add this task to the parent
        auto git = tasks.find(guid);
        git->second.children.push_back(tuid);
        // Flag the task as having a parent
        auto tit = tasks.find(tuid);
        tit->second.parentUID = guid;
    }

    void startGroup(uint64_t uid)
    {
        startTask(uid);
        tasks[uid].isGroup = true;
    }

    void endGroup(uint64_t uid)
    {
        endTask(uid);
    }

    void dump()
    {
        std::vector<task_info> infos;
        for (auto &p : tasks)
        {
            if (p.second.parentUID == oqpi::invalid_task_uid)
                infos.push_back(p.second);
        }
        print(infos, 0);
    }

    void print(const std::vector<task_info> &infos, int depth)
    {
        for (const auto &ti : infos)
        {
            auto d = depth;
            while (d--) std::cout << "    ";

            ti.print();

            if (!ti.children.empty())
            {   
                double totalDuration = 0.0;
                print(collectChildren(ti.children, totalDuration), depth + 1);

                d = depth;
                while (d--) std::cout << "    ";
                std::cout
                    << "End of "
                    << ti.name
                    << " with "
                    << ti.duration_
                    << "ms (total accumulated time of tasks = "
                    << totalDuration
                    << "ms, that is "
                    << ti.duration_ * 100.0 / totalDuration
                    << "% of group duration)"
                    << std::endl;
            }

        }
    }

    std::vector<task_info> collectChildren(const std::vector<uint64_t> &children, double &totalDuration)
    {
        std::vector<task_info> infos;
        totalDuration = 0;
        for (auto &c : children)
        {
            infos.push_back(tasks.find(c)->second);
            totalDuration += infos.back().duration_;
        }
        return std::move(infos);
    }

    void reset()
    {
        tasks.clear();
    }

private:
    std::mutex m;       
    std::unordered_map<uint64_t, task_info> tasks;
};

class timer_task_context
    : public oqpi::task_context_base
{
public:
    timer_task_context(oqpi::task_base *pOwner, const std::string &name)
        : oqpi::task_context_base(pOwner, name)
    {
        timing_registry::get().registerTask(pOwner->getUID(), name);
    }

    inline void onPreExecute()
    {
        timing_registry::get().startTask(this->owner()->getUID());
    }

    inline void onPostExecute()
    {
        timing_registry::get().endTask(this->owner()->getUID());
    }

private:
};


class timer_group_context
    : public oqpi::group_context_base
{
public:
    timer_group_context(oqpi::task_group_base *pOwner, const std::string &name)
        : oqpi::group_context_base(pOwner, name)
    {
        timing_registry::get().registerTask(pOwner->getUID(), name);
    }

    inline void onTaskAdded(const oqpi::task_handle &hTask)
    {
        timing_registry::get().addToGroup(this->owner()->getUID(), hTask.getUID());
    }

    inline void onPreExecute()
    {
        timing_registry::get().startGroup(this->owner()->getUID());
    }

    inline void onPostExecute()
    {
        timing_registry::get().endGroup(this->owner()->getUID());
    }
};


namespace this_task {

}
