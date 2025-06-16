#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

class Client {
private:
    SOCKET clientSocket;
    atomic<bool> running;
    thread inputThread;

public:
    Client() : clientSocket(INVALID_SOCKET), running(false) {}

    bool connect(const string& host, int port) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << "WSAStartup failed" << endl;
            return false;
        }

        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Error creating socket" << endl;
            WSACleanup();
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

        if (::connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cerr << "Connection failed" << endl;
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        cout << "Connected to server" << endl;
        return true;
    }

    void start() {
        running = true;
        inputThread = thread(&Client::handleInput, this);

        char buffer[4096];
        while (running) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                break;
            }

            buffer[bytesReceived] = '\0';
            cout << buffer;
        }

        stop();
    }

    void stop() {
        running = false;
        if (inputThread.joinable()) {
            inputThread.join();
        }
        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
        }
        WSACleanup();
    }

private:
    void handleInput() {
        string input;
        while (running) {
            getline(cin, input);
            if (!running) break;

            input += "\n";
            if (send(clientSocket, input.c_str(), input.length(), 0) == SOCKET_ERROR) {
                cerr << "Send failed" << endl;
                break;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <host> <port>" << endl;
        return 1;
    }

    string host = argv[1];
    int port = stoi(argv[2]);

    Client client;
    if (!client.connect(host, port)) {
        return 1;
    }

    client.start();
    return 0;
} 