#pragma once

#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include "KeyValueStore.h"
#include "CommandHandler.h"
#include "Logger.h"
#include "ThreadPool.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

class Server {
public:
    explicit Server(Logger& logger);
    ~Server();

    bool start(int port);
    void stop();

private:
    SOCKET serverSocket_;
    KeyValueStore store_;
    CommandHandler commandHandler_;
    Logger& logger_;
    std::atomic<bool> running_;
    std::thread serverThread_;
    ThreadPool threadPool_;

    void handleClient(SOCKET clientSocket);
    void serverLoop(int port);
}; 