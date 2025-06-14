#include "Server.h"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <functional>

#pragma comment(lib, "ws2_32.lib")

Server::Server(int port) 
    : port_(port), threadPool_(4), store_(100), commandHandler_(store_), running_(false) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}

Server::~Server() {
    running_ = false;
    WSACleanup();
}

void Server::start() {
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        throw std::runtime_error("Error creating socket");
    }

    // Allow socket reuse
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        closesocket(serverSocket);
        throw std::runtime_error("setsockopt failed");
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        throw std::runtime_error("Error binding socket to port " + std::to_string(port_));
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        closesocket(serverSocket);
        throw std::runtime_error("Error listening on socket");
    }

    std::cout << "Server started on port " << port_ << std::endl;
    running_ = true;

    while (running_) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            if (running_) {
                std::cerr << "Error accepting connection: " << WSAGetLastError() << std::endl;
            }
            continue;
        }

        threadPool_.enqueue([this, clientSocket]() {
            handleClient(clientSocket);
        });
    }

    closesocket(serverSocket);
}

void Server::handleClient(SOCKET clientSocket) {
    try {
        while (running_) {
            std::string command = readLine(clientSocket);
            if (command.empty()) {
                break;
            }

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