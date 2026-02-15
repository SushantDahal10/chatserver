#include "TCPSocket.h"

TCPSocket::TCPSocket(SOCKET s) : socket_(s) {}

TCPSocket::~TCPSocket() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
    }
}

int TCPSocket::send(const std::string& msg) {
    return ::send(socket_, msg.c_str(), msg.length(), 0);
}

int TCPSocket::receive(char* buffer, int size) {
    return ::recv(socket_, buffer, size, 0);
}

void TCPSocket::close() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

SOCKET TCPSocket::getRawSocket() {
    return socket_;
}
