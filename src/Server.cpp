#include "Server.h"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <functional>
#include <csignal>

#pragma comment(lib, "ws2_32.lib")

std::atomic<Server*> Server::instance_(nullptr);

Server::Server(int port) 
    : port_(port), threadPool_(4), store_(100), commandHandler_(store_), running_(false), serverSocket_(INVALID_SOCKET) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
    store_.setThreadPool(&threadPool_);
    instance_ = this;
}

Server::~Server() {
    if (running_) {
        stop();
    }
    instance_ = nullptr;
    WSACleanup();
}

void Server::handleSignal(int signal) {
    if (signal == SIGINT && instance_) {
        std::cout << "\nShutting down server..." << std::endl;
        instance_.load()->stop();
    }
}

void Server::stop() {
    if (!running_) return;
    
    running_ = false;
    Logger::getInstance().logServerStop();
    std::cout << "Server stopped" << std::endl;
    
    if (serverSocket_ != INVALID_SOCKET) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }
}

void Server::start() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == INVALID_SOCKET) {
        throw std::runtime_error("Error creating socket");
    }

    // Allow socket reuse
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        closesocket(serverSocket_);
        throw std::runtime_error("setsockopt failed");
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket_);
        throw std::runtime_error("Error binding socket to port " + std::to_string(port_));
    }

    if (listen(serverSocket_, 5) == SOCKET_ERROR) {
        closesocket(serverSocket_);
        throw std::runtime_error("Error listening on socket");
    }

    Logger::getInstance().logServerStart(port_);
    std::cout << "Server started on port " << port_ << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    running_ = true;

    // Set up signal handler
    signal(SIGINT, handleSignal);

    while (running_) {
        SOCKET clientSocket = accept(serverSocket_, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            if (running_) {
                std::cerr << "Error accepting connection: " << WSAGetLastError() << std::endl;
            }
            continue;
        }

        // Get client info for logging
        struct sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        getpeername(clientSocket, (struct sockaddr*)&clientAddr, &addrLen);
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::string clientInfo = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));
        
        Logger::getInstance().logClientConnect(clientInfo);
        std::cout << "New client connected: " << clientInfo << std::endl;

        threadPool_.enqueue([this, clientSocket]() {
            handleClient(clientSocket);
        });
    }
}

void Server::handleClient(SOCKET clientSocket) {
    try {
        while (running_) {
            std::string command = readLine(clientSocket);
            if (command.empty()) {
                break;
            }

            Logger::getInstance().logCommand(command);
            std::string response = commandHandler_.handle(command);
            if (!sendResponse(clientSocket, response)) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
    }
    closesocket(clientSocket);
}

std::string Server::readLine(SOCKET clientSocket) {
    std::string line;
    char buffer[1024];
    int bytesRead;

    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
        line.append(buffer);

        // Check if we have a complete line
        size_t pos = line.find('\n');
        if (pos != std::string::npos) {
            std::string result = line.substr(0, pos);
            line.erase(0, pos + 1);
            return result;
        }
    }

    return line;
}

bool Server::sendResponse(SOCKET clientSocket, const std::string& response) {
    std::string fullResponse = response + "\n";
    int totalSent = 0;
    int remaining = fullResponse.length();

    while (totalSent < fullResponse.length()) {
        int sent = send(clientSocket, fullResponse.c_str() + totalSent, remaining, 0);
        if (sent == SOCKET_ERROR) {
            return false;
        }
        totalSent += sent;
        remaining -= sent;
    }

    return true;
} 