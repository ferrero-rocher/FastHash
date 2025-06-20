#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>
#include <algorithm>
#include "Logger.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")

class Client {
private:
    SOCKET clientSocket;
    atomic<bool> running;
    thread inputThread;
    thread receiveThread;
    Logger& logger_;

    void printResponse(const string& response) {
        cout << response << endl;
    }

    void printTableRow(const string& key, const string& value) {
        cout << "| " << key << string(15 - key.length(), ' ') << " | " << value << string(30 - value.length(), ' ') << " |" << endl;
    }

    void printTableHeader() {
        cout << "+-----------------+--------------------------------+" << endl;
        cout << "| Key            | Value                          |" << endl;
        cout << "+-----------------+--------------------------------+" << endl;
    }

    void printTableFooter() {
        cout << "+-----------------+--------------------------------+" << endl;
    }

public:
    Client() : clientSocket(INVALID_SOCKET), running(false), logger_(Logger::getInstance()) {
        logger_.setLogFile("client.log");
    }

    bool connect(const string& host, int port) {
        try {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                logger_.error("WSAStartup failed with error: " + to_string(WSAGetLastError()));
                return false;
            }

            clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (clientSocket == INVALID_SOCKET) {
                logger_.error("Error creating socket: " + to_string(WSAGetLastError()));
                WSACleanup();
                return false;
            }

            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(port);
            
            // Try to resolve hostname if it's not an IP address
            if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
                struct addrinfo hints, *result = nullptr;
                ZeroMemory(&hints, sizeof(hints));
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;
                
                if (getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0) {
                    logger_.error("Failed to resolve hostname: " + host);
                    closesocket(clientSocket);
                    WSACleanup();
                    return false;
                }
                
                serverAddr.sin_addr = ((struct sockaddr_in*)result->ai_addr)->sin_addr;
                freeaddrinfo(result);
            }

            if (::connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                logger_.error("Connection failed with error: " + to_string(WSAGetLastError()));
                closesocket(clientSocket);
                WSACleanup();
                return false;
            }

            printResponse("Connected to server successfully!");
            return true;
        } catch (const exception& e) {
            logger_.error("Exception in connect: " + string(e.what()));
            if (clientSocket != INVALID_SOCKET) {
                closesocket(clientSocket);
                clientSocket = INVALID_SOCKET;
            }
            WSACleanup();
            return false;
        }
    }

    void start() {
        running = true;
        inputThread = thread(&Client::handleInput, this);
        receiveThread = thread(&Client::handleReceive, this);

        // Wait for threads to complete
        if (inputThread.joinable()) {
            inputThread.join();
        }
        if (receiveThread.joinable()) {
            receiveThread.join();
        }

        stop();
    }

    void stop() {
        // Set running to false first to prevent any new operations
        running = false;
        
        // Close the socket first to break any blocking operations
        if (clientSocket != INVALID_SOCKET) {
            try {
                shutdown(clientSocket, SD_BOTH);
                closesocket(clientSocket);
            } catch (...) {
                // Ignore any errors during shutdown
            }
            clientSocket = INVALID_SOCKET;
        }

        // Then wait for threads to complete
        try {
            if (inputThread.joinable()) {
                inputThread.join();
            }
            if (receiveThread.joinable()) {
                receiveThread.join();
            }
        } catch (...) {
            // Ignore any errors during thread joining
        }

        WSACleanup();
    }

private:
    void handleInput() {
        string input;
        while (running) {
            getline(cin, input);
            
            if (input.empty()) continue;
            
            if (input == "QUIT") {
                cout << "Disconnecting from server..." << endl;
                stop();
                break;
            }
            
            // Add newline if not present
            if (input.back() != '\n') {
                input += '\n';
            }
            
            if (send(clientSocket, input.c_str(), input.length(), 0) == SOCKET_ERROR) {
                cout << "Error: Failed to send command" << endl;
                stop();
                break;
            }
        }
    }

    void handleReceive() {
        char buffer[4096];
        while (running) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                if (bytesReceived == 0 && running) {
                    cout << "Server closed connection" << endl;
                } else if (running) {
                    logger_.error("recv failed with error: " + to_string(WSAGetLastError()));
                }
                stop();
                break;
            }

            buffer[bytesReceived] = '\0';
            string response(buffer);
            
            if (!response.empty() && running) {
                cout << response << endl;
                if (response == "BYE") {
                    stop();
                    break;
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <host> <port>" << endl;
        return 1;
    }
    
    Client client;
    
    if (!client.connect(argv[1], atoi(argv[2]))) {
        cout << "Error: Failed to connect to server" << endl;
        return 1;
    }
    
    client.start();
    return 0;
} 