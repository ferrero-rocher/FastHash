#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    // Create socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket" << std::endl;
        WSACleanup();
        return 1;
    }

    // Connect to server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server! Type commands (or 'quit' to exit):" << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "  SET key value    - Set a key-value pair" << std::endl;
    std::cout << "  GET key          - Get value for a key" << std::endl;
    std::cout << "  DEL key          - Delete a key" << std::endl;
    std::cout << "  EXPIRE key secs  - Set expiration for a key" << std::endl;
    std::cout << "  quit             - Exit the program" << std::endl;

    std::string command;
    char buffer[1024];

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);

        if (command == "quit") {
            break;
        }

        // Send command to server
        command += "\n";
        if (send(clientSocket, command.c_str(), command.length(), 0) == SOCKET_ERROR) {
            std::cerr << "Error sending command" << std::endl;
            break;
        }

        // Receive response
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Server response: " << buffer;
        } else {
            std::cerr << "Error receiving response" << std::endl;
            break;
        }
    }

    // Cleanup
    closesocket(clientSocket);
    WSACleanup();
    return 0;
} 