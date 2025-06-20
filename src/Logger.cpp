#include "Logger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;

// Initialize static members
Logger* Logger::instance = nullptr;
mutex Logger::mutex_;

Logger& Logger::getInstance() {
    if (instance == nullptr) {
        lock_guard<mutex> lock(mutex_);
        if (instance == nullptr) {
            instance = new Logger();
        }
    }
    return *instance;
}

void Logger::info(const string& message) {
    try {
        lock_guard<mutex> lock(mutex_);
        
        // Get current time using chrono
        auto now = chrono::system_clock::now();
        auto now_time_t = chrono::system_clock::to_time_t(now);
        
        // Format time safely
        char time_str[32];
        #ifdef _WIN32
            struct tm timeinfo;
            localtime_s(&timeinfo, &now_time_t);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
        #else
            struct tm* timeinfo = localtime(&now_time_t);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
        #endif
        
        // Build log message
        stringstream ss;
        ss << time_str << " [INFO] " << message;
        string log_message = ss.str();
        
        // Always output to console first
        cout << log_message << endl;
        cout.flush();
        
        // Try to write to file if open
        if (logFile_.is_open()) {
            logFile_ << log_message << endl;
            logFile_.flush();
        }
    } catch (const exception& e) {
        cerr << "Logger error in info(): " << e.what() << endl;
    } catch (...) {
        cerr << "Unknown error in Logger::info()" << endl;
    }
}

void Logger::warning(const string& message) {
    try {
        lock_guard<mutex> lock(mutex_);
        
        // Get current time using chrono
        auto now = chrono::system_clock::now();
        auto now_time_t = chrono::system_clock::to_time_t(now);
        
        // Format time safely
        char time_str[32];
        #ifdef _WIN32
            struct tm timeinfo;
            localtime_s(&timeinfo, &now_time_t);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
        #else
            struct tm* timeinfo = localtime(&now_time_t);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
        #endif
        
        // Build log message
        stringstream ss;
        ss << time_str << " [WARNING] " << message;
        string log_message = ss.str();
        
        // Always output to console first
        cout << log_message << endl;
        cout.flush();
        
        // Try to write to file if open
        if (logFile_.is_open()) {
            logFile_ << log_message << endl;
            logFile_.flush();
        }
    } catch (const exception& e) {
        cerr << "Logger error in warning(): " << e.what() << endl;
    } catch (...) {
        cerr << "Unknown error in Logger::warning()" << endl;
    }
}

void Logger::error(const string& message) {
    try {
        lock_guard<mutex> lock(mutex_);
        
        // Get current time using chrono
        auto now = chrono::system_clock::now();
        auto now_time_t = chrono::system_clock::to_time_t(now);
        
        // Format time safely
        char time_str[32];
        #ifdef _WIN32
            struct tm timeinfo;
            localtime_s(&timeinfo, &now_time_t);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
        #else
            struct tm* timeinfo = localtime(&now_time_t);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
        #endif
        
        // Build log message
        stringstream ss;
        ss << time_str << " [ERROR] " << message;
        string log_message = ss.str();
        
        // Always output to console first
        cerr << log_message << endl;
        cerr.flush();
        
        // Try to write to file if open
        if (logFile_.is_open()) {
            logFile_ << log_message << endl;
            logFile_.flush();
        }
    } catch (const exception& e) {
        cerr << "Logger error in error(): " << e.what() << endl;
    } catch (...) {
        cerr << "Unknown error in Logger::error()" << endl;
    }
}

void Logger::setLogFile(const string& filename) {
    try {
        lock_guard<mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
        logFile_.open(filename, ios::app);
        if (!logFile_.is_open()) {
            cerr << "Failed to open log file: " << filename << endl;
            cerr.flush();
        }
    } catch (const exception& e) {
        cerr << "Logger error in setLogFile(): " << e.what() << endl;
        cerr.flush();
    }
}

Logger::~Logger() {
    try {
        lock_guard<mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
    } catch (const exception& e) {
        cerr << "Logger error in destructor: " << e.what() << endl;
        cerr.flush();
    }
} 