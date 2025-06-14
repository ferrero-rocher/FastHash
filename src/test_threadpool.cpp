#include "ThreadPool.h"
#include <iostream>
#include <chrono>
#include <string>

int main() {
    try {
        // Create a thread pool with 4 threads
        ThreadPool pool(4);
        std::cout << "ThreadPool created with " << pool.threadCount() << " threads\n";

        // Test basic task execution
        auto result1 = pool.enqueue([](int x) { return x * x; }, 12);
        std::cout << "12 squared is: " << result1.get() << std::endl;

        // Test multiple tasks
        std::vector<std::future<int>> results;
        for (int i = 0; i < 8; ++i) {
            results.emplace_back(
                pool.enqueue([i] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    return i * i;
                })
            );
        }

        // Get results
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "Task " << i << " result: " << results[i].get() << std::endl;
        }

        // Test exception handling
        try {
            auto result = pool.enqueue([]() { throw std::runtime_error("Test exception"); });
            result.get();
        } catch (const std::exception& e) {
            std::cout << "Caught exception: " << e.what() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 