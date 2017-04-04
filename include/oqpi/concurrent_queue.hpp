#pragma once

#include <mutex>
#include <queue>


#if OQPI_PLATFORM_WIN
#   pragma warning (push)
#   pragma warning (disable : 4127)
#   pragma warning (disable : 4706)
#endif

#include "oqpi/concurrentqueue.h"


template <typename T>
class concurrent_queue
{
    using lock_t = std::lock_guard<std::mutex>;

public:
    void push(const T &t)
    {
//         lock_t __l(mutex_);
//         queue_.emplace(t);
        queue_.enqueue(t);
    }

    template<typename _It>
    void push(_It it, size_t count)
    {
        queue_.enqueue_bulk(it, count);
    }

    bool tryPop(typename std::queue<T>::reference v)
    {
//         lock_t __l(mutex_);
//         if (!queue_.empty())
//         {
//             v = std::move(queue_.front());
//             queue_.pop();
//             return true;
//         }
//         return false;
        return queue_.try_dequeue(v);
    }

    bool empty() const
    {
        return queue_.empty();
    }

private:
//     std::mutex      mutex_;
//     std::queue<T>   queue_;
    moodycamel::ConcurrentQueue<T> queue_;
};

#if OQPI_PLATFORM_WIN
#   pragma warning (pop)
#endif
