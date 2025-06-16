#pragma once

#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "KeyValueStore.h"
#include "CommandHandler.h"
#include "Logger.h"

using namespace std;

class Server {
public:
    explicit Server(int port);
    ~Server();

    void start();
    void stop();

private:
    SOCKET serverSocket_;
    int port_;
    KeyValueStore store_;
    CommandHandler commandHandler_;
    Logger& logger_;
    bool running_;

    void handleClient(int clientSocket);
}; 