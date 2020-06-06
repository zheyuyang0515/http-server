//
// Created by zheyu on 6/1/20.
//

#include "Server.h"
#include "../threadpool/Task.h"

#define DEFAULT_LISTEN 128
Server::Server(std::string ip, int port, int max_request, ThreadPool *pool) {
    this->max_request = max_request;
    this->pool = pool;
    int ret = -1;   //result
    char* error_msg;    //error message
    struct sockaddr_in serveraddr;      //server and client sockaddr
    struct epoll_event event;   //epoll_event
    int opt_status = 1;        //option used in setsockopt()

    //initiate logger obj
    logger = Logger::get_instance(LOG_DIR, ALL);

    //create server socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Create socket failed(" + std::string(error_msg) + ").", ERROR));
        perror("Server: Create socket failed.");
        exit(-1);
    }

    //set server-socket nonblock
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if(flags == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Get socket descriptor failed(" + std::string(error_msg) + ").", ERROR));
        perror("Server: Get socket descriptor failed.");
        exit(-1);
    }
    ret = fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Set socket descriptor failed(" + std::string(error_msg) + ").", ERROR));
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
        logger->add_log(new Log("Server: Bind socket failed(" + std::string(error_msg) + ").", ERROR));
        perror("Server: Bind socket failed.");
        exit(-1);
    }

    //set port reusable
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt_status, sizeof(opt_status));
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Set sock opt(reuseaddr) failed(" + std::string(error_msg) + ").", WARNING));
        perror("Server: Set sock opt(reuseaddr) failed.");
        //this error does not have fatal effects to the program, so the program do not need to exit
    }
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt_status, sizeof(opt_status));
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Set sock opt(reuseport) failed(" + std::string(error_msg) + ").", WARNING));
        perror("Server: Set sock opt(reuseaddr) failed.");
        //this error does not have fatal effects to the program, so the program do not need to exit
    }

    //set listen
    ret = listen(socket_fd, DEFAULT_LISTEN);
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Listen socket failed(" + std::string(error_msg) + ").", ERROR));
        perror("Server: Listen socket failed.");
        exit(-1);
    }

    //create EPOLL_FD
    epfd = epoll_create(128);
    if(epfd == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Create epoll fd failed(" + std::string(error_msg) + ").", ERROR));
        perror("Server: Create epoll fd failed.");
        exit(-1);
    }

    //add server fd into epoll
    event.events = EPOLLIN;
    event.data.fd = socket_fd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &event);
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Add epoll server fd failed(" + std::string(error_msg) + ").", ERROR));
        perror("Server: Add epoll server fd failed.");
        exit(-1);
    }
    logger->add_log(new Log("Server: Initiate successfully. ip: " + ip + ", port: " + std::to_string(port), INFO));
}

