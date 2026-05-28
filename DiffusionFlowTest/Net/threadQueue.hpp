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
        std::unique_lock<std::mutex> ul(mutex);
        queue_.push(value);
        cv_.notify_one();          // 通知等待的消费者
    }

    T pop()                     // 阻塞等待
    {
        std::unique_lock<std::mutex> ul(mutex);
        while (queue_.isEmpty()) 
        {
            cv_.wait(&mutex);
        }
        return queue_.dequeue();
    }

    bool tryPop(T &value)       // 非阻塞尝试
    {
        std::unique_lock<std::mutex> ul(mutex);
        if (queue_.isEmpty()) 
            return false;
        value = queue_.dequeue();
        return true;
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

#endif