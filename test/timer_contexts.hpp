#pragma once

#include <chrono>
#include <unordered_map>
#include "oqpi.hpp"


int64_t query_performance_counter_aux()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

int64_t first_measure()
{
    static const auto firstMeasure = query_performance_counter_aux();
    return firstMeasure;
}

int64_t query_performance_counter()
{
    first_measure();
    const auto t = query_performance_counter_aux();
    return t - first_measure();
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
        std::vector<oqpi::task_handle> children;
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

    void registerTask(uint64_t uid, const std::string &name)
    {
        auto t = query_performance_counter();
        std::lock_guard<std::recursive_mutex> __l(m);
        auto &h = tasks[uid];
        h.name = name;
        h.createdAt = t;
    }

    void unregisterTask(uint64_t uid)
    {
        std::lock_guard<std::recursive_mutex> __l(m);
        tasks.erase(uid);
    }

    void startTask(uint64_t uid)
    {
        auto t = query_performance_counter();
        std::lock_guard<std::recursive_mutex> __l(m);
        //oqpi_check(tasks.find(name) == tasks.end());
        auto &h = tasks[uid];
        h.startedOnCore_ = oqpi::this_thread::get_current_core();
        h.startedAt_ = t;
    }

    void endTask(uint64_t uid)
    {
        auto t = query_performance_counter();
        std::lock_guard<std::recursive_mutex> __l(m);
        auto it = tasks.find(uid);
        //oqpi_check(it != tasks.end());
        it->second.stoppedAt_ = t;
        it->second.duration_ = duration(it->second.startedAt_, it->second.stoppedAt_);
        it->second.stoppedOnCore_ = oqpi::this_thread::get_current_core();
    }

    void addToGroup(uint64_t guid, oqpi::task_handle hTask)
    {
        std::lock_guard<std::recursive_mutex> __l(m);
        // Flag the task as having a parent
        auto tit = tasks.find(hTask.getUID());
        tit->second.parentUID = guid;
        // Add this task to the parent
        auto git = tasks.find(guid);
        git->second.children.emplace_back(std::move(hTask));
        git->second.isGroup = true;
    }

    void dump()
    {
        std::vector<oqpi::task_handle> childrenToDelete;
        {
            std::lock_guard<std::recursive_mutex> __l(m);
            std::vector<task_info*> infos;
            for (auto &p : tasks)
            {
                if (p.second.parentUID == oqpi::invalid_task_uid)
                    infos.push_back(&p.second);
            }
            print(infos, 0, childrenToDelete);
        }
        childrenToDelete.clear();
    }

    void print(std::vector<task_info*> &infos, int depth, std::vector<oqpi::task_handle> &childrenToDelete)
    {
        for (auto pti : infos)
        {
            auto &ti = (*pti);
            auto d = depth;
            while (d--) std::cout << "    ";

            ti.print();

            if (!ti.children.empty())
            {   
                double totalDuration = 0.0;
                auto children = collectChildren(ti.children, totalDuration);
                print(children, depth + 1, childrenToDelete);

                d = depth;
                while (d--) std::cout << "    ";
                std::cout
                    << "End of "
                    << ti.name
                    << " with "
                    << ti.duration_
                    << "ms (total accumulated time of tasks = "
                    << totalDuration
                    << "ms, ";
                    if (totalDuration < ti.duration_)
                    {
                        std::cout
                            << "that is an overhead of "
                            << ti.duration_ - totalDuration
                            << "ms for group management)";
                    }
                    else
                    {
                        std::cout
                            << "the group represent "
                            << ti.duration_ * 100.0 / totalDuration
                            << "% of the total duration)";
                    }
                    std::cout << std::endl;
            }

            for (auto &i : ti.children)
            {
                childrenToDelete.emplace_back(std::move(i));
            }
            //ti.children.clear();
        }
    }

    std::vector<task_info*> collectChildren(const std::vector<oqpi::task_handle> &children, double &totalDuration)
    {
        std::vector<task_info*> infos;
        infos.reserve(children.size());
        totalDuration = 0;
        for (auto &c : children)
        {
            infos.push_back(&tasks.find(c.getUID())->second);
            totalDuration += infos.back()->duration_;
        }
        return std::move(infos);
    }

    void reset()
    {
        tasks.clear();
    }

private:
    std::recursive_mutex m;
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

    ~timer_task_context()
    {
        timing_registry::get().unregisterTask(this->owner()->getUID());
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

    ~timer_group_context()
    {
        timing_registry::get().unregisterTask(this->owner()->getUID());
    }

    inline void onTaskAdded(const oqpi::task_handle &hTask)
    {
        timing_registry::get().addToGroup(this->owner()->getUID(), hTask);
    }

    inline void onPreExecute()
    {
        timing_registry::get().startTask(this->owner()->getUID());
    }

    inline void onPostExecute()
    {
        timing_registry::get().endTask(this->owner()->getUID());
    }
};


namespace this_task {

}