void Server::Listen() {
    logger->add_log(new Log("Server: Server start working, maximal number of requests the server could handle in one epoll_wait loop: " + std::to_string(max_request) + ".", DEBUG));
    struct epoll_event event, events[max_request];  //epoll structs
    int event_sum;      //total number of events in each epoll_wait call
    char *error_msg;    //error message
    int client_fd;      //client fd
    struct sockaddr_in clientaddr;  //client sockaddr
    int ret = -1;

    while(true) {
        event_sum = epoll_wait(epfd, events, max_request, -1);
        //std::cout << "event_sum: " << event_sum << std::endl;
        //no events found
        if(event_sum == 0) {
            continue;
        }
        //error
        if(event_sum == -1) {
            error_msg = strerror(errno);
            logger->add_log(new Log("Server: epoll_wait failed(" + std::string(error_msg) + ").", ERROR));
            perror("Server: epoll_wait failed.");
            exit(-1);
        }
        //found events
        for(auto i = 0; i < event_sum; ++i) {
            if(events[i].data.fd == socket_fd) {
                //accept(new connection)
                socklen_t socklen = sizeof(struct sockaddr);
                client_fd = accept(socket_fd, (struct sockaddr *)&clientaddr, &socklen);
                if(client_fd == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Accept new client failed(" + std::string(error_msg) + ").", ERROR));
                    perror("Server: Accept new client failed.");
                    exit(-1);
                }
                //set client_fd descriptor nonblock
                int flags = fcntl(client_fd, F_GETFL, 0);
                if(flags == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Get client descriptor failed(" + std::string(error_msg) + ").", ERROR));
                    perror("Server: Get client descriptor failed.");
                    exit(-1);
                }
                ret = fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                if(ret == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Set client descriptor failed(" + std::string(error_msg) + ").", ERROR));
                    perror("Server: Set client descriptor failed.");
                    exit(-1);
                }
                //set client's event
                event.events = EPOLLIN | EPOLLERR | EPOLLET;
                struct client_struct *cs = new client_struct();
                cs->client_fd = client_fd;
                cs->clientaddr = clientaddr;
                event.data.ptr = cs;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event);
                if(ret == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Add epoll client fd(1st time) failed(" + std::string(error_msg) + ").", ERROR));
                    perror("Server: Add epoll client fd(1st time) failed.");
                    exit(-1);
                }
                logger->add_log(new Log("Server: Incoming new connection. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ".", INFO));
            } else {
                //client closed
                if(events[i].events & EPOLLERR) {
                    struct client_struct *cs = (struct client_struct *)events[i].data.ptr;
                    clientaddr = cs->clientaddr;
                    client_fd = cs->client_fd;
                    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, nullptr);
                    if(ret == -1) {
                        logger->add_log(new Log("Server: Remove fd from epfd(" + std::string(error_msg) + ").", WARNING));
                    }
                    close(client_fd);
                    logger->add_log(new Log("Server: Client disconnected. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ".", INFO));
                    delete cs;
                    continue;
                }
                //create task struct
                struct task_struct *ts = new task_struct();
                ts->epfd = epfd;
                ts->logger = logger;
                ts->cs = new client_struct();
                ts->cs->client_fd = ((struct client_struct *)events[i].data.ptr)->client_fd;
                ts->cs->clientaddr = ((struct client_struct *)events[i].data.ptr)->clientaddr;
                //create task object
                logger->add_log(new Log("Server: Recv Request from a client. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ".", DEBUG));
                //std::cout << "Server: Recv Request from a client. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + "." << std::endl;
                Task *task = new Task(Server::handle_request, ts);
                //remove client descriptor from epoll
                /*ret = epoll_ctl(epfd, EPOLL_CTL_DEL, ((struct client_struct *)events[i].data.ptr)->client_fd, nullptr);
                if(ret == -1) {
                    logger->logging_warning("Server: Remove fd from epfd(" + std::string(error_msg) + ").");
                }*/
                //add task to the queue
                pool->add_task(task);
            }
        }

    }
}

char* Server::readn(int n, struct client_struct *cs, Logger *logger, int epfd) {
    char in_buff[n + 1];
    char *result = new char[n + 1];
    struct sockaddr_in clientaddr = cs->clientaddr;
    char *error_msg;
    //reset buffer
    memset(in_buff, 0, sizeof(in_buff));
    memset(result, 0, sizeof(result));
    int ret = -1;

    //One recv might not be able to read enough length of chars
    do{
        ret = recv(cs->client_fd, in_buff, n, 0);
        if(ret > 0) {
            strcat(result, in_buff);
        }
    } while(strlen(result) < n && (ret > 0 || errno == EINTR));

    //client closed or an error occurred(eagain and ewouldblock means resource temporarily unavailable, not an error)
    if(ret <= 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        if(ret == 0) {
            logger->add_log(new Log("Server: Client disconnected. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ".", INFO));
        } else {
            error_msg = strerror(errno);
            logger->add_log(new Log("Server: Recv data from client socket failed. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ". (" + std::string(error_msg) + ")", ERROR));
        }
        ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cs->client_fd, nullptr);
        if(ret == -1) {
            error_msg = strerror(errno);
            logger->add_log(new Log("Server: Remove fd from epfd(" + std::string(error_msg) + ").", WARNING));
        }
        close(cs->client_fd);
        delete cs;
        return nullptr;
    }
    return result;
}

void Server::handle_request(void *arg) {
    //std::cout << "handle_request" << std::endl;
    int ret = -1;
    struct task_struct *ts = (struct task_struct *)arg;
    struct client_struct *cs = ts->cs;
    Logger *logger = ts->logger;
    int epfd = ts->epfd;
    int n = 1024;
    struct epoll_event event;
    char* error_msg;
    std::string msg = "received msg!";

    //test
    char *in_buff;
    in_buff = readn(n, cs, logger, epfd);
    if(in_buff != nullptr && strlen(in_buff) != 0) {
        std::cout << "recv: " << in_buff << std::endl;
        send(cs->client_fd, msg.c_str(), msg.length(), 0);
        /*event.events = EPOLLIN | EPOLLERR;
        event.data.ptr = cs;
        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cs->client_fd, &event);
        if(ret == -1) {
            error_msg = strerror(errno);
            logger->logging_warning("Server: Add epoll client fd failed(" + std::string(error_msg) + ").");
            perror("Server: Add epoll client fd failed");
        }*/
    }
    delete in_buff;
}
