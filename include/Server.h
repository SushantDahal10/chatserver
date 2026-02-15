#pragma once
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include "TCPSocket.h"
#include "ChatRoom.h"
#include "ThreadSafeQueue.h"

class Server {
private:
    std::unique_ptr<TCPSocket> listenSocket_;
    std::shared_ptr<ChatRoom> room_;
    ThreadSafeQueue<Message> messageQueue_;
    std::vector<std::shared_ptr<ClientSession>> clients_;
    std::atomic<bool> running_;
    std::thread workerThread_;

    void processMessages();

public:
    Server();
    bool start(int port);
    void stop();
};
