#include "CommandHandler.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <fstream>
#include <iomanip>

using namespace std;

CommandHandler::CommandHandler(KeyValueStore& store, Logger& logger) : store_(store), logger_(logger) {}

string CommandHandler::handleCommand(const string& command) {
    istringstream iss(command);
    string cmd;
    iss >> cmd;
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "SET") {
        string key, value;
        int ttl = 0;
        if (iss >> key >> value) {
            iss >> ttl;  // Optional TTL
            if (store_.set(key, value, ttl)) {
                logger_.info("SET " + key + " " + value + (ttl > 0 ? " " + to_string(ttl) : ""));
                return "OK";
            }
        }
        return "ERROR: Invalid SET command";
    }
    else if (cmd == "GET") {
        string key;
        if (iss >> key) {
            auto value = store_.get(key);
            if (value) {
                logger_.info("GET " + key + " -> " + *value);
                return *value;
            }
            return "(nil)";
        }
        return "ERROR: Invalid GET command";
    }
    else if (cmd == "DEL") {
        string key;
        if (iss >> key) {
            if (store_.del(key)) {
                logger_.info("DEL " + key);
                return "OK";
            }
            return "ERROR: Key not found";
        }
        return "ERROR: Invalid DEL command";
    }
    else if (cmd == "EXISTS") {
        string key;
        if (iss >> key) {
            bool exists = store_.exists(key);
            logger_.info("EXISTS " + key + " -> " + (exists ? "true" : "false"));
            return exists ? "1" : "0";
        }
        return "ERROR: Invalid EXISTS command";
    }
    else if (cmd == "EXPIRE") {
        string key;
        int ttl;
        if (iss >> key >> ttl) {
            if (store_.expire(key, ttl)) {
                logger_.info("EXPIRE " + key + " " + to_string(ttl));
                return "OK";
            }
            return "ERROR: Key not found";
        }
        return "ERROR: Invalid EXPIRE command";
    }
    else if (cmd == "TTL") {
        string key;
        if (iss >> key) {
            auto ttl = store_.ttl(key);
            if (ttl) {
                logger_.info("TTL " + key + " -> " + to_string(ttl->count()));
                return to_string(ttl->count());
            }
            return "(nil)";
        }
        return "ERROR: Invalid TTL command";
    }
    else if (cmd == "KEYS") {
        auto keys = store_.getKeys();
        if (keys.empty()) {
            return "(empty)";
        }
        stringstream ss;
        for (size_t i = 0; i < keys.size(); ++i) {
            ss << keys[i];
            if (i < keys.size() - 1) {
                ss << "\n";
            }
        }
        logger_.info("KEYS -> " + to_string(keys.size()) + " keys");
        return ss.str();
    }
    else if (cmd == "CLEAR") {
        if (store_.clear()) {
            logger_.info("CLEAR");
            return "OK";
        }
        return "ERROR: Failed to clear store";
    }
    else if (cmd == "SAVE") {
        string filename;
        if (iss >> filename) {
            if (store_.save(filename)) {
                logger_.info("SAVE " + filename);
                return "OK";
            }
            return "ERROR: Failed to save to file";
        }
        return "ERROR: Invalid SAVE command";
    }
    else if (cmd == "LOAD") {
        string filename;
        if (iss >> filename) {
            if (store_.load(filename)) {
                logger_.info("LOAD " + filename);
                return "OK";
            }
            return "ERROR: Failed to load from file";
        }
        return "ERROR: Invalid LOAD command";
    }
    else if (cmd == "STATS") {
        auto stats = store_.getStats();
        stringstream ss;
        ss << "Total Keys: " << stats.total_keys << "\n"
           << "Memory Usage: " << stats.memory_usage << " bytes\n"
           << "Total Operations: " << stats.totalOperations << "\n"
           << "Active Threads: " << stats.activeThreads << "\n"
           << "Uptime: " << stats.uptime.count() << " seconds\n"
           << "Expired Keys: " << stats.expired_keys;
        logger_.info("STATS");
        return ss.str();
    }
    else if (cmd == "FLUSH") {
        if (store_.flush()) {
            logger_.info("FLUSH");
            return "OK";
        }
        return "ERROR: Failed to flush store";
    }
    else if (cmd == "QUIT") {
        logger_.info("QUIT");
        return "BYE";
    }
    else {
        return "ERROR: Unknown command";
    }
}

