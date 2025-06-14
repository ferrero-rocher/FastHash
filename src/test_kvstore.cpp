#include "KeyValueStore.h"
#include "CommandHandler.h"
#include <iostream>
#include <thread>
#include <vector>
#include <cassert>

void testBasicOperations() {
    KeyValueStore store;
    
    // Test SET and GET
    store.set("key1", "value1");
    auto value = store.get("key1");
    assert(value && *value == "value1");
    std::cout << "Basic SET/GET test passed\n";

    // Test DEL
    assert(store.del("key1"));
    assert(!store.get("key1"));
    std::cout << "DEL test passed\n";

    // Test non-existent key
    assert(!store.get("nonexistent"));
    std::cout << "Non-existent key test passed\n";
}

void testExpiration() {
    KeyValueStore store;
    
    store.set("key1", "value1");
    store.expire("key1", 1);  // 1 second expiration
    assert(store.get("key1"));  // Should still exist
    std::this_thread::sleep_for(std::chrono::seconds(2));
    assert(!store.get("key1"));  // Should be expired
    std::cout << "Expiration test passed\n";
}

void testConcurrentAccess() {
    KeyValueStore store;
    std::vector<std::thread> threads;
    const int numThreads = 4;
    const int numOperations = 1000;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&store, i, numOperations]() {
            for (int j = 0; j < numOperations; ++j) {
                std::string key = "key" + std::to_string(i) + "_" + std::to_string(j);
                store.set(key, "value");
                store.expire(key, 1);
            }
        });
    }

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&store, i, numOperations]() {
            for (int j = 0; j < numOperations; ++j) {
                std::string key = "key" + std::to_string(i) + "_" + std::to_string(j);
                store.get(key);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Concurrent access test passed\n";
}

void testBackgroundCleaner() {
    KeyValueStore store;
    store.set("temp", "value");
    store.expire("temp", 1); // 1 second TTL
    std::this_thread::sleep_for(std::chrono::seconds(3));
    assert(!store.get("temp"));
    std::cout << "Background cleaner test passed\n";
}

void testCommandHandler() {
    KeyValueStore store;
    CommandHandler handler(store);
    
    // SET
    assert(handler.handle("SET foo bar") == "OK");
    // GET
    assert(handler.handle("GET foo") == "bar");
    // DEL
    assert(handler.handle("DEL foo") == "OK");
    assert(handler.handle("GET foo") == "NOT_FOUND");
    // EXPIRE
    handler.handle("SET temp value");
    assert(handler.handle("EXPIRE temp 1") == "OK");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    assert(handler.handle("GET temp") == "NOT_FOUND");
    assert(handler.handle("").find("ERROR") == 0);
    assert(handler.handle("SET onlykey").find("ERROR") == 0);
    assert(handler.handle("GET").find("ERROR") == 0);
    assert(handler.handle("DEL").find("ERROR") == 0);
    assert(handler.handle("EXPIRE temp notanumber").find("ERROR") == 0);
    assert(handler.handle("UNKNOWN foo").find("ERROR") == 0);
    std::cout << "CommandHandler test passed\n";
}

int main() {
    try {
        std::cout << "Starting KeyValueStore tests...\n";
        
        testBasicOperations();
        testExpiration();
        testConcurrentAccess();
        testBackgroundCleaner();
        testCommandHandler();
        
        std::cout << "All tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
} 