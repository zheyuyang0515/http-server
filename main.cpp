#include <iostream>
#include "server/Server.h"
#define IPADDR "127.0.0.1"
#define PORT 8686
int main() {
    Server *server = new Server(IPADDR, PORT);
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
