#include "../include/Server.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <thread>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

Server::Server(Logger& logger) : logger_(logger), commandHandler_(store_, logger_) {
    serverSocket_ = INVALID_SOCKET;
    running_ = false;
}

Server::~Server() {
    stop();
    WSACleanup();
}

bool Server::start(int port) {
    try {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            logger_.error("WSAStartup failed with error: " + to_string(WSAGetLastError()));
            return false;
        }

        serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket_ == INVALID_SOCKET) {
            logger_.error("Error creating socket: " + to_string(WSAGetLastError()));
            WSACleanup();
            return false;
        }

        // Allow address reuse
        int opt = 1;
        if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
            logger_.error("setsockopt failed with error: " + to_string(WSAGetLastError()));
            closesocket(serverSocket_);
            WSACleanup();
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            logger_.error("Bind failed with error: " + to_string(WSAGetLastError()));
            closesocket(serverSocket_);
            WSACleanup();
            return false;
        }

        if (listen(serverSocket_, SOMAXCONN) == SOCKET_ERROR) {
            logger_.error("Listen failed with error: " + to_string(WSAGetLastError()));
            closesocket(serverSocket_);
            WSACleanup();
            return false;
        }

        running_ = true;
        serverThread_ = thread(&Server::serverLoop, this, port);
        return true;
    } catch (const exception& e) {
        logger_.error("Exception in start(): " + string(e.what()));
        if (serverSocket_ != INVALID_SOCKET) {
            closesocket(serverSocket_);
            serverSocket_ = INVALID_SOCKET;
        }
        WSACleanup();
        return false;
    }
}

void Server::stop() {
    logger_.info("Server stop requested");
    running_ = false;
    
    if (serverSocket_ != INVALID_SOCKET) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
        logger_.info("Server socket closed");
    }
    
    if (serverThread_.joinable()) {
        serverThread_.join();
        logger_.info("Server thread joined");
    }
    logger_.info("Server stopped successfully");
}

void Server::serverLoop(int port) {
    logger_.info("Server loop started, listening for connections on port " + to_string(port));
    while (running_) {
        try {
            SOCKET clientSocket = accept(serverSocket_, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                if (running_) {
                    logger_.error("Accept failed with error: " + to_string(WSAGetLastError()));
                }
                continue;
            }

            logger_.info("New client connection accepted");
            threadPool_.submit([this, clientSocket]() {
                handleClient(clientSocket);
            });
        } catch (const exception& e) {
            logger_.error("Exception in serverLoop: " + string(e.what()));
        } catch (...) {
            logger_.error("Unknown exception in serverLoop");
        }
    }
    logger_.info("Server loop stopped");
}

void Server::handleClient(SOCKET clientSocket) {
    try {
        logger_.info("Handling client connection");
        
        // Send welcome message and menu
        string welcome = "Welcome to Key-Value Store Server!\n"
                        "Commands:\n"
                        "  SET <key> <value> [ttl]  - Set key-value pair\n"
                        "  GET <key>               - Get value\n"
                        "  DEL <key>               - Delete key\n"
                        "  EXISTS <key>            - Check if key exists\n"
                        "  KEYS                    - List all keys\n"
                        "  STATS                   - Show statistics\n"
                        "  SAVE <filename>         - Save to file\n"
                        "  LOAD <filename>         - Load from file\n"
                        "  CLEAR                   - Clear all data\n"
                        "  FLUSH                   - Flush to disk\n"
                        "  HELP                    - Show this help\n"
                        "  QUIT                    - Disconnect\n\n";
        
        if (send(clientSocket, welcome.c_str(), welcome.length(), 0) == SOCKET_ERROR) {
            logger_.error("Failed to send welcome message: " + to_string(WSAGetLastError()));
            closesocket(clientSocket);
            return;
        }

        char buffer[1024];
        int bytesReceived;
        string commandBuffer;

        while (running_) {
            bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                if (bytesReceived == 0) {
                    logger_.info("Client disconnected gracefully");
                } else {
                    logger_.error("recv failed with error: " + to_string(WSAGetLastError()));
                }
                break;
            }

            buffer[bytesReceived] = '\0';
            commandBuffer += buffer;

            // Process complete commands (those ending with newline)
            size_t pos;
            while ((pos = commandBuffer.find('\n')) != string::npos) {
                string command = commandBuffer.substr(0, pos);
                commandBuffer = commandBuffer.substr(pos + 1);

                // Trim whitespace
                command.erase(0, command.find_first_not_of(" \t\r\n"));
                command.erase(command.find_last_not_of(" \t\r\n") + 1);

                if (!command.empty()) {
                    logger_.info("[REQUEST] " + command);
                    
                    string response;
                    try {
                        response = commandHandler_.handleCommand(command);
                    } catch (const exception& e) {
                        logger_.error("Exception in handleCommand: " + string(e.what()));
                        response = "ERROR: Internal server error\n";
                    } catch (...) {
                        logger_.error("Unknown exception in handleCommand");
                        response = "ERROR: Internal server error\n";
                    }
                    
                    // Add newline to response if not present
                    if (!response.empty() && response.back() != '\n') {
                        response += '\n';
                    }

                    if (send(clientSocket, response.c_str(), response.length(), 0) == SOCKET_ERROR) {
                        logger_.error("Send failed with error: " + to_string(WSAGetLastError()));
                        closesocket(clientSocket);
                        return;
                    }

                    // If command was QUIT, close the connection after sending BYE
                    if (command == "QUIT") {
                        logger_.info("Client requested disconnect");
                        closesocket(clientSocket);
                        return;
                    }
                }
            }
        }
    } catch (const exception& e) {
        logger_.error("Exception in handleClient: " + string(e.what()));
    } catch (...) {
        logger_.error("Unknown exception in handleClient");
    }
    
    closesocket(clientSocket);
} 