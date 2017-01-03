#pragma once

#include <mutex>
#include <queue>

template <typename T, typename _Mutex>
class qqueue
{
public:
    qqueue(/*const std::string &name = ""*/)
        //: mutex_("QueueMutex/" + name)
    {}

    ~qqueue() = default;

    void push(T &&t)
    {
        std::lock_guard<_Mutex> lock(mutex_);
        queue_.emplace(std::move(t));
    }

    void push(const T &t)
    {
        std::lock_guard<_Mutex> lock(mutex_);
        queue_.emplace(t);
    }

    bool try_pop(typename std::queue<T>::reference v)
    {
        std::lock_guard<_Mutex> lock(mutex_);
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
    std::queue<T>   queue_;
    _Mutex          mutex_;
};
