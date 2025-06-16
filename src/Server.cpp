#include "../include/Server.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <thread>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

Server::Server(Logger& logger) : logger_(logger), commandHandler_(store_, logger_) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        logger_.error("Failed to initialize Winsock");
        throw std::runtime_error("Failed to initialize Winsock");
    }
}

Server::~Server() {
    stop();
    WSACleanup();
}

bool Server::start(int port) {
    if (running_) {
        logger_.warning("Server is already running");
        return false;
    }

    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ == INVALID_SOCKET) {
        logger_.error("Failed to create socket");
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        logger_.error("Failed to bind socket");
        closesocket(serverSocket_);
        return false;
    }

    if (listen(serverSocket_, SOMAXCONN) == SOCKET_ERROR) {
        logger_.error("Failed to listen on socket");
        closesocket(serverSocket_);
        return false;
    }

    running_ = true;
    serverThread_ = std::thread(&Server::serverLoop, this, port);
    logger_.info("Server started on port " + std::to_string(port));
    std::cout << "Server started on port " << port << std::endl;
    return true;
}

void Server::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (serverSocket_ != INVALID_SOCKET) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    logger_.info("Server stopped");
}

void Server::serverLoop(int port) {
    while (running_) {
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket_, (sockaddr*)&clientAddr, &addrLen);

        if (clientSocket == INVALID_SOCKET) {
            if (running_) {
                logger_.error("Failed to accept connection");
            }
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        logger_.info("New connection from " + std::string(clientIP));

        std::thread clientThread(&Server::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void Server::handleClient(int clientSocket) {
    store_.incrementActiveThreads();
    char buffer[4096];
    std::string command;

    while (running_) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            break;
        }

        buffer[bytesReceived] = '\0';
        command += buffer;

        size_t pos;
        while ((pos = command.find('\n')) != std::string::npos) {
            std::string line = command.substr(0, pos);
            command.erase(0, pos + 1);

            if (!line.empty()) {
                std::cout << "Received command: " << line << std::endl;
                std::string response = commandHandler_.handleCommand(line);
                std::cout << "Response: " << response << std::endl;
                response += "\n";  // Add newline to response
                send(clientSocket, response.c_str(), response.length(), 0);
            }
        }
    }

    store_.decrementActiveThreads();
    closesocket(clientSocket);
} 