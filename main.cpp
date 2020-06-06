#include <iostream>
#include "server/Server.h"
#include "logger/Logger.h"
#include "util/Utility.h"
//#define IPADDR "192.168.1.111"
#define IPADDR "127.0.0.1"
#define PORT 8686
int main(int argc, char* argv[]) {
    //init logger singleton(indicate log file directory and ignorance of severity)
    Logger *logger = Logger::get_instance(LOG_DIR, INFO);
    //init threadpool
    ThreadPool *pool = new ThreadPool(4, 4, 1);
    //init server
    Server *server = new Server(IPADDR, PORT, 1024, pool);
    //start server
    server->Listen();

    return 0;
}
