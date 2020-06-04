#include <iostream>
#include "server/Server.h"
#define IPADDR "192.168.1.111"
#define PORT 8686
int main() {
    Logger *logger = Logger::get_instance();
    ThreadPool *pool = new ThreadPool(2, 4, 1);
    Server *server = new Server(IPADDR, PORT, 1024, pool);
    server->Listen();
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
