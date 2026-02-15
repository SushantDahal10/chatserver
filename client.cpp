#include <iostream>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <string>
#include <atomic>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

atomic<bool> running(true);

bool initialize() {
    WSADATA data;
    return WSAStartup(MAKEWORD(2,2), &data) == 0;
}

void receiveMessages(SOCKET sock) {
    char buffer[4096];

    while (running) {
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            cout << "Disconnected from server\n";
            running = false;
            break;
        }

        string message(buffer, bytes);
        cout << message << endl;
    }

    closesocket(sock);
    WSACleanup();
}

void sendMessages(SOCKET sock) {
    cout << "Enter your name: ";
    string name;
    getline(cin, name);

    while (running) {
        string text;
        getline(cin, text);

        if (text == "quit") {
            running = false;
            break;
        }

        string message = name + " : " + text;
        int result = send(sock, message.c_str(), message.length(), 0);

        if (result == SOCKET_ERROR) {
            cout << "Send failed\n";
            running = false;
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
}

int main() {

    if (!initialize()) {
        cout << "Winsock initialization failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET) {
        cout << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Connection failed\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server\n";

    thread receiver(receiveMessages, sock);
    thread sender(sendMessages, sock);

    sender.join();
    receiver.join();

    return 0;
}
