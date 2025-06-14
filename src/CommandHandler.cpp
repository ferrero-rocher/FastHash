#include "CommandHandler.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iomanip>

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
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "SET") {
        std::string key, value;
        if (iss >> key >> value) {
            store_.set(key, value);
            return "OK";
        }
        return "ERROR Invalid SET command format";
    }
    else if (cmd == "GET") {
        std::string key;
        if (iss >> key) {
            auto value = store_.get(key);
            return value ? *value : "NOT_FOUND";
        }
        return "ERROR Invalid GET command format";
    }
    else if (cmd == "DEL") {
        std::string key;
        if (iss >> key) {
            return store_.del(key) ? "OK" : "NOT_FOUND";
        }
        return "ERROR Invalid DEL command format";
    }
    else if (cmd == "EXPIRE") {
        std::string key;
        int seconds;
        if (iss >> key >> seconds) {
            store_.expire(key, seconds);
            return "OK";
        }
        return "ERROR Invalid EXPIRE command format";
    }
    else if (cmd == "STATS") {
        auto stats = store_.getStats();
        std::ostringstream oss;
        oss << "{\n"
            << "  \"total_keys\": " << stats.totalKeys << ",\n"
            << "  \"uptime_seconds\": " << stats.uptime.count() << ",\n"
            << "  \"active_threads\": " << stats.activeThreads << "\n"
            << "}";
        return oss.str();
    }

    return "ERROR Unknown command";
} 