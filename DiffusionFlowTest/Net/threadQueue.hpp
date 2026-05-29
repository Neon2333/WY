#ifndef THREADQUEUE_HPP
#define THREADQUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue
{
public:
    void push(const T& value)
    {
        std::unique_lock<std::mutex> ul(mutex_);
        queue_.push(value);
        cv_.notify_one();          
    }

    // 阻塞式弹出
    T pop()                     
    {
        std::unique_lock<std::mutex> ul(mutex_);
        cv_.wait(ul, [this](){return !queue_.empty();});
        T value = queue_.front();
        queue_.pop();
        return value;
    }

    // 非阻塞试弹出
    bool tryPop(T &value)       
    {
        std::unique_lock<std::mutex> ul(mutex);
        if (queue_.empty())
            return false;
        value = queue_.front();
        queue_.pop();
        return true;
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

#endif