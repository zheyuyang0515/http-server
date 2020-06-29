//
// Created by zheyu on 6/1/20.
//

#ifndef HTTP_SERVER_SERVER_H
#define HTTP_SERVER_SERVER_H

#include <unistd.h>
#include <string>
#include <pthread.h>
#include <sys/socket.h>
#include "../logger/Logger.h"
#include "../threadpool/ThreadPool.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cstring>
#include "../threadpool/Task.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <sys/stat.h>
#include <vector>
class Server {
private:
    enum request_type {PIC, HTML};
    enum request_method {GET, POST};
    int socket_fd;      //server fd
    int epfd;           //epoll fd
    Logger *logger;     //logger
    int max_request;    //maximal number of requests the server could handle in one epoll_wait loop
    ThreadPool *pool;   //thread pool
    static const std::unordered_map<std::string, std::string> pic_type_map;    //save all pic suffix(bmp, jpeg, png, gif, jpg, ico)
    static const std::unordered_map<std::string, std::string> text_type_map;    //save all pic suffix(html, css)
    static int get_random_host();
public:
    /**
     * Start listening at the server socket to wait events using epoll_wait
     */
    void Listen();
    //client information for epoll's data attribute
    struct client_struct {
        struct sockaddr_in clientaddr;
        int client_fd;
        int mode;    //1 from client, 2 from host/original server
    };
    //arg used to pass to the Task obj
    struct task_struct {
        struct client_struct *cs;
        Logger *logger;
        int epfd;
    };

    static bool reverse_proxy_mode;  //is reverse proxy opened?
    static int proxy_server_num;    //totally number of servers which needs to be did proxy
    //static std::vector<std::string> ips;        //host servers' ips array
    //static std::vector<int> ports;              //host servers' ports array
    static std::vector<Utility::host_server_struct *> host_server_list;              //host servers' ports array
    static pthread_mutex_t reverse_proxy_client_map_mutex;
    static pthread_mutex_t reverse_proxy_server_map_mutex;
    static pthread_mutex_t keep_alive_map_mutex;
    static pthread_mutex_t proxy_host_map_mutex;
    static std::unordered_map<int, int> reverse_proxy_client_map;  //recv-send map
    static std::unordered_map<int, int> reverse_proxy_server_map;  //send-recv map
    static std::unordered_map<int, time_t> keep_alive_map;    //save a keep-alive client and its start time
    static std::unordered_map<int, sockaddr_in> proxy_host_map; //save the host server info for each client
    static int total_weight;
    static Utility::proxy_algorithm proxyAlgorithm;
    static int round_ptr;       //pointer used to do round robin algorithm proxy
    static int round_ctr;       //counter used to do round robin algorithm proxy
    //read n characters from the specific client socket
    //static int readn(char *buff, int n, struct client_struct *cs, Logger *logger, int epfd);
    static int readn(char *buff, int n, int client_fd);
    /**
     * handle request
     * @param ts: struct task_struct
     */
    static void handle_request(void *ts);
public:
    struct page_struct {
        std::string main_page;
        std::string error_404_page;
    };
    /**
     * Constructor
     * @param ip: ip address which the server needs to listen to
     * @param port: port number which the server needs to listen to
     * @param max_request: maximal number of requests the server could handle in one epoll_wait loop
     */
    Server(std::string ip, int port, int max_request, ThreadPool *pool, struct Server::page_struct ps, int max_connection);
    ~Server() {};
private:
    struct http_header_get_struct {
        std::unordered_map<std::string, std::string> header_map;    //header information(k-v)
        std::unordered_map<std::string, std::string> arg_map;       //argument in the url(k-v): this map is not being used currently
    };
    /**
     * @def: recv requests from client and dispatch to host/original server
     * @param header_map: http header info
     * @param send_fd: use to connect to the host/original server
     * @param cs: client info struct
     * @param logger: log
     * @return 1 on success, -1 on failed
     */
    static int dispatch_request(std::unordered_map<std::string, std::string> header_map, int send_fd, struct client_struct *cs, Logger *logger);
    struct dispatch_response_task_struct {
        struct client_struct *cs;
        Logger *logger;
    };
    static void dispatch_response(void *arg);
     /**
      * @def parse http header information, and store the information in two hashmaps
      * @param header_struct: store the header result
      * @param cs: client structure to store client's information
      * @param logger: logger instance
      * @param epfd: Epoll fd
      * @return : return 1 for success, -1 for error
      */
    static int parse_http_header(struct http_header_get_struct *header_struct, struct client_struct *cs, Logger *logger, int epfd);
    /**
     * @def create response message and send to the client
     * @param dir: resource root directory
     * @param req_url: request url
     * @param cs: client information
     * @param logger: logger instance
     * @param type: response type
     * @return
     */
    static int http_response(std::string dir, std::unordered_map<std::string, std::string> map, struct client_struct *cs, Logger *logger, request_type type);
    static char* get_cur_time() {
        std::string weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        std::string monthes[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"};
        time_t timet;
        char *cur_time = new char[40];
        struct tm time_struct;
        time(&timet);
        time_struct = *gmtime(&timet);
        std::string format = "%a, %d %b %Y %H:%M:%S GMT";
        strftime(cur_time, 40, format.c_str(), &time_struct);
        return cur_time;
    }
};




#endif //HTTP_SERVER_SERVER_H
