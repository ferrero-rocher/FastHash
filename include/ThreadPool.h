#pragma once
#include <cstddef>
#include <functional>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <type_traits>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    // Submit a task and return a future
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks_.emplace([task](){ (*task)(); });
        }
        condition_.notify_one();
        return res;
    }

    // Get the number of worker threads
    size_t threadCount() const { return threads_.size(); }

    // Get the number of active threads
    size_t activeThreadCount() const { return activeThreads_; }

    // Check if the thread pool is stopped
    bool isStopped() const {
        std::unique_lock<std::mutex> lock(queueMutex_);
        return stop_;
    }

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    std::atomic<size_t> activeThreads_;
}; 