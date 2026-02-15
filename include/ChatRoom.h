#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include "Message.h"

class ClientSession;

class ChatRoom {
private:
    std::vector<std::weak_ptr<ClientSession>> clients_;
    std::mutex mutex_;

public:
    void join(std::shared_ptr<ClientSession> client);
    void leave(std::shared_ptr<ClientSession> client);
    void broadcast(const Message& msg);
};
