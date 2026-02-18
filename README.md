chatroom.h

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

//clientsession.h
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

//Isocket.h
#pragma once
#include <string>

class ISocket {
public:
    virtual int send(const std::string& msg) = 0;
    virtual int receive(char* buffer, int size) = 0;
    virtual void close() = 0;
    virtual ~ISocket() = default;
};

//Message.h
#pragma once
#include <string>
#include <memory>

class ClientSession;

struct Message {
    std::string text;
    std::shared_ptr<ClientSession> sender;
};
//server.h
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

//TCPsocket.h
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

//threadsafequeue.h
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;

public:
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
        cv_.notify_one();
    }

    T wait_and_pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        T value = queue_.front();
        queue_.pop();
        return value;
    }
};

//chatroom.cpp
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
//clientsession.cpp
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
//main.cpp
#include "Server.h"
#include <iostream>

int main() {
    Server server;

    if (!server.start(12345)) {
        std::cout << "Server failed to start\n";
        return 1;
    }

    return 0;
}
//server.cpp
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
//tcpsocket.cpp
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

//client.cpp
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
 

I have been reading Human Edge in the AI Age by Nitin Seth, and one idea completely shifted how I think about growth in the AI era, what he calls the Important Flip. For years, the safe path seemed obvious, become more technical, more specialised, more data driven. But Seth introduces a powerful lens through what he describes as an architectural mirror, inspired by the Yoga Sutras. Buddhi, the macro layer, collective intelligence like large AI models synthesising knowledge at scale. Ahamkara, the enterprise layer, where organisational context, culture and identity shape outcomes. Manas, the individual sensing layer, reacting and customising in the moment. At first it sounds philosophical, but it becomes deeply practical when you apply it to real work.
Last year, our backend system kept crashing under heavy traffic. We operated at the Manas layer, using AI to generate snippets, fixing compiler errors, optimising small sections. We were improving parts, not solving the problem. Then we stepped back. We moved to the Buddhi level, gathered around a whiteboard, mapped the full system, questioned assumptions. The real issue was not syntax. It was a race condition created by conflicting priorities between two teams, speed versus data integrity. No debugger surfaced it. No prompt revealed it. It was a design conflict rooted in human decisions.
That experience changed how I define growth. Competing with AI on speed or data processing is a losing game. The edge lies in sensing what the data does not show, asking better questions, zooming out when everyone else is zooming in. In the age of AI, the real advantage is not becoming more machine like. It is becoming more human.
