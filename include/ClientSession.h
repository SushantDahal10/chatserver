#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include "ISocket.h"
#include "ThreadSafeQueue.h"
#include "Message.h"

class ChatRoom;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
private:
    std::shared_ptr<ISocket> socket_;
    std::shared_ptr<ChatRoom> room_;
    ThreadSafeQueue<Message>& queue_;
    std::thread thread_;
    std::atomic<bool> running_;

    void run();

public:
    ClientSession(std::shared_ptr<ISocket> socket,
                  std::shared_ptr<ChatRoom> room,
                  ThreadSafeQueue<Message>& queue);

    void start();
    void deliver(const std::string& msg);
    void stop();
};
