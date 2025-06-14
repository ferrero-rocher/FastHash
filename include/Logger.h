#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        
        logFile_ << "[" << ss.str() << "] " << message << std::endl;
        logFile_.flush();
    }

    void logClientConnect(const std::string& clientInfo) {
        log("Client connected: " + clientInfo);
    }

    void logCommand(const std::string& command) {
        log("Command processed: " + command);
    }

    void logKeyExpired(const std::string& key) {
        log("Key expired: " + key);
    }

    void logServerStart(int port) {
        log("Server started on port " + std::to_string(port));
    }

    void logServerStop() {
        log("Server shutting down");
    }

private:
    Logger() {
        // Create logs directory if it doesn't exist
        std::filesystem::create_directories("logs");
        
        // Create log file with timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
        std::string logPath = "logs/kvstore_" + ss.str() + ".log";
        
        logFile_.open(logPath, std::ios::app);
        if (!logFile_.is_open()) {
            throw std::runtime_error("Failed to open log file: " + logPath);
        }
        log("Log file created: " + logPath);
    }

    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile_;
    std::mutex mutex_;
}; 