string CommandHandler::handleSet(istringstream& iss) {
    string key, value;
    if (!(iss >> key >> value)) {
        return "ERROR: SET requires key and value";
    }
    
    int ttl = 0;
    if (iss >> ttl) {
        store_.set(key, value, ttl);
    } else {
        store_.set(key, value);
    }
    
    return "OK";
}

string CommandHandler::handleGet(istringstream& iss) {
    string key;
    if (!(iss >> key)) {
        return "ERROR: GET requires a key";
    }
    
    auto value = store_.get(key);
    if (value) {
        logger_.info("GET " + key + " = " + *value);
        return *value;
    }
    return "Key not found";
}

string CommandHandler::handleDel(istringstream& iss) {
    string key;
    if (!(iss >> key)) {
        return "ERROR: DEL requires a key";
    }
    
    if (store_.del(key)) {
        logger_.info("DEL " + key);
        return "OK";
    }
    return "Key not found";
}

string CommandHandler::handleExists(istringstream& iss) {
    string key;
    if (!(iss >> key)) {
        return "ERROR: EXISTS requires a key";
    }
    
    return store_.exists(key) ? "1" : "0";
}

string CommandHandler::handleExpire(istringstream& iss) {
    string key;
    int ttl;
    if (!(iss >> key >> ttl)) {
        return "ERROR: EXPIRE requires key and TTL";
    }
    
    if (store_.expire(key, ttl)) {
        logger_.info("EXPIRE " + key + " " + to_string(ttl));
        return "OK";
    }
    return "Key not found";
}

string CommandHandler::handleTtl(istringstream& iss) {
    string key;
    if (!(iss >> key)) {
        return "ERROR: TTL requires a key";
    }
    
    auto ttl = store_.ttl(key);
    if (ttl) {
        logger_.info("TTL " + key + " = " + to_string(ttl->count()) + " seconds");
        return to_string(ttl->count());
    }
    return "Key not found or has no TTL";
}

string CommandHandler::handleKeys(istringstream& iss) {
    auto keys = store_.getKeys();
    if (keys.empty()) {
        return "No keys found";
    }
    
    stringstream ss;
    for (size_t i = 0; i < keys.size(); ++i) {
        ss << keys[i];
        if (i < keys.size() - 1) {
            ss << ", ";
        }
    }
    logger_.info("KEYS command executed");
    return ss.str();
}

string CommandHandler::handleClear(istringstream& iss) {
    if (store_.clear()) {
        logger_.info("CLEAR");
        return "OK";
    }
    return "ERROR: Failed to clear store";
}

string CommandHandler::handleSave(istringstream& iss) {
    string filename;
    if (!(iss >> filename)) {
        return "ERROR: SAVE requires a filename";
    }
    
    if (store_.save(filename)) {
        logger_.info("SAVE to " + filename);
        return "OK";
    }
    return "ERROR: Failed to save to file";
}

string CommandHandler::handleLoad(istringstream& iss) {
    string filename;
    if (!(iss >> filename)) {
        return "ERROR: LOAD requires a filename";
    }
    
    if (store_.load(filename)) {
        logger_.info("LOAD from " + filename);
        return "OK";
    }
    return "ERROR: Failed to load from file";
}

string CommandHandler::handleDump(istringstream& iss) {
    auto stats = store_.getStats();
    stringstream ss;
    ss << "Total keys: " << stats.total_keys << "\n"
       << "Expired keys: " << stats.expired_keys << "\n"
       << "Memory usage: " << stats.memory_usage << " bytes";
    return ss.str();
}

string CommandHandler::handleHelp(istringstream& iss) {
    return "Available commands:\n"
           "SET key value [ttl] - Set a key-value pair with optional TTL\n"
           "GET key - Get value for a key\n"
           "DEL key - Delete a key\n"
           "EXISTS key - Check if a key exists\n"
           "EXPIRE key ttl - Set TTL for a key\n"
           "TTL key - Get remaining TTL for a key\n"
           "KEYS - List all keys\n"
           "CLEAR - Clear all keys\n"
           "SAVE filename - Save to file\n"
           "LOAD filename - Load from file\n"
           "DUMP - Show statistics\n"
           "HELP - Show this help\n"
           "FLUSH - Clear and save to last used file";
}

string CommandHandler::handleFlush(istringstream& iss) {
    if (store_.flush()) {
        logger_.info("FLUSH");
        return "OK";
    }
    return "ERROR: No dump file has been used yet. Use SAVE first.";
} 