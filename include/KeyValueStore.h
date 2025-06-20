#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include <optional>
#include "Logger.h"

using namespace std;

struct StoreStats {
    size_t totalOperations;
    size_t memoryUsage;
    size_t activeThreads;
    size_t totalKeys;
};

class KeyValueStore {
public:
    KeyValueStore();
    ~KeyValueStore();

    // Core operations
    bool set(const string& key, const string& value, int ttl = 0);
    string get(const string& key);
    bool del(const string& key);
    bool exists(const string& key);
    vector<string> keys();
    void clear();
    bool save(const string& filename);
    bool load(const string& filename);
    bool flush(const string& filename);
    StoreStats getStats();
    bool expire(const string& key, int ttl_seconds);
    optional<chrono::seconds> ttl(const string& key);

private:
    struct Value {
        string value;
        chrono::system_clock::time_point expiry;
    };

    unordered_map<string, Value> store_;
    mutex mutex_;
    thread cleanerThread_;
    bool running_;
    size_t memoryUsage_;
    size_t totalOperations_;
    size_t activeThreads_;
    Logger& logger_;

    void cleanerLoop();
    bool isExpired(const Value& value) const;
}; 