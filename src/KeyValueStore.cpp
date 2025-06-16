#include "KeyValueStore.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

KeyValueStore::~KeyValueStore() {
    running_ = false;
    if (cleanerThread_.joinable()) {
        cleanerThread_.join();
    }
}

bool KeyValueStore::set(const string& key, const string& value, int ttl_seconds) {
    lock_guard<mutex> lock(mutex_);
    Value val;
    val.data = value;
    if (ttl_seconds > 0) {
        val.expiry = chrono::system_clock::now() + chrono::seconds(ttl_seconds);
    }
    store_[key] = val;
    updateMemoryUsage(key, value, false);
    totalOperations_++;
    return true;
}

optional<string> KeyValueStore::get(const string& key) {
    lock_guard<mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it == store_.end() || isExpired(it->second)) {
        return nullopt;
    }
    totalOperations_++;
    return it->second.data;
}

bool KeyValueStore::del(const string& key) {
    lock_guard<mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        updateMemoryUsage(it->first, it->second.data, true);
        store_.erase(it);
        totalOperations_++;
        return true;
    }
    return false;
}

bool KeyValueStore::exists(const string& key) {
    lock_guard<mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it == store_.end() || isExpired(it->second)) {
        return false;
    }
    totalOperations_++;
    return true;
}

bool KeyValueStore::expire(const string& key, int ttl_seconds) {
    lock_guard<mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it == store_.end() || isExpired(it->second)) {
        return false;
    }
    if (ttl_seconds > 0) {
        it->second.expiry = chrono::system_clock::now() + chrono::seconds(ttl_seconds);
    } else {
        it->second.expiry = nullopt;
    }
    totalOperations_++;
    return true;
}

optional<chrono::seconds> KeyValueStore::ttl(const string& key) {
    lock_guard<mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it == store_.end() || isExpired(it->second)) {
        return nullopt;
    }
    if (!it->second.expiry) {
        return chrono::seconds(-1);  // No expiry
    }
    auto remaining = chrono::duration_cast<chrono::seconds>(
        *it->second.expiry - chrono::system_clock::now());
    totalOperations_++;
    return remaining.count() > 0 ? remaining : chrono::seconds(0);
}

vector<string> KeyValueStore::getKeys() const {
    lock_guard<mutex> lock(mutex_);
    vector<string> keys;
    for (const auto& pair : store_) {
        if (!isExpired(pair.second)) {
            keys.push_back(pair.first);
        }
    }
    return keys;
}

bool KeyValueStore::clear() {
    lock_guard<mutex> lock(mutex_);
    store_.clear();
    memoryUsage_ = 0;
    totalOperations_++;
    return true;
}

bool KeyValueStore::save(const string& filename) const {
    return saveToFile(filename);
}

bool KeyValueStore::load(const string& filename) {
    return loadFromFile(filename);
}

KeyValueStoreStats KeyValueStore::getStats() const {
    lock_guard<mutex> lock(mutex_);
    KeyValueStoreStats stats;
    stats.total_keys = store_.size();
    stats.memory_usage = memoryUsage_;
    stats.totalOperations = totalOperations_;
    stats.activeThreads = activeThreads_;
    stats.uptime = chrono::duration_cast<chrono::seconds>(
        chrono::system_clock::now() - startTime_);
    
    for (const auto& pair : store_) {
        if (isExpired(pair.second)) {
            stats.expired_keys++;
        }
    }
    return stats;
}

bool KeyValueStore::flush() {
    lock_guard<mutex> lock(mutex_);
    store_.clear();
    memoryUsage_ = 0;
    totalOperations_++;
    return true;
}

void KeyValueStore::updateMemoryUsage(const string& key, const string& value, bool isDelete) {
    if (isDelete) {
        memoryUsage_ -= (key.size() + value.size());
    } else {
        memoryUsage_ += (key.size() + value.size());
    }
}

void KeyValueStore::cleanerLoop() {
    while (running_) {
        this_thread::sleep_for(chrono::seconds(1));
        lock_guard<mutex> lock(mutex_);
        for (auto it = store_.begin(); it != store_.end();) {
            if (isExpired(it->second)) {
                updateMemoryUsage(it->first, it->second.data, true);
                it = store_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool KeyValueStore::isExpired(const Value& value) const {
    if (!value.expiry) {
        return false;
    }
    return chrono::system_clock::now() >= *value.expiry;
}

bool KeyValueStore::saveToFile(const string& filename) const {
    lock_guard<mutex> lock(mutex_);
    ofstream file(filename, ios::binary);
    if (!file) {
        return false;
    }

    size_t size = store_.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (const auto& pair : store_) {
        if (!isExpired(pair.second)) {
            size_t keySize = pair.first.size();
            size_t valueSize = pair.second.data.size();
            bool hasExpiry = pair.second.expiry.has_value();

            file.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
            file.write(pair.first.c_str(), keySize);
            file.write(reinterpret_cast<const char*>(&valueSize), sizeof(valueSize));
            file.write(pair.second.data.c_str(), valueSize);
            file.write(reinterpret_cast<const char*>(&hasExpiry), sizeof(hasExpiry));

            if (hasExpiry) {
                auto expiryTime = pair.second.expiry->time_since_epoch().count();
                file.write(reinterpret_cast<const char*>(&expiryTime), sizeof(expiryTime));
            }
        }
    }

    return true;
}

bool KeyValueStore::loadFromFile(const string& filename) {
    lock_guard<mutex> lock(mutex_);
    ifstream file(filename, ios::binary);
    if (!file) {
        return false;
    }

    store_.clear();
    memoryUsage_ = 0;

    size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));

    for (size_t i = 0; i < size; ++i) {
        size_t keySize, valueSize;
        file.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));
        string key(keySize, '\0');
        file.read(&key[0], keySize);

        file.read(reinterpret_cast<char*>(&valueSize), sizeof(valueSize));
        string value(valueSize, '\0');
        file.read(&value[0], valueSize);

        Value val;
        val.data = value;

        bool hasExpiry;
        file.read(reinterpret_cast<char*>(&hasExpiry), sizeof(hasExpiry));
        if (hasExpiry) {
            chrono::system_clock::rep expiryTime;
            file.read(reinterpret_cast<char*>(&expiryTime), sizeof(expiryTime));
            val.expiry = chrono::system_clock::time_point(chrono::system_clock::duration(expiryTime));
        }

        store_[key] = val;
        updateMemoryUsage(key, value, false);
    }

    return true;
}
