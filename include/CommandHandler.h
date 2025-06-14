#pragma once
#include <string>
#include "KeyValueStore.h"

// Supported commands:
// SET key value
// GET key
// DEL key
// EXPIRE key seconds
// STATS
// Returns: "OK", value, "NOT_FOUND", "ERROR <msg>", or stats in JSON format
class CommandHandler {
public:
    explicit CommandHandler(KeyValueStore& store);
    std::string handle(const std::string& command);
private:
    KeyValueStore& store_;
}; 