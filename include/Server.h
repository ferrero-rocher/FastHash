#pragma once
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ThreadPool.h"
#include "KeyValueStore.h"
#include "CommandHandler.h"

class Server {
public:
    explicit Server(int port);
    ~Server();
    void start();

private:
    void handleClient(SOCKET clientSocket);
    bool sendResponse(SOCKET clientSocket, const std::string& response);
    std::string readLine(SOCKET clientSocket);

    int port_;
    ThreadPool threadPool_;
    KeyValueStore store_;
    CommandHandler commandHandler_;
    bool running_;
}; 