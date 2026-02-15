#include "Server.h"
#include "ClientSession.h"
#include <WS2tcpip.h>

Server::Server() : running_(false) {}

bool Server::start(int port) {
    WSADATA data;
    if (WSAStartup(MAKEWORD(2,2), &data) != 0)
        return false;

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
        return false;

    listenSocket_ = std::make_unique<TCPSocket>(s);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    InetPton(AF_INET, "0.0.0.0", &addr.sin_addr);

    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
        return false;

    if (listen(s, SOMAXCONN) == SOCKET_ERROR)
        return false;

    room_ = std::make_shared<ChatRoom>();
    running_ = true;

    workerThread_ = std::thread(&Server::processMessages, this);

    while (running_) {
        SOCKET clientSocket = accept(s, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
            continue;

        auto socketObj = std::make_shared<TCPSocket>(clientSocket);
        auto session = std::make_shared<ClientSession>(
            socketObj, room_, messageQueue_);

        clients_.push_back(session);
        session->start();
    }

    return true;
}

void Server::processMessages() {
    while (running_) {
        Message msg = messageQueue_.wait_and_pop();
        room_->broadcast(msg);
    }
}

void Server::stop() {
    running_ = false;
    workerThread_.join();
    WSACleanup();
}
