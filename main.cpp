#include <iostream>
#include "server/Server.h"
#define IPADDR "127.0.0.1"
#define PORT 8686
int main() {
    ThreadPool *pool = new ThreadPool(2, 4, 1);
    Server *server = new Server(IPADDR, PORT, 1024, pool);
    server->Listen();
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
