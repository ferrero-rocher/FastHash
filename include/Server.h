#pragma once
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ThreadPool.h"
#include "KeyValueStore.h"
#include "CommandHandler.h"
#include "Logger.h"
#include <atomic>

class Server {
public:
    explicit Server(int port);
    ~Server();
    void start();
    void stop();

private:
    void handleClient(SOCKET clientSocket);
    bool sendResponse(SOCKET clientSocket, const std::string& response);
    std::string readLine(SOCKET clientSocket);
    static void handleSignal(int signal);

    int port_;
    ThreadPool threadPool_;
    KeyValueStore store_;
    CommandHandler commandHandler_;
    std::atomic<bool> running_;
    SOCKET serverSocket_;
    static std::atomic<Server*> instance_;
}; 