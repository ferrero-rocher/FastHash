#pragma once

#include <string>
#include <sstream>
#include "KeyValueStore.h"
#include "Logger.h"

using namespace std;

// Supported commands:
// SET key value
// GET key
// DEL key
// EXPIRE key seconds
// STATS
// Returns: "OK", value, "NOT_FOUND", "ERROR <msg>", or stats in JSON format
class CommandHandler {
public:
    CommandHandler(KeyValueStore& store, Logger& logger) : store_(store), logger_(logger) {}
    
    string handleCommand(const string& command);

    // Public methods for testing
    string handleSet(std::istringstream& iss);
    string handleGet(std::istringstream& iss);
    string handleDel(std::istringstream& iss);
    string handleExists(std::istringstream& iss);
    string handleExpire(std::istringstream& iss);
    string handleTtl(std::istringstream& iss);
    string handleKeys(std::istringstream& iss);
    string handleClear(std::istringstream& iss);
    string handleSave(std::istringstream& iss);
    string handleLoad(std::istringstream& iss);
    string handleDump(std::istringstream& iss);
    string handleHelp(std::istringstream& iss);
    string handleFlush(std::istringstream& iss);
    string handleQuit(std::istringstream& iss);

private:
    KeyValueStore& store_;
    Logger& logger_;

    // Command handlers
    string handleStats(std::istringstream& iss);
}; 