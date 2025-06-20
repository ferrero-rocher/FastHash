#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>

using namespace std;

class Logger {
private:
    static Logger* instance;
    static std::mutex mutex_;
    ofstream logFile_;

    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

public:
    static Logger& getInstance();
    void info(const string& message);
    void error(const string& message);
    void warning(const string& message);
    void setLogFile(const string& filename);
    ~Logger();
}; 