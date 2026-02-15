#pragma once
#include <string>
#include <memory>

class ClientSession;

struct Message {
    std::string text;
    std::shared_ptr<ClientSession> sender;
};
