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
