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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
class Server {
private:
    int socket_fd;      //server fd
    int epfd;           //epoll fd
    Logger *logger;     //logger
    int max_request;    //maximal number of requests the server could handle in one epoll_wait loop
public:
    /**
     * Constructor
     * @param ip: ip address which the server needs to listen to
     * @param port: port number which the server needs to listen to
     * @param max_request: maximal number of requests the server could handle in one epoll_wait loop
     */
    Server(std::string ip, int port, int max_request);
    ~Server() {};
public:
    /**
     * Start listening at the server socket to wait events using epoll_wait
     */
    void Listen();
};


#endif //HTTP_SERVER_SERVER_H
