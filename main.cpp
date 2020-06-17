#include <iostream>
#include "src/server/Server.h"
#include "src/logger/Logger.h"
#include "src/util/Utility.h"
#define IPADDR "192.168.1.111"
//#define IPADDR "127.0.0.1"
#define PORT 8686
int main(int argc, char* argv[]) {
    //init logger singleton(indicate log file directory and ignorance of severity)
    Logger *logger = Logger::get_instance(LOG_DIR, Log::ALL);
    //init threadpool
    ThreadPool *pool = new ThreadPool(1, 4, 2);
    //init server
    Server *server = new Server(IPADDR, PORT, 1024, pool);
    //start server
    server->Listen();

    return 0;
}
