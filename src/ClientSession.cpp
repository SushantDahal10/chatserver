#include "ClientSession.h"
#include "ChatRoom.h"

ClientSession::ClientSession(std::shared_ptr<ISocket> socket,
                             std::shared_ptr<ChatRoom> room,
                             ThreadSafeQueue<Message>& queue)
    : socket_(socket), room_(room), queue_(queue), running_(true) {}

void ClientSession::start() {
    room_->join(shared_from_this());
    thread_ = std::thread(&ClientSession::run, this);
    thread_.detach();
}

void ClientSession::run() {
    char buffer[4096];

    while (running_) {
        int bytes = socket_->receive(buffer, sizeof(buffer));
        if (bytes <= 0) {
            break;
        }

        std::string text(buffer, bytes);
        Message msg{ text, shared_from_this() };
        queue_.push(msg);
    }

    room_->leave(shared_from_this());
    socket_->close();
}

void ClientSession::deliver(const std::string& msg) {
    socket_->send(msg);
}

void ClientSession::stop() {
    running_ = false;
}
