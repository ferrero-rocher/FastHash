#include "../include/KeyValueStore.h"
#include "../include/CommandHandler.h"
#include "../include/Logger.h"
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <sstream>

void testBasicOperations() {
    KeyValueStore store;
    Logger& logger = Logger::getInstance();
    CommandHandler handler(store, logger);
    
    // Test SET and GET
    store.set("key1", "value1");
    auto value = store.get("key1");
    assert(value && *value == "value1");
    
    // Test DEL
    store.del("key1");
    assert(!store.get("key1"));
    
    // Cleanup
    store.clear();
}

void testExpiration() {
    KeyValueStore store;
    Logger& logger = Logger::getInstance();
    CommandHandler handler(store, logger);
    
    store.set("key1", "value1");
    store.expire("key1", 1);  // 1 second expiration
    
    // Value should exist
    auto value = store.get("key1");
    assert(value && *value == "value1");
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Value should be expired
    assert(!store.get("key1"));
    
    // Cleanup
    store.clear();
}

void testConcurrentAccess() {
    KeyValueStore store;
    Logger& logger = Logger::getInstance();
    CommandHandler handler(store, logger);
    
    std::vector<std::thread> threads;
    const int numThreads = 10;
    const int numOperations = 100;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&store, i, numOperations]() {
            for (int j = 0; j < numOperations; ++j) {
                std::string key = "key" + std::to_string(i) + "_" + std::to_string(j);
                std::string value = "value" + std::to_string(i) + "_" + std::to_string(j);
                
                store.set(key, value);
                auto retrieved = store.get(key);
                assert(retrieved && *retrieved == value);
                store.expire(key, 1);
                store.del(key);
                assert(!store.get(key));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Cleanup
    store.clear();
}

void testCommandHandler() {
    KeyValueStore store;
    Logger& logger = Logger::getInstance();
    CommandHandler handler(store, logger);
    
    // Test SET command
    assert(handler.handleCommand("SET foo bar") == "OK");
    
    // Test GET command
    assert(handler.handleCommand("GET foo") == "bar");
    
    // Test DEL command
    assert(handler.handleCommand("DEL foo") == "OK");
    
    // Test GET after DEL
    assert(handler.handleCommand("GET foo") == "Key not found");
    
    // Test EXPIRE command
    assert(handler.handleCommand("SET temp value") == "OK");
    assert(handler.handleCommand("EXPIRE temp 1") == "OK");
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Test GET after expiration
    assert(handler.handleCommand("GET temp") == "Key not found");
    
    // Test error cases
    assert(handler.handleCommand("SET onlykey").find("ERROR") == 0);
    assert(handler.handleCommand("GET").find("ERROR") == 0);
    assert(handler.handleCommand("DEL").find("ERROR") == 0);
    assert(handler.handleCommand("EXPIRE temp notanumber").find("ERROR") == 0);
    
    // Cleanup
    store.clear();
}

int main() {
    try {
        testBasicOperations();
        testExpiration();
        testConcurrentAccess();
        testCommandHandler();
        
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
} 