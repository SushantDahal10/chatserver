#pragma once
#include <string>

class ISocket {
public:
    virtual int send(const std::string& msg) = 0;
    virtual int receive(char* buffer, int size) = 0;
    virtual void close() = 0;
    virtual ~ISocket() = default;
};
