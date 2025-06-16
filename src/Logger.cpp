#include "Logger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;

// Define static members
Logger* Logger::instance = nullptr;
mutex Logger::mutex;

Logger& Logger::getInstance() {
    lock_guard<mutex> lock(mutex);
    if (instance == nullptr) {
        instance = new Logger();
    }
    return *instance;
}

void Logger::info(const string& message) {
    lock_guard<mutex> lock(mutex);
    auto now = chrono::system_clock::now();
    auto time = chrono::system_clock::to_time_t(now);
    
    stringstream ss;
    ss << put_time(localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << " [INFO] " << message;
    
    cout << ss.str() << endl;
    
    if (logFile_.is_open()) {
        logFile_ << ss.str() << endl;
        logFile_.flush();
    }
}

void Logger::error(const string& message) {
    lock_guard<mutex> lock(mutex);
    auto now = chrono::system_clock::now();
    auto time = chrono::system_clock::to_time_t(now);
    
    stringstream ss;
    ss << put_time(localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << " [ERROR] " << message;
    
    cerr << ss.str() << endl;
    
    if (logFile_.is_open()) {
        logFile_ << ss.str() << endl;
        logFile_.flush();
    }
}

void Logger::setLogFile(const string& filename) {
    lock_guard<mutex> lock(mutex);
    if (logFile_.is_open()) {
        logFile_.close();
    }
    logFile_.open(filename, ios::app);
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
} 