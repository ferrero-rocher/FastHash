#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>

using namespace std;

class Logger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    static Logger& getInstance(const string& filename = "kvstore.log");
    void debug(const string& message);
    void info(const string& message);
    void warning(const string& message);
    void error(const string& message);

private:
    Logger(const string& filename);
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger* instance_;
    static mutex mutex_;
    ofstream logFile_;
    Level minLevel_ = Level::INFO;

    void log(Level level, const string& message);
    string getTimestamp();
    string levelToString(Level level);
}; 