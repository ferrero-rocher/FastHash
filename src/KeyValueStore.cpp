#include "KeyValueStore.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

using namespace std;

KeyValueStore::KeyValueStore() : 
    running_(true), 
    memoryUsage_(0), 
    totalOperations_(0), 
    activeThreads_(0),
    logger_(Logger::getInstance()) {
    cleanerThread_ = thread(&KeyValueStore::cleanerLoop, this);
}

KeyValueStore::~KeyValueStore() {
    running_ = false;
    if (cleanerThread_.joinable()) {
        cleanerThread_.join();
    }
}

bool KeyValueStore::set(const string& key, const string& value, int ttl) {
    lock_guard<mutex> lock(mutex_);
    Value v;
    v.value = value;
    if (ttl > 0) {
        v.expiry = chrono::system_clock::now() + chrono::seconds(ttl);
    }
    store_[key] = v;
    totalOperations_++;
    memoryUsage_ += key.size() + value.size();
    // logger_.info("SET operation: key=" + key + ", value=" + value + (ttl > 0 ? ", ttl=" + to_string(ttl) : ""));
    return true;
}

string KeyValueStore::get(const string& key) {
    lock_guard<mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        if (isExpired(it->second)) {
            store_.erase(it);
            // logger_.info("GET operation: key=" + key + " (expired)");
            return "";
        }
        // logger_.info("GET operation: key=" + key + ", value=" + it->second.value);
        return it->second.value;
    }
    // logger_.info("GET operation: key=" + key + " (not found)");
    return "";
}

bool KeyValueStore::del(const string& key) {
    lock_guard<mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        memoryUsage_ -= key.size() + it->second.value.size();
        store_.erase(it);
        totalOperations_++;
        // logger_.info("DEL operation: key=" + key + " (deleted)");
        return true;
    }
    // logger_.info("DEL operation: key=" + key + " (not found)");
    return false;
}

bool KeyValueStore::exists(const string& key) {
    lock_guard<mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        if (isExpired(it->second)) {
            store_.erase(it);
            // logger_.info("EXISTS operation: key=" + key + " (expired)");
            return false;
        }
        // logger_.info("EXISTS operation: key=" + key + " (exists)");
        return true;
    }
    // logger_.info("EXISTS operation: key=" + key + " (not found)");
    return false;
}

bool KeyValueStore::expire(const string& key, int ttl_seconds) {
    lock_guard<mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        it->second.expiry = chrono::system_clock::now() + chrono::seconds(ttl_seconds);
        // logger_.info("EXPIRE operation: key=" + key + ", ttl=" + to_string(ttl_seconds));
        return true;
    }
    // logger_.info("EXPIRE operation: key=" + key + " (not found)");
    return false;
}

optional<chrono::seconds> KeyValueStore::ttl(const string& key) {
    lock_guard<mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        if (isExpired(it->second)) {
            // logger_.info("TTL operation: key=" + key + " (expired)");
            return nullopt;
        }
        auto now = chrono::system_clock::now();
        auto ttl = chrono::duration_cast<chrono::seconds>(it->second.expiry - now);
        // logger_.info("TTL operation: key=" + key + ", ttl=" + to_string(ttl.count()));
        return ttl;
    }
    // logger_.info("TTL operation: key=" + key + " (not found)");
    return nullopt;
}

vector<string> KeyValueStore::keys() {
    lock_guard<mutex> lock(mutex_);
    vector<string> result;
    for (const auto& pair : store_) {
        if (!isExpired(pair.second)) {
            result.push_back(pair.first);
        }
    }
    // logger_.info("KEYS operation: returned " + to_string(result.size()) + " keys");
    return result;
}

void KeyValueStore::clear() {
    lock_guard<mutex> lock(mutex_);
    store_.clear();
    memoryUsage_ = 0;
    totalOperations_++;
    // logger_.info("CLEAR operation: all keys removed");
}

bool KeyValueStore::save(const string& filename) {
    lock_guard<mutex> lock(mutex_);
    ofstream file(filename);
    if (!file) {
        // logger_.error("SAVE operation: failed to open file " + filename);
        return false;
    }
    
    for (const auto& pair : store_) {
        if (!isExpired(pair.second)) {
            file << pair.first << " " << pair.second.value << "\n";
        }
    }
    
    // logger_.info("SAVE operation: saved to " + filename);
    return true;
}

bool KeyValueStore::load(const string& filename) {
    lock_guard<mutex> lock(mutex_);
    ifstream file(filename);
    if (!file) {
        // logger_.error("LOAD operation: failed to open file " + filename);
        return false;
    }
    
    store_.clear();
    memoryUsage_ = 0;
    string key, value;
    while (file >> key >> value) {
        Value v;
        v.value = value;
        store_[key] = v;
        memoryUsage_ += key.size() + value.size();
    }
    
    // logger_.info("LOAD operation: loaded from " + filename);
    return true;
}

bool KeyValueStore::flush(const string& filename) {
    lock_guard<mutex> lock(mutex_);
    ofstream file(filename);
    if (!file) {
        // logger_.error("FLUSH operation: failed to open file " + filename);
        return false;
    }
    
    for (const auto& pair : store_) {
        if (!isExpired(pair.second)) {
            file << pair.first << " " << pair.second.value << "\n";
        }
    }
    
    store_.clear();
    memoryUsage_ = 0;
    // logger_.info("FLUSH operation: flushed to " + filename);
    return true;
}

StoreStats KeyValueStore::getStats() {
    lock_guard<mutex> lock(mutex_);
    StoreStats stats;
    stats.totalOperations = totalOperations_;
    stats.memoryUsage = memoryUsage_;
    stats.activeThreads = activeThreads_;
    stats.totalKeys = store_.size();
    // logger_.info("STATS operation: retrieved statistics");
    return stats;
}

void KeyValueStore::cleanerLoop() {
    while (running_) {
        {
            lock_guard<mutex> lock(mutex_);
            for (auto it = store_.begin(); it != store_.end();) {
                if (isExpired(it->second)) {
                    memoryUsage_ -= it->first.size() + it->second.value.size();
                    it = store_.erase(it);
                    // logger_.info("Cleaner: removed expired key");
                } else {
                    ++it;
                }
            }
        }
        this_thread::sleep_for(chrono::seconds(1));
    }
}

bool KeyValueStore::isExpired(const Value& value) const {
    if (value.expiry == chrono::system_clock::time_point()) {
        return false;
    }
    return chrono::system_clock::now() >= value.expiry;
}
