#include <iostream>
#include "src/server/Server.h"
#include "src/logger/Logger.h"
#include "src/util/Utility.h"

int main(int argc, char* argv[]) {
    int ret = -1;
    struct Utility::init_struct is;
    //read config file and set corresponding attributes in the memory
    ret = Utility::parse_conf_file(&is);
    if(ret == -1) {
        exit(-1);
    }
    //init logger singleton(indicate log file directory and ignorance of severity)
    Logger *logger = Logger::get_instance(LOG_DIR, is.severity_ignore);
    //init threadpool
    ThreadPool *pool = new ThreadPool(is.min_worker, is.max_worker, is.add_step, is.adjust_time, is.add_worker_rate, is.delete_worker_rate);
    //init server
    struct Server::page_struct ps;
    ps.error_404_page = is.error_404_page;
    ps.main_page = is.main_page;
    Server *server = new Server(is.ip, is.port, is.max_request, pool, ps, is.max_connection);
    //start server
    server->Listen();

    return 0;
}
