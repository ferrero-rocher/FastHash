#include "ThreadPool.h"
#include <iostream>
#include <stdexcept>

ThreadPool::ThreadPool(size_t numThreads) : stop_(false) {
    if (numThreads == 0) {
        throw std::invalid_argument("ThreadPool must have at least one thread");
    }

    for (size_t i = 0; i < numThreads; ++i) {
        threads_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                    if (stop_ && tasks_.empty()) {
                        return;
                    }
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                try {
                    task();
                } catch (const std::exception& e) {
                    // Log or handle the exception
                    std::cerr << "Exception in thread pool: " << e.what() << std::endl;
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (std::thread& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
} 