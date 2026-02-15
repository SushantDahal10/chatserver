#include "ChatRoom.h"
#include "ClientSession.h"
#include <algorithm>

void ChatRoom::join(std::shared_ptr<ClientSession> client) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.push_back(client);
}

void ChatRoom::leave(std::shared_ptr<ClientSession> client) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.erase(
        std::remove_if(clients_.begin(), clients_.end(),
            [client](std::weak_ptr<ClientSession>& wp) {
                auto sp = wp.lock();
                return !sp || sp == client;
            }),
        clients_.end()
    );
}

void ChatRoom::broadcast(const Message& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = clients_.begin(); it != clients_.end();) {
        if (auto client = it->lock()) {
            if (client != msg.sender) {
                client->deliver(msg.text);
            }
            ++it;
        } else {
            it = clients_.erase(it);
        }
    }
}
