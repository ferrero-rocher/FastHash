#include "CommandHandler.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

CommandHandler::CommandHandler(KeyValueStore& store) : store_(store) {}

static std::vector<std::string> tokenize(const std::string& str) {
    std::istringstream iss(str);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string CommandHandler::handle(const std::string& command) {
    auto tokens = tokenize(command);
    if (tokens.empty()) return "ERROR Empty command";
    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "SET") {
        if (tokens.size() < 3) return "ERROR Usage: SET key value";
        store_.set(tokens[1], tokens[2]);
        return "OK";
    } else if (cmd == "GET") {
        if (tokens.size() != 2) return "ERROR Usage: GET key";
        auto val = store_.get(tokens[1]);
        if (val) return *val;
        return "NOT_FOUND";
    } else if (cmd == "DEL") {
        if (tokens.size() != 2) return "ERROR Usage: DEL key";
        return store_.del(tokens[1]) ? "OK" : "NOT_FOUND";
    } else if (cmd == "EXPIRE") {
        if (tokens.size() != 3) return "ERROR Usage: EXPIRE key seconds";
        try {
            int seconds = std::stoi(tokens[2]);
            store_.expire(tokens[1], seconds);
            return "OK";
        } catch (...) {
            return "ERROR Invalid seconds";
        }
    } else {
        return "ERROR Unknown command";
    }
} 