#include "KeyValueStore.h"
#include <functional>
#include <stdexcept>
#include <iostream>

KeyValueStore::KeyValueStore(size_t numBuckets) 
    : buckets_(numBuckets), numBuckets_(numBuckets), stopCleaner_(false), startTime_(std::chrono::system_clock::now()), threadPool_(nullptr) {
    if (numBuckets == 0) {
        throw std::invalid_argument("Number of buckets must be greater than 0");
    }
    cleanerThread_ = std::thread(&KeyValueStore::cleanerLoop, this);
}

KeyValueStore::~KeyValueStore() {
    stopCleaner_ = true;
    if (cleanerThread_.joinable()) {
        cleanerThread_.join();
    }
}

void KeyValueStore::cleanerLoop() {
    using namespace std::chrono_literals;
    while (!stopCleaner_) {
        std::this_thread::sleep_for(1s);
        for (auto& bucket : buckets_) {
            std::unique_lock<std::shared_mutex> lock(bucket.mutex);
            for (auto it = bucket.entries.begin(); it != bucket.entries.end(); ) {
                if (it->second.hasExpiry && std::chrono::system_clock::now() > it->second.expiry) {
                    it = bucket.entries.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

size_t KeyValueStore::getBucketIndex(const std::string& key) const {
    return std::hash<std::string>{}(key) % numBuckets_;
}

KeyValueStore::Bucket& KeyValueStore::getBucket(const std::string& key) {
    return buckets_[getBucketIndex(key)];
}

const KeyValueStore::Bucket& KeyValueStore::getBucket(const std::string& key) const {
    return buckets_[getBucketIndex(key)];
}

void KeyValueStore::set(const std::string& key, const std::string& value) {
    auto& bucket = getBucket(key);
    std::unique_lock<std::shared_mutex> lock(bucket.mutex);
    bucket.entries[key] = Entry{value, std::chrono::system_clock::time_point(), false};
}

std::optional<std::string> KeyValueStore::get(const std::string& key) {
    auto& bucket = getBucket(key);
    std::shared_lock<std::shared_mutex> lock(bucket.mutex);
    
    auto it = bucket.entries.find(key);
    if (it == bucket.entries.end()) {
        return std::nullopt;
    }

    const auto& entry = it->second;
    if (entry.hasExpiry && std::chrono::system_clock::now() > entry.expiry) {
        // Upgrade to unique lock to remove expired entry
        lock.unlock();
        std::unique_lock<std::shared_mutex> writeLock(bucket.mutex);
        bucket.entries.erase(it);
        return std::nullopt;
    }

    return entry.value;
}

bool KeyValueStore::del(const std::string& key) {
    auto& bucket = getBucket(key);
    std::unique_lock<std::shared_mutex> lock(bucket.mutex);
    return bucket.entries.erase(key) > 0;
}

void KeyValueStore::expire(const std::string& key, int seconds) {
    auto& bucket = getBucket(key);
    std::unique_lock<std::shared_mutex> lock(bucket.mutex);
    
    auto it = bucket.entries.find(key);
    if (it != bucket.entries.end()) {
        it->second.expiry = std::chrono::system_clock::now() + 
                           std::chrono::seconds(seconds);
        it->second.hasExpiry = true;
    }
}

bool KeyValueStore::hasExpired(const std::string& key) const {
    const auto& bucket = getBucket(key);
    std::shared_lock<std::shared_mutex> lock(bucket.mutex);
    
    auto it = bucket.entries.find(key);
    if (it == bucket.entries.end()) {
        return true;
    }

    const auto& entry = it->second;
    return entry.hasExpiry && std::chrono::system_clock::now() > entry.expiry;
}

size_t KeyValueStore::size() const {
    size_t total = 0;
    for (const auto& bucket : buckets_) {
        std::shared_lock<std::shared_mutex> lock(bucket.mutex);
        total += bucket.entries.size();
    }
    return total;
}

void KeyValueStore::clear() {
    for (auto& bucket : buckets_) {
        std::unique_lock<std::shared_mutex> lock(bucket.mutex);
        bucket.entries.clear();
    }
}

KeyValueStoreStats KeyValueStore::getStats() const {
    KeyValueStoreStats stats;
    stats.totalKeys = size();
    stats.uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - startTime_
    );
    stats.activeThreads = threadPool_ ? threadPool_->activeThreadCount() : 0;
    return stats;
} 