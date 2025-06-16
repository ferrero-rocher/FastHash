#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>
#include <vector>
#include <thread>
#include <atomic>
#include "Logger.h"

using namespace std;

struct KeyValueStoreStats {
    size_t total_keys = 0;
    size_t expired_keys = 0;
    size_t memory_usage = 0;
    size_t totalOperations = 0;
    size_t activeThreads = 0;
    chrono::seconds uptime{0};
};

class KeyValueStore {
public:
    KeyValueStore() = default;
    KeyValueStore(const KeyValueStore&) = delete;
    KeyValueStore& operator=(const KeyValueStore&) = delete;
    KeyValueStore(KeyValueStore&&) = default;
    ~KeyValueStore();

    bool set(const string& key, const string& value, int ttl_seconds = 0);
    optional<string> get(const string& key);
    bool del(const string& key);
    bool exists(const string& key);
    bool expire(const string& key, int ttl_seconds);
    optional<chrono::seconds> ttl(const string& key);
    vector<string> getKeys() const;
    bool clear();
    bool save(const string& filename) const;
    bool load(const string& filename);
    KeyValueStoreStats getStats() const;
    bool flush();

    void incrementActiveThreads() { activeThreads_++; }
    void decrementActiveThreads() { activeThreads_--; }

private:
    struct Value {
        string data;
        optional<chrono::system_clock::time_point> expiry;
    };

    unordered_map<string, Value> store_;
    mutable mutex mutex_;
    atomic<size_t> memoryUsage_{0};
    atomic<size_t> totalOperations_{0};
    atomic<size_t> activeThreads_{0};
    chrono::system_clock::time_point startTime_{chrono::system_clock::now()};
    thread cleanerThread_;
    atomic<bool> running_{true};

    void updateMemoryUsage(const string& key, const string& value, bool isDelete);
    void cleanerLoop();
    bool isExpired(const Value& value) const;
    bool saveToFile(const string& filename) const;
    bool loadFromFile(const string& filename);
}; 