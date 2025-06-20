#include "../include/KeyValueStore.h"
#include "../include/CommandHandler.h"
#include "../include/Logger.h"
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <sstream>

using namespace std;

void testBasicOperations() {
    KeyValueStore store;
    
    // Test SET and GET
    assert(store.set("key1", "value1"));
    string value = store.get("key1");
    assert(value == "value1");
    
    // Test DEL
    assert(store.del("key1"));
    value = store.get("key1");
    assert(value.empty());
    
    // Test EXISTS
    assert(store.set("key2", "value2"));
    assert(store.exists("key2"));
    assert(!store.exists("key1"));
}

void testExpiration() {
    KeyValueStore store;
    
    // Test TTL
    assert(store.set("key1", "value1", 1));  // 1 second expiration
    string value = store.get("key1");
    assert(value == "value1");
    
    // Wait for expiration
    this_thread::sleep_for(chrono::seconds(2));
    value = store.get("key1");
    assert(value.empty());
}

void testConcurrentAccess() {
    KeyValueStore store;
    const int numThreads = 10;
    const int numOperations = 100;
    
    vector<thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&store, i, numOperations]() {
            for (int j = 0; j < numOperations; ++j) {
                string key = "key" + to_string(i) + "_" + to_string(j);
                string value = "value" + to_string(i) + "_" + to_string(j);
                store.set(key, value);
                string retrieved = store.get(key);
                assert(retrieved == value);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
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
    this_thread::sleep_for(chrono::seconds(2));
    
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
    cout << "Running KeyValueStore tests..." << endl;
    
    testBasicOperations();
    cout << "Basic operations test passed" << endl;
    
    testExpiration();
    cout << "Expiration test passed" << endl;
    
    testConcurrentAccess();
    cout << "Concurrent access test passed" << endl;
    
    testCommandHandler();
    cout << "Command handler test passed" << endl;
    
    cout << "All tests passed!" << endl;
    return 0;
} 