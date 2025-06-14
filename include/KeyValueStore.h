#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <optional>
#include <thread>
#include <atomic>
#include "ThreadPool.h"

struct KeyValueStoreStats {
    size_t totalKeys;
    std::chrono::seconds uptime;
    size_t activeThreads;
};

class KeyValueStore {
public:
    explicit KeyValueStore(size_t numBuckets = 100);
    ~KeyValueStore();
    
    // Core operations
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    
    // Expiration
    void expire(const std::string& key, int seconds);
    bool hasExpired(const std::string& key) const;
    
    // Utility
    size_t size() const;
    void clear();
    KeyValueStoreStats getStats() const;
    void setThreadPool(ThreadPool* pool) { threadPool_ = pool; }

private:
    struct Entry {
        std::string value;
        std::chrono::system_clock::time_point expiry;
        bool hasExpiry;
    };

    struct Bucket {
        std::unordered_map<std::string, Entry> entries;
        mutable std::shared_mutex mutex;
    };

    // Get bucket index for a key
    size_t getBucketIndex(const std::string& key) const;
    
    // Get bucket for a key
    Bucket& getBucket(const std::string& key);
    const Bucket& getBucket(const std::string& key) const;

    std::vector<Bucket> buckets_;
    size_t numBuckets_;

    // TTL cleaner thread
    std::thread cleanerThread_;
    std::atomic<bool> stopCleaner_;
    void cleanerLoop();
    std::chrono::system_clock::time_point startTime_;
    ThreadPool* threadPool_ = nullptr;
}; 