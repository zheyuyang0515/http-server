#include <iostream>
#include "src/server/Server.h"
#include "src/logger/Logger.h"
#include "src/util/Utility.h"
pthread_mutex_t Server::keep_alive_map_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Server::reverse_proxy_client_map_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Server::reverse_proxy_server_map_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Server::proxy_host_map_mutex = PTHREAD_MUTEX_INITIALIZER;
bool Server::reverse_proxy_mode;  //is reverse proxy opened?
int Server::proxy_server_num;    //totally number of servers which needs to be did proxy
//std::vector<std::string> Server::ips;        //host servers' ips array
//std::vector<int> Server::ports;              //host servers' ports array
std::vector<Utility::host_server_struct *> Server::host_server_list;
std::unordered_map<int, int> Server::reverse_proxy_client_map;  //recv-send map
std::unordered_map<int, int> Server::reverse_proxy_server_map;  //send-recv map
std::unordered_map<int, time_t> Server::keep_alive_map;    //save a keep-alive client and its start time
std::unordered_map<int, sockaddr_in> Server::proxy_host_map;
Utility::proxy_algorithm Server::proxyAlgorithm;
int Server::total_weight;
std::unordered_set<std::string> Server::proxy_suffix_set;
int main(int argc, char* argv[]) {
    //chdir("../");
    //running on the background and ignore output
    /*if(daemon(1, 0) < 0) {
        perror("Server running on the background failed");
        return -1;
    }*/
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
    if(is.reverse_proxy_mode) {
        //switch mode to proxy
        Server::reverse_proxy_mode = true;
        //init reverse proxy related data structure.
        Server::proxy_server_num = is.host_num;
        for(int i = 0; i < Server::proxy_server_num; ++i) {
            Server::host_server_list.push_back(is.host_server_list[i]);
        }
        Server::total_weight = is.total_weight;
        Server::proxyAlgorithm = is.proxyAlgorithm;
        //init proxy suffix
        for(auto i = 0; i < is.proxy_suffix_list.size(); ++i) {
            Server::proxy_suffix_set.insert(is.proxy_suffix_list[i]);
        }
    } else {
        Server::reverse_proxy_mode = false;
    }
    //start server
    server->Listen();

    return 0;
}
