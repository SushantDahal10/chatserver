#pragma once
#include "ISocket.h"
#include <Winsock2.h>

class TCPSocket : public ISocket {
private:
    SOCKET socket_;

public:
    TCPSocket(SOCKET s);
    ~TCPSocket();

    int send(const std::string& msg) override;
    int receive(char* buffer, int size) override;
    void close() override;

    SOCKET getRawSocket();
};
