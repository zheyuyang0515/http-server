//
// Created by zheyu on 6/1/20.
//

#include "Server.h"
#define DEFAULT_LISTEN 128
Server::Server(std::string ip, int port, int max_request) {
    this->max_request = max_request;
    int ret = -1;   //result
    char* error_msg;    //error message
    struct sockaddr_in serveraddr;      //server and client sockaddr
    struct epoll_event event;   //epoll_event
    int opt_status = 1;        //option used in setsockopt()

    //initiate logger obj
    logger = Logger::get_instance();

    //create server socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1) {
        error_msg = strerror(errno);
        logger->logging_error("Server: Create socket failed(" + std::string(error_msg) + ").");
        perror("Server: Create socket failed.");
        exit(-1);
    }

    //set socket nonblock
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if(flags == -1) {
        error_msg = strerror(errno);
        logger->logging_error("Server: Get socket descriptor failed(" + std::string(error_msg) + ").");
        perror("Server: Get socket descriptor failed.");
        exit(-1);
    }
    ret = fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->logging_error("Server: Set socket descriptor failed(" + std::string(error_msg) + ").");
        perror("Server: Set socket descriptor failed.");
        exit(-1);
    }

    //bind socket
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip.c_str());
    ret = bind(socket_fd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->logging_error("Server: Bind socket failed(" + std::string(error_msg) + ").");
        perror("Server: Bind socket failed.");
        exit(-1);
    }

    //set port reusable
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt_status, sizeof(opt_status));
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->logging_warning("Server: Set sock opt(reuseaddr) failed(" + std::string(error_msg) + ").");
        perror("Server: Set sock opt(reuseaddr) failed.");
        //this error does not have fatal effects to the program, so the program do not need to exit
    }

    //set listen
    ret = listen(socket_fd, DEFAULT_LISTEN);
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->logging_error("Server: Listen socket failed(" + std::string(error_msg) + ").");
        perror("Server: Listen socket failed.");
        exit(-1);
    }

    //create EPOLL_FD
    epfd = epoll_create(128);
    if(epfd == -1) {
        error_msg = strerror(errno);
        logger->logging_error("Server: Create epoll fd failed(" + std::string(error_msg) + ").");
        perror("Server: Create epoll fd failed.");
        exit(-1);
    }

    //add server fd into epoll
    event.events = EPOLLIN;
    event.data.fd = socket_fd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &event);
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->logging_error("Server: Add epoll server fd failed(" + std::string(error_msg) + ").");
        perror("Server: Add epoll server fd failed.");
        exit(-1);
    }
    logger->logging_info("Server: Initiate successfully. ip: " + ip + ", port: " + std::to_string(port));
}

void Server::Listen() {
    logger->logging_info("Server: Server start working, maximal number of requests the server could handle in one epoll_wait loop: " + std::to_string(max_request) + ".");
    struct epoll_event event, events[max_request];  //epoll structs
    int event_sum;      //total number of events in each epoll_wait call
    char *error_msg;    //error message
    int client_fd;      //client fd
    struct sockaddr_in clientaddr;  //client sockaddr
    int ret = -1;

    while(true) {
        event_sum = epoll_wait(epfd, events, max_request, -1);
        //no events found
        if(event_sum == 0) {
            continue;
        }
        //error
        if(event_sum == -1) {
            error_msg = strerror(errno);
            logger->logging_error("Server: epoll_wait failed(" + std::string(error_msg) + ").");
            perror("Server: epoll_wait failed.");
            exit(-1);
        }
        //found events
        for(auto i = 0; i < event_sum; ++i) {
            //accept(new connection)
            if(events[i].data.fd == socket_fd) {
                socklen_t socklen = sizeof(struct sockaddr);
                client_fd = accept(socket_fd, (struct sockaddr *)&clientaddr, &socklen);
                if(client_fd == -1) {
                    error_msg = strerror(errno);
                    logger->logging_error("Server: Accept new client failed(" + std::string(error_msg) + ").");
                    perror("Server: Accept new client failed.");
                    exit(-1);
                }
                //set client's event
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &event);
                if(ret == -1) {
                    error_msg = strerror(errno);
                    logger->logging_error("Server: Add epoll server fd failed(" + std::string(error_msg) + ").");
                    perror("Server: Add epoll server fd failed.");
                    exit(-1);
                }
                logger->logging_info("Server: Incoming new connection. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ".");
            } else {
                //TODO: Existing client, data transfer
            }
        }

    }
}
