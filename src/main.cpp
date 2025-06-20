#include "Server.h"
#include "Logger.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

using namespace std;

atomic<bool> running(true);

void signalHandler(int signum) {
    cout << "\nShutting down server..." << endl;
    running = false;
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            cerr << "Usage: " << argv[0] << " <port>" << endl;
            return 1;
        }

        int port = stoi(argv[1]);
        if (port <= 0 || port > 65535) {
            cerr << "Invalid port number. Must be between 1 and 65535." << endl;
            return 1;
        }

        // Initialize logger first
        Logger& logger = Logger::getInstance();
        logger.setLogFile("server.log");  // Use a different log file for server
        
        logger.info("Server starting up...");
        logger.info("Port: " + to_string(port));
        
        // Initialize server
        Server server(logger);

        // Set up signal handlers
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        logger.info("Signal handlers configured");

        // Start server
        if (!server.start(port)) {
            logger.error("Failed to start server");
            cerr << "Failed to start server" << endl;
            return 1;
        }

        logger.info("Server started successfully on port " + to_string(port));
        cout << "Server started on port " << port << endl;
        cout << "Press Ctrl+C to stop" << endl;

        // Main loop
        while (running) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }

        // Cleanup
        logger.info("Shutdown signal received, stopping server...");
        server.stop();
        logger.info("Server stopped successfully");
        return 0;
    } catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "Unknown fatal error" << endl;
        return 1;
    }
} 