#pragma once
#include <string>
#include "KeyValueStore.h"

// Supported commands:
// SET key value
// GET key
// DEL key
// EXPIRE key seconds
// Returns: "OK", value, "NOT_FOUND", "ERROR <msg>"
class CommandHandler {
public:
    explicit CommandHandler(KeyValueStore& store);
    std::string handle(const std::string& command);
private:
    KeyValueStore& store_;
}; 