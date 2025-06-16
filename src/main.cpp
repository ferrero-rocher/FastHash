#include "Server.h"
#include "Logger.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

using namespace std;

atomic<bool> running(true);

void signalHandler(int signum) {
    cout << "Interrupt signal (" << signum << ") received.\n";
    running = false;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    Logger& logger = Logger::getInstance();
    Server server(logger);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if (!server.start(port)) {
        logger.error("Failed to start server");
        return 1;
    }

    logger.info("Server started on port " + std::to_string(port));

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    logger.info("Server stopped");
    return 0;
} 