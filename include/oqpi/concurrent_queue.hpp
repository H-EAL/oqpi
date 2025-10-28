#pragma once

#include <mutex>
#include <queue>

#include "concurrentqueue.h"


template<typename T>
class concurrent_queue
    : public moodycamel::ConcurrentQueue<T>
{
public:
    void push(T &&t)
    {
        moodycamel::ConcurrentQueue<T>::enqueue(std::move(t));
    }

    void push(const T &t)
    {
        moodycamel::ConcurrentQueue<T>::enqueue(t);
    }

    bool tryPop(T &item)
    {
        return moodycamel::ConcurrentQueue<T>::try_dequeue(item);
    }

    bool empty() const
    {
        return moodycamel::ConcurrentQueue<T>::size_approx() == 0;
    }
};

template <typename T>
class concurrent_queue_
{
    using lock_t = std::lock_guard<std::mutex>;

public:
    void push(T &&t)
    {
        lock_t __l(mutex_);
        queue_.emplace(std::move(t));
    }

    void push(const T &t)
    {
        lock_t __l(mutex_);
        queue_.emplace(t);
    }

    bool tryPop(typename std::queue<T>::reference v)
    {
        lock_t __l(mutex_);
        if (!queue_.empty())
        {
            v = std::move(queue_.front());
            queue_.pop();
            return true;
        }
        return false;
    }

    bool empty() const
    {
        return queue_.empty();
    }

private:
    std::mutex      mutex_;
    std::queue<T>   queue_;
};
