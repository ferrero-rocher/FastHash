#include "CommandHandler.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;

string CommandHandler::handleCommand(const string& command) {
    istringstream iss(command);
    string cmd;
    iss >> cmd;
    
    // Convert command to uppercase
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    
    if (cmd.empty()) {
        return "ERROR: Empty command";
    }
    
    try {
        if (cmd == "SET") {
            return handleSet(iss);
        } else if (cmd == "GET") {
            return handleGet(iss);
        } else if (cmd == "DEL") {
            return handleDel(iss);
        } else if (cmd == "EXISTS") {
            return handleExists(iss);
        } else if (cmd == "KEYS") {
            return handleKeys(iss);
        } else if (cmd == "STATS") {
            return handleStats(iss);
        } else if (cmd == "SAVE") {
            return handleSave(iss);
        } else if (cmd == "LOAD") {
            return handleLoad(iss);
        } else if (cmd == "CLEAR") {
            return handleClear(iss);
        } else if (cmd == "FLUSH") {
            return handleFlush(iss);
        } else if (cmd == "HELP") {
            return handleHelp(iss);
        } else if (cmd == "QUIT") {
            return handleQuit(iss);
        } else {
            return "ERROR: Unknown command";
        }
    } catch (const exception& e) {
        logger_.error("Error handling command: " + string(e.what()));
        return "ERROR: Internal server error";
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

    string value = store_.get(key);
    if (value.empty()) {
        return "(nil)";
    }
    return value;
}

string CommandHandler::handleDel(istringstream& iss) {
    string key;
    if (!(iss >> key)) {
        return "ERROR: DEL requires a key";
    }
    
    if (store_.del(key)) {
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
    auto keys = store_.keys();
    if (keys.empty()) {
        return "(empty)";
    }
    
    stringstream ss;
    for (const auto& key : keys) {
        ss << key << "\n";
    }
    return ss.str();
}

string CommandHandler::handleClear(istringstream& iss) {
    store_.clear();
    return "OK";
}

string CommandHandler::handleSave(istringstream& iss) {
    string filename;
    if (!(iss >> filename)) {
        return "ERROR: SAVE requires a filename";
    }
    
    if (store_.save(filename)) {
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
        return "OK";
    }
    return "ERROR: Failed to load from file";
}

string CommandHandler::handleDump(istringstream& iss) {
    auto stats = store_.getStats();
    stringstream ss;
    ss << "Total operations: " << stats.totalOperations << "\n"
       << "Active threads: " << stats.activeThreads << "\n"
       << "Total keys: " << stats.totalKeys << "\n"
       << "Memory usage: " << stats.memoryUsage << " bytes";
    logger_.info("DUMP command: statistics retrieved");
    return ss.str();
}

string CommandHandler::handleHelp(istringstream& iss) {
    return "Commands:\n"
           "  SET <key> <value> [ttl]  - Set key-value pair\n"
           "  GET <key>               - Get value\n"
           "  DEL <key>               - Delete key\n"
           "  EXISTS <key>            - Check if key exists\n"
           "  KEYS                    - List all keys\n"
           "  STATS                   - Show statistics\n"
           "  SAVE <filename>         - Save to file\n"
           "  LOAD <filename>         - Load from file\n"
           "  CLEAR                   - Clear all data\n"
           "  FLUSH                   - Flush to disk\n"
           "  HELP                    - Show this help\n"
           "  QUIT                    - Disconnect";
}

string CommandHandler::handleFlush(istringstream& iss) {
    string filename;
    if (!(iss >> filename)) {
        return "ERROR: FLUSH requires a filename";
    }
    
    if (store_.flush(filename)) {
        return "OK";
    }
    return "ERROR: Failed to flush to file";
}

string CommandHandler::handleQuit(istringstream& iss) {
    return "BYE";
}

string CommandHandler::handleStats(istringstream& iss) {
    auto stats = store_.getStats();
    stringstream ss;
    ss << "Total operations: " << stats.totalOperations << "\n"
       << "Active threads: " << stats.activeThreads << "\n"
       << "Total keys: " << stats.totalKeys << "\n"
       << "Memory usage: " << stats.memoryUsage << " bytes";
    return ss.str();
} 