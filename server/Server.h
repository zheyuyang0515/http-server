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
#include <string.h>
#include "../threadpool/Task.h"
#include <unordered_map>

class Server {
private:
    enum request_method {GET, POST};
    int socket_fd;      //server fd
    int epfd;           //epoll fd
    Logger *logger;     //logger
    int max_request;    //maximal number of requests the server could handle in one epoll_wait loop
    ThreadPool *pool;   //thread pool
public:
    /**
     * Constructor
     * @param ip: ip address which the server needs to listen to
     * @param port: port number which the server needs to listen to
     * @param max_request: maximal number of requests the server could handle in one epoll_wait loop
     */
    Server(std::string ip, int port, int max_request, ThreadPool *pool);
    ~Server() {};
public:
    /**
     * Start listening at the server socket to wait events using epoll_wait
     */
    void Listen();
    //client information for epoll's data attribute
    struct client_struct {
        struct sockaddr_in clientaddr;
        int client_fd;
    };
    //arg used to pass to the Task obj
    struct task_struct {
        struct client_struct *cs;
        Logger *logger;
        int epfd;
    };
    //read n characters from the specific client socket
    static char* readn(int n, struct client_struct *cs, Logger *logger, int epfd);
    /**
     * handle request
     * @param ts: struct task_struct
     */
    static void handle_request(void *ts);
private:
    struct http_header_get_struct {
        std::unordered_map<std::string, std::string> header_map;
        std::unordered_map<std::string, std::string> arg_map;
    };
     /**
      * @def parse http header information, and store the information in two hashmaps
      * @param header_struct: store the header result
      * @param cs: client structure to store client's information
     * @param logger: logger instance
     * @param epfd: Epoll fd
      * @return : return 1 for success, -1 for error
      */
    int parse_http_header(struct http_header_get_struct &header_struct, struct client_struct *cs, Logger *logger, int epfd);
};




#endif //HTTP_SERVER_SERVER_H
