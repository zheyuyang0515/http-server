//
// Created by zheyu on 6/1/20.
//

#include "Server.h"

#define PIC_DIR "res"
#define HTML_DIR "res/html"
//init pic_type
const std::unordered_map<std::string, std::string> Server::pic_type_map {{"jpg", "image/jpg"}, {"jpeg", "image/jpeg"}, {"gif", "image/gif"}, {"ico", "image/ico"}, {"png", "image/png"}, {"bmp", "image/bmp"}};
const std::unordered_map<std::string, std::string> Server::text_type_map {{"html", "text/html"}, {"css", "text/css"}, {"js", "application/javascript"}};
int Server::round_ptr;
int Server::round_ctr;
Server::Server(std::string ip, int port, int max_request, ThreadPool *pool, struct page_struct ps, int max_connection) {
    this->max_request = max_request;
    this->pool = pool;
    int ret = -1;   //result
    char* error_msg;    //error message
    struct sockaddr_in serveraddr;      //server and client sockaddr
    struct epoll_event event;   //epoll_event
    int opt_status = 1;        //option used in setsockopt()
    //initiate logger obj
    logger = Logger::get_instance(LOG_DIR, Log::ALL);
    //set proxy_algorithm
    if(reverse_proxy_mode == true) {
        round_ctr = 0;
        round_ptr = 0;
    }


    //create server socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Create socket failed(" + std::string(error_msg) + ").", Log::ERROR));
        perror("Server: Create socket failed.");
        exit(-1);
    }

    //set server-socket nonblock
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if(flags == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Get socket descriptor failed(" + std::string(error_msg) + ").", Log::ERROR));
        perror("Server: Get socket descriptor failed.");
        exit(-1);
    }
    ret = fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Set socket descriptor failed(" + std::string(error_msg) + ").", Log::ERROR));
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
        logger->add_log(new Log("Server: Bind socket failed(" + std::string(error_msg) + ").", Log::ERROR));
        perror("Server: Bind socket failed.");
        exit(-1);
    }

    //set port reusable
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt_status, sizeof(opt_status));
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Set sock opt(reuseaddr) failed(" + std::string(error_msg) + ").", Log::WARNING));
        perror("Server: Set sock opt(reuseaddr) failed.");
        //this error does not have fatal effects to the program, so the program do not need to exit
    }
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt_status, sizeof(opt_status));
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Set sock opt(reuseport) failed(" + std::string(error_msg) + ").", Log::WARNING));
        perror("Server: Set sock opt(reuseaddr) failed.");
        //this error does not have fatal effects to the program, so the program do not need to exit
    }

    //set listen
    ret = listen(socket_fd, max_connection);
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Listen socket failed(" + std::string(error_msg) + ").", Log::ERROR));
        perror("Server: Listen socket failed.");
        exit(-1);
    }

    //create EPOLL_FD
    epfd = epoll_create(128);
    if(epfd == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Create epoll fd failed(" + std::string(error_msg) + ").", Log::ERROR));
        perror("Server: Create epoll fd failed.");
        exit(-1);
    }

    //add server fd into epoll
    event.events = EPOLLIN;
    event.data.fd = socket_fd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &event);
    if(ret == -1) {
        error_msg = strerror(errno);
        logger->add_log(new Log("Server: Add epoll server fd failed(" + std::string(error_msg) + ").", Log::ERROR));
        perror("Server: Add epoll server fd failed.");
        exit(-1);
    }
    logger->add_log(new Log("Server: Initiate successfully. ip: " + ip + ", port: " + std::to_string(port), Log::INFO));
}

void Server::Listen() {
    logger->add_log(new Log("Server: Server start working, maximal number of requests the server could handle in one epoll_wait loop: " + std::to_string(max_request) + ".", Log::DEBUG));
    struct epoll_event event, events[max_request];  //epoll structs
    int event_sum;      //total number of events in each epoll_wait call
    char *error_msg;    //error message
    int client_fd;      //client fd
    struct sockaddr_in clientaddr;  //client sockaddr
    int ret = -1;

    while(true) {
        event_sum = epoll_wait(epfd, events, max_request, -1);
        //std::cout << event_sum << std::endl;
        //no events found
        if(event_sum == 0) {
            continue;
        }
        //error
        if(event_sum == -1) {
            error_msg = strerror(errno);
            logger->add_log(new Log("Server: epoll_wait failed(" + std::string(error_msg) + ").", Log::ERROR));
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
                    logger->add_log(new Log("Server: Accept new client failed(" + std::string(error_msg) + ").", Log::ERROR));
                    perror("Server: Accept new client failed.");
                    exit(-1);
                }
                //set client_fd descriptor nonblock
                int flags = fcntl(client_fd, F_GETFL, 0);
                if(flags == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Get client descriptor failed(" + std::string(error_msg) + ").", Log::ERROR));
                    perror("Server: Get client descriptor failed.");
                    exit(-1);
                }
                ret = fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                if(ret == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Set client descriptor failed(" + std::string(error_msg) + ").", Log::ERROR));
                    perror("Server: Set client descriptor failed.");
                    exit(-1);
                }
                //set client's event
                event.events = EPOLLIN | EPOLLERR | EPOLLET;
                struct client_struct *cs = new client_struct();
                cs->client_fd = client_fd;
                cs->clientaddr = clientaddr;
                cs->mode = 1;
                event.data.ptr = cs;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event);
                if(ret == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Add epoll client fd(1st time) failed(" + std::string(error_msg) + ").", Log::ERROR));
                    perror("Server: Add epoll client fd(1st time) failed.");
                    exit(-1);
                }
                logger->add_log(new Log("Server: Incoming new connection. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ".", Log::INFO));
            } else {
                //client closed
                if(events[i].events & EPOLLERR) {
                    struct client_struct *cs = (struct client_struct *)events[i].data.ptr;
                    clientaddr = cs->clientaddr;
                    client_fd = cs->client_fd;
                    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, nullptr);
                    if(ret == -1) {
                        logger->add_log(new Log("Server: Remove fd from epfd(" + std::string(error_msg) + ").", Log::WARNING));
                    }
                    close(client_fd);
                    logger->add_log(new Log("Server: Client disconnected. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ".", Log::INFO));
                    delete cs;
                    continue;
                }
                if(((struct client_struct *)events[i].data.ptr)->mode == 1) {   //static resources
                    //create task struct
                    struct task_struct *ts = new task_struct();
                    ts->epfd = epfd;
                    ts->logger = logger;
                    ts->cs = new client_struct();
                    ts->cs->client_fd = ((struct client_struct *)events[i].data.ptr)->client_fd;
                    ts->cs->clientaddr = ((struct client_struct *)events[i].data.ptr)->clientaddr;

                    //create task object
                    logger->add_log(new Log("Server: Recv Request from a client. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ".", Log::DEBUG));
                    Task *task = new Task(Server::handle_request, ts);
                    //add task to the queue
                    pool->add_task(task);
                } else if(((struct client_struct *)events[i].data.ptr)->mode == 2) {            //reverse proxy
                    //std::cout << "reverse" << std::endl;
                    //std::cout << "response received from host" << ((struct client_struct *)events[i].data.ptr)->client_fd << std::endl;
                    struct dispatch_response_task_struct *drts = new dispatch_response_task_struct;
                    drts->logger = logger;
                    drts->cs = new client_struct();
                    drts->cs->client_fd = ((struct client_struct *)events[i].data.ptr)->client_fd;
                    drts->cs->clientaddr = ((struct client_struct *)events[i].data.ptr)->clientaddr;
                    //create task object
                    logger->add_log(new Log("Server: Recv Response from a host/original server. Client IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ".", Log::DEBUG));
                    Task *task = new Task(Server::dispatch_response, drts);
                    //add task to the queue
                    pool->add_task(task);
                }

            }
        }

    }
}
int Server::readn(char* buff, int n, int client_fd) {
    //std::cout << "123" << std::endl;
    int nleft = n;
    int count = 0;
    while(nleft > 0) {
        if((count = recv(client_fd, buff, nleft, 0)) < 0) {
            //std::cout << count << std::endl;
            if(errno == EINTR) {
                count = 0;
            } else if(errno == EWOULDBLOCK || errno == EAGAIN) {
                return -2;
            } else {
                return -1;
            }
        } else if(count == 0) {
            break;
        }
        nleft -= count;
        buff += count;
    }
    //std::cout << "456" << std::endl;
    return n - nleft;
}
void Server::handle_request(void *arg) {
    int ret = -1;
    struct task_struct *ts = (struct task_struct *)arg;
    struct client_struct *cs = ts->cs;
    Logger *logger = ts->logger;
    int epfd = ts->epfd;
    char* error_msg;
    int result = 0;    //final result of this function, >=0 on success, -1: error when interacting with client, -2: error when interacting with host server
    int send_fd;

    //parse http request header
    struct http_header_get_struct header_struct;
    ret = Server::parse_http_header(&header_struct, cs, logger, epfd);
    if(ret == -1) {
        //disconnect client
        ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cs->client_fd, nullptr);
        if(ret == -1) {
            error_msg = strerror(errno);
            logger->add_log(new Log("Server: Remove fd from epfd(" + std::string(error_msg) + ").", Log::WARNING));
        }
        close(cs->client_fd);
        delete cs;
        delete ts;
        return;
    }
    if(!reverse_proxy_mode) {       //reverse proxy service is off
        do{
            std::unordered_map<std::string, std::string>::iterator it;  //map iterator
            std::string req_method = "UNKNOWN";
            std::string req_url = "UNKNOWN";
            std::string user_agent = "UNKNOWN";
            std::string suffix = header_struct.header_map["suffix"];
            //picture
            if(pic_type_map.count(suffix) > 0) {
                ret = http_response(PIC_DIR, header_struct.header_map, cs, logger, Server::PIC);
                if(ret == -1) {
                    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cs->client_fd, nullptr);
                    if(ret == -1) {
                        error_msg = strerror(errno);
                        logger->add_log(new Log("Server: Remove fd from epfd(" + std::string(error_msg) + ").", Log::WARNING));
                    }
                    close(cs->client_fd);
                    pthread_mutex_lock(&keep_alive_map_mutex);
                    keep_alive_map.erase(cs->client_fd);
                    pthread_mutex_unlock(&keep_alive_map_mutex);
                    delete cs;
                    delete ts;
                    return;
                }
            } else {
                //text
                ret = http_response(HTML_DIR, header_struct.header_map, cs, logger, Server::HTML);
                if(ret == -1) {
                    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cs->client_fd, nullptr);
                    if(ret == -1) {
                        error_msg = strerror(errno);
                        logger->add_log(new Log("Server: Remove fd from epfd(" + std::string(error_msg) + ").", Log::WARNING));
                    }
                    close(cs->client_fd);
                    pthread_mutex_lock(&keep_alive_map_mutex);
                    keep_alive_map.erase(cs->client_fd);
                    pthread_mutex_unlock(&keep_alive_map_mutex);
                    delete cs;
                    delete ts;
                    return;
                }
            }
            if((it = header_struct.header_map.find("req_method")) != header_struct.header_map.end()) {
                req_method = it->second;
            }
            if((it = header_struct.header_map.find("req_url")) != header_struct.header_map.end()) {
                req_url = it->second;
            }
            if((it = header_struct.header_map.find("User-Agent")) != header_struct.header_map.end()) {
                user_agent = it->second;
            }
            logger->add_log(new Log("Server: New Request " + req_method + " " + req_url + " from: " + std::string(inet_ntoa(cs->clientaddr.sin_addr)) + ". Browser: " + user_agent + ", Status: " + std::to_string(ret), Log::DEBUG));
            if((it = header_struct.header_map.find("Connection")) == header_struct.header_map.end()) {
                close(cs->client_fd);
            }
        } while(false);
    } else {
        do{
            //if client fd exist then use the previous one(keep-alive)
            if(keep_alive_map.count(cs->client_fd) > 0) {
                send_fd = reverse_proxy_client_map[cs->client_fd];
            } else {    //create new fd to connect to the server;
                send_fd = socket(AF_INET, SOCK_STREAM, 0);
                //random choose a host/original server
                int host_index = get_random_host();
                //connect to that server
                struct sockaddr_in server_addr;
                server_addr.sin_family = AF_INET;
                server_addr.sin_addr.s_addr = inet_addr(host_server_list[host_index]->ip.c_str());
                server_addr.sin_port = htons(host_server_list[host_index]->port);
                ret = connect(send_fd,
                              (struct sockaddr *)&server_addr,
                              sizeof(struct sockaddr));
                if(ret < 0) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log(("Server: Connect to origin failed: " + std::string(error_msg)), Log::ERROR));
                    result = -1;
                    break;
                }
                //set send-socket nonblock
                int flags = fcntl(send_fd, F_GETFL, 0);
                if(flags == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Get send fd descriptor failed during dispatch request(" + std::string(error_msg) + ").", Log::ERROR));
                    result = -1;
                    break;
                }
                ret = fcntl(send_fd, F_SETFL, flags | O_NONBLOCK);
                if(ret == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Set socket descriptor failed during dispatch request(" + std::string(error_msg) + ").", Log::ERROR));
                    result = -1;
                    break;
                }
                //save host_server info
                pthread_mutex_lock(&Server::proxy_host_map_mutex);
                proxy_host_map[cs->client_fd] = server_addr;
                pthread_mutex_unlock(&Server::proxy_host_map_mutex);

                //save reverse_proxy_client_map
                pthread_mutex_lock(&Server::reverse_proxy_client_map_mutex);
                reverse_proxy_client_map[cs->client_fd] = send_fd;
                pthread_mutex_unlock(&Server::reverse_proxy_client_map_mutex);

                //save reverse_proxy_server_map
                pthread_mutex_lock(&Server::reverse_proxy_server_map_mutex);
                reverse_proxy_server_map[send_fd] = cs->client_fd;
                pthread_mutex_unlock(&Server::reverse_proxy_server_map_mutex);

                //add fd into epoll
                struct epoll_event event;
                event.events = EPOLLIN | EPOLLERR | EPOLLET;
                struct client_struct *cs = new client_struct;
                cs->client_fd = send_fd;
                cs->mode = 2;
                event.data.ptr = cs;
                ret = epoll_ctl(ts->epfd, EPOLL_CTL_ADD, send_fd, &event);
                if(ret < 0) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Add client to epoll failed(dispatch request): " + std::string(error_msg), Log::ERROR));
                    result = -2;
                    break;
                }

            }

            //send http msg which is recved from the client to the host server
            ret = Server::dispatch_request(header_struct.header_map, send_fd, cs, logger);
            if(ret < 0) {
                ret = epoll_ctl(ts->epfd, EPOLL_CTL_DEL, send_fd, nullptr);
                if(ret < 0) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Delete client from epoll failed(dispatch request): " + std::string(error_msg), Log::WARNING));
                }
                close(send_fd);
                close(cs->client_fd);
                pthread_mutex_lock(&proxy_host_map_mutex);
                proxy_host_map.erase(cs->client_fd);
                pthread_mutex_unlock(&proxy_host_map_mutex);
                pthread_mutex_lock(&reverse_proxy_client_map_mutex);
                reverse_proxy_client_map.erase(cs->client_fd);
                pthread_mutex_unlock(&reverse_proxy_client_map_mutex);
                pthread_mutex_lock(&reverse_proxy_server_map_mutex);
                reverse_proxy_server_map.erase(send_fd);
                pthread_mutex_unlock(&reverse_proxy_server_map_mutex);
                if(keep_alive_map.count(cs->client_fd) > 0) {
                    pthread_mutex_lock(&keep_alive_map_mutex);
                    keep_alive_map.erase(cs->client_fd);
                    pthread_mutex_unlock(&keep_alive_map_mutex);
                }
                continue;
            }
            //write log save client's info
            std::string req_method = header_struct.header_map.count("req_method") > 0 ? header_struct.header_map["req_method"] : "UNKNOWN";
            std::string req_url = header_struct.header_map.count("req_url") > 0 ? header_struct.header_map["req_url"] : "UNKNOWN";
            std::string user_agent = header_struct.header_map.count("User-Agent") > 0 ? header_struct.header_map["User-Agent"] : "UNKNOWN";
            logger->add_log(new Log("Server: New Request " + req_method + " " + req_url + " from: " + std::string(inet_ntoa(cs->clientaddr.sin_addr)) + ". Browser: " + user_agent + ".", Log::DEBUG));
        } while(false);
    }
    if(result == -1) {  //error when interacting with client
        std::cout << "error -1" << std::endl;
        close(cs->client_fd);
        close(send_fd);
    } else if(result == -2) {   //error when interacting with host server
        std::cout << "error -2" << std::endl;
        close(send_fd);
        close(cs->client_fd);
        pthread_mutex_lock(&proxy_host_map_mutex);
        proxy_host_map.erase(cs->client_fd);
        pthread_mutex_unlock(&proxy_host_map_mutex);
        pthread_mutex_lock(&reverse_proxy_client_map_mutex);
        reverse_proxy_client_map.erase(cs->client_fd);
        pthread_mutex_unlock(&reverse_proxy_client_map_mutex);
        pthread_mutex_lock(&reverse_proxy_server_map_mutex);
        reverse_proxy_server_map.erase(send_fd);
        pthread_mutex_unlock(&reverse_proxy_server_map_mutex);
        if(keep_alive_map.count(cs->client_fd) > 0) {
            pthread_mutex_lock(&keep_alive_map_mutex);
            keep_alive_map.erase(cs->client_fd);
            pthread_mutex_unlock(&keep_alive_map_mutex);
        }
    } else {    //success
        //add client to keep_alive_map if it is keep-alive
        if(header_struct.header_map["Connection"] == "keep-alive") {
            time_t timet;
            time(&timet);
            pthread_mutex_lock(&keep_alive_map_mutex);
            keep_alive_map[cs->client_fd] = timet;
            pthread_mutex_unlock(&keep_alive_map_mutex);
        }
    }
    delete cs;
    delete ts;
}
int Server::dispatch_request(std::unordered_map<std::string, std::string> header_map, int send_fd, struct client_struct *cs, Logger *logger) {
    //std::cout << "start" << std::endl;
    std::string out_buff;
    char *errormsg;
    int ret = -1;
    char in_buff[1];
    //header
    //first line
    out_buff = header_map["req_method"] + " " + header_map["req_url"] + " " + header_map["http_version"] + "\n";
    //all rest of headers
    for(auto entry : header_map) {
        if(entry.first == "req_method" || entry.first == "req_url" || entry.first == "http_version" || entry.first == "suffix") {
            continue;
        }
        //test
        if(entry.first == "Host") {
            out_buff += "Host: " + std::string(inet_ntoa(proxy_host_map[cs->client_fd].sin_addr)) + "\n";
            continue;
        }
        //test
        out_buff += entry.first + ": " + entry.second + "\n";
    }
    out_buff += "\n";
    //std::cout << out_buff << std::endl;
    ret = send(send_fd, out_buff.c_str(), out_buff.length(), 0);
    if(ret == -1) {
        errormsg = strerror(errno);
        logger->add_log(new Log("Server: Dispatch request failed(send request header): " + std::string(errormsg), Log::ERROR));
        return -1;
    }
    //body(if has)
    while(true) {
        memset(in_buff, 0, sizeof(in_buff));
        ret = readn(in_buff, 1, cs->client_fd);
        if(ret == -1) {
            errormsg = strerror(errno);
            logger->add_log(new Log("Server: Dispatch request failed(fetch request body from client): " + std::string(errormsg), Log::ERROR));
            return -1;
        }
        //read to tail or EAGAIN
        if(ret <= 0) {
            break;
        }
        ret = send(send_fd, in_buff, 1, 0);
        if(ret < 0) {
            errormsg = strerror(errno);
            logger->add_log(new Log("Server: Dispatch request failed(send request body): " + std::string(errormsg), Log::ERROR));
            return -1;
        }
    }
    //std::cout << "finish" << std::endl;
    return 1;
}
void Server::dispatch_response(void *arg) {
    //std::cout << "start" << std::endl;
    struct dispatch_response_task_struct *drts = (struct dispatch_response_task_struct *)arg;
    struct client_struct *cs = drts->cs;
    Logger *logger = drts->logger;
    char *error_msg;
    int ret = -1;
    char in_buff[1];
    memset(in_buff, 0, sizeof(in_buff));
    if(reverse_proxy_server_map.count(cs->client_fd) == 0) {
        logger->add_log(new Log("Server: Receive message from host/original server failed: client not found", Log::ERROR));
        return;
    }
    int client_fd = reverse_proxy_server_map[cs->client_fd];
    while((ret = readn(in_buff, 1, cs->client_fd)) > 0) {
        ret = send(client_fd, in_buff, 1, 0);
        if(ret == -1) {
            break;
        }
        memset(in_buff, 0, sizeof(in_buff));
    }
    if(ret == -2) {
        return;
    }
    //error, close
    if(ret <= 0) {
        error_msg = strerror(errno);
        close(cs->client_fd);
        close(client_fd);
        pthread_mutex_lock(&proxy_host_map_mutex);
        proxy_host_map.erase(client_fd);
        pthread_mutex_unlock(&proxy_host_map_mutex);
        pthread_mutex_lock(&reverse_proxy_server_map_mutex);
        reverse_proxy_server_map.erase(cs->client_fd);
        pthread_mutex_unlock(&reverse_proxy_server_map_mutex);
        pthread_mutex_lock(&reverse_proxy_client_map_mutex);
        reverse_proxy_client_map.erase(client_fd);
        pthread_mutex_unlock(&reverse_proxy_client_map_mutex);
        if(keep_alive_map.count(client_fd) > 0) {
            pthread_mutex_lock(&keep_alive_map_mutex);
            keep_alive_map.erase(client_fd);
            pthread_mutex_unlock(&keep_alive_map_mutex);
        }
        if(ret == -1) {
            logger->add_log(new Log("Server: Receive message from host/original server failed: " + std::string(error_msg), Log::ERROR));
        } else {
            logger->add_log(new Log("Server: Client disconnect: " + std::string(error_msg), Log::INFO));
        }
        //std::cout << "disconnect" << std::endl;
    }
}




int Server::parse_http_header(struct http_header_get_struct *header_struct, Server::client_struct *cs, Logger *logger, int epfd) {
    char in_buff[1];    //temp read buffer
    std::string key = "";   //key
    std::string value = "";  //value
    bool is_reading_key = true;     //track pointer traverse the data, is the pointer belong to key(true) or value(false) currently
    //save suffix
    std::string suffix = "";
    bool dot = false;
    int ret = -1;

    // req method
    do {
        memset(in_buff, 0, sizeof(in_buff));
        ret = readn(in_buff, 1, cs->client_fd);
        if(ret == -1) {
            return -1;
        }
        if(ret != 0 && strlen(in_buff) != 0 && strcmp(in_buff, " ") != 0) {
            value += std::string(in_buff);
        }
    } while((ret > 0 || strlen(in_buff) > 0) && strcmp(in_buff, " ") != 0);
    if(value.length() == 0) return -1;
    header_struct->header_map["req_method"] = value;
    value.clear();

    //req_url
    do {
        memset(in_buff, 0, sizeof(in_buff));
        ret = readn(in_buff, 1, cs->client_fd);
        if(ret == -1) {
            return -1;
        }
        if(dot && ret != 0 && strcmp(in_buff, " ") != 0) {
            suffix += in_buff;
        }
        if(strcmp(in_buff, ".") == 0) {
            if(dot) {
                suffix.clear();
            } else {
                dot = true;
            }
        }
        if(ret != 0 && strlen(in_buff) != 0 && strcmp(in_buff, " ") != 0) {
            value += in_buff;
        }
    } while((ret > 0 || strlen(in_buff) > 0) && strcmp(in_buff, " ") != 0);
    if(value.length() == 0) return -1;

    header_struct->header_map["req_url"] = value;
    header_struct->header_map["suffix"] = suffix;
    value.clear();

    //http-version
    do {
        memset(in_buff, 0, sizeof(in_buff));
        ret = readn(in_buff, 1, cs->client_fd);
        if(ret == -1) {
            return -1;
        }
        if(ret != 0 && strcmp(in_buff, " ") != 0 && strcmp(in_buff, "\r") != 0 && strcmp(in_buff, "\n") != 0) {
            value += in_buff;
        }
    } while((ret > 0 || strlen(in_buff) > 0) && strcmp(in_buff, " ") != 0 && strcmp(in_buff, "\r") != 0 && strcmp(in_buff, "\n") != 0);
    if(value.length() == 0) return -1;
    header_struct->header_map["http_version"] = value;
    value.clear();
    //skip '\n'
    if(strcmp(in_buff, "\r") == 0) {
        ret = readn(in_buff, 1, cs->client_fd);
        if(ret == -1) {
            return -1;
        }
    }

    //read the rest of data
    while(true) {
        memset(in_buff, 0, sizeof(in_buff));
        ret = readn(in_buff, 1, cs->client_fd);
        if(ret == -1) return -1;
        if(ret <= 0 || strlen(in_buff) <= 0) {
            break;
        }
        if(strcmp(in_buff, "\r") == 0 || strcmp(in_buff, "\n") == 0) {
            //skip '\n'
            if(strcmp(in_buff, "\r") == 0) {
                ret = readn(in_buff, 1, cs->client_fd);
                if(ret == -1) return -1;
                if(ret <= 0) {
                    break;
                }
            }
            //next is the request body
            if(key.length() == 0 && value.length() == 0) {
                break;
            }
            if(key.length() > 0 && value.length() > 0) {
                header_struct->header_map[key] = value;
            }
            key.clear();
            value.clear();
            is_reading_key = true;
            continue;
        }
        if(strcmp(in_buff, ":") == 0 && is_reading_key) {
            is_reading_key = false;
            //skip a space
            ret = readn(in_buff, 1, cs->client_fd);
            if(ret == -1) return -1;
            if(ret <= 0) break;
            continue;
        }
        if(is_reading_key == true) {
            key += std::string(in_buff);
        } else {
            value += std::string(in_buff);
        }
    }
    return 1;
}

int Server::http_response(std::string dir, std::unordered_map<std::string, std::string> map, Server::client_struct *cs, Logger *logger,
                          Server::request_type type) {
    std::ifstream in;
    std::string req_url = "UNKNOWN";
    std::string http_version = "HTTP/1.1";
    std::string status = "200 OK";
    std::string suffix = "html";
    //keep alive
    bool keep_alive = false;
    int result = 200;
    int ret = -1;
    if(map["req_url"].length() > 0) {
        req_url = map["req_url"];
    }
    if(map["http_version"].length() > 0) {
        http_version = map["http_version"];
    }
    if(map["Connection"].length() > 0 && map["Connection"] == "keep-alive") {
        keep_alive = true;
    }
    //std::cout << req_url << ":" << std::endl;
    suffix = map["suffix"].length() > 0 ? map["suffix"] : "html";
    dir = req_url == "/" ? dir + "/index.html" : dir + req_url;
    std::string out_buff;
    std::unordered_map<std::string, std::string>::iterator it;
    //check if file exist
    while(type == Server::PIC) {
        if(Server::pic_type_map.count(suffix) == 0) {
            dir = std::string(HTML_DIR) + "/500.html";
            type = Server::HTML;
            status = "500 Internal Server Error";
            suffix = "html";
            result = 500;
            break;
        }
        //response pic
        in.open(dir, std::ios::binary);
        //resource not exist
        if(!in) {
            dir = std::string(HTML_DIR) + "/404.html";
            type = Server::HTML;
            status = "404 Not Found";
            suffix = "html";
            result = 404;
            break;
        }
        //get size of a file
        struct stat stat_buff;
        stat(dir.c_str(), &stat_buff);
        int length = stat_buff.st_size;
        //create http header
        //http version and status
        out_buff = http_version + " " + status + "\r\n";
        ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
        if(ret == -1) {
            //error
            return -1;
        }
        //Date
        out_buff = "Date:" + std::string(get_cur_time()) + "\r\n";
        ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
        if(ret == -1) {
            //error
            return -1;
        }
        //keep alive
        if(keep_alive) {
            out_buff = "Connection: keep-alive\r\n";
            ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
            if(ret == -1) {
                //error
                return -1;
            }
        }
        //content type
        std::string content_type = Server::pic_type_map.find(suffix)->second;
        out_buff = "Content-Type: " + content_type + "\r\n";
        ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
        if(ret == -1) {
            //error
            return -1;
        }
        //content length
        out_buff = "Content-Length: " + std::to_string(length) + "\r\n";
        ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
        if(ret == -1) {
            //error
            return -1;
        }
        //line break
        out_buff = "\r\n";
        ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
        if(ret == -1) {
            //error
            return -1;
        }
        //read pic and send body
        std::string pic_buff;
        while(!in.eof()) {
            getline(in, pic_buff);
            if(!in.eof()) {
                pic_buff += "\n";
            }
            ret = send(cs->client_fd, pic_buff.c_str(), pic_buff.length(), 0);
            if(ret == -1) {
                //error
                return -1;
            }
        }
        return result;
    }
    if(Server::text_type_map.count(suffix) == 0) {
        dir = std::string(HTML_DIR) + "/500.html";
        status = "500 Internal Server Error";
        suffix = "html";
        result = 500;
    }
    //response html
    in.open(dir);
    if(!in) {
        dir = std::string(HTML_DIR) + "/404.html";
        in.open(dir);
        status = "404 Not Found";
        result = 404;
    }
    //create http header
    //http version and status
    out_buff = http_version + " " + status + "\r\n";
    ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
    if(ret == -1) {
        //error
        return -1;
    }
    //Date
    out_buff = "Date:" + std::string(get_cur_time()) + "\r\n";
    ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
    if(ret == -1) {
        //error
        return -1;
    }
    //keep-alive
    if(keep_alive) {
        out_buff = "Connection: keep-alive\r\n";
        ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
        if(ret == -1) {
            //error
            return -1;
        }
    }
    //content type
    std::string content_type = Server::text_type_map.find(suffix)->second;
    out_buff = "Content-Type: " + content_type + "\r\n";
    ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
    if(ret == -1) {
        //error
        return -1;
    }
    //content length
    //get size of a file
    struct stat stat_buff;
    stat(dir.c_str(), &stat_buff);
    int length = stat_buff.st_size;
    //send content length
    out_buff = "Content-Length: " + std::to_string(length) + "\r\n";
    ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
    if(ret == -1) {
        //error
        return -1;
    }
    //line break
    out_buff = "\r\n";
    ret = send(cs->client_fd, out_buff.c_str(), out_buff.length(), 0);
    if(ret == -1) {
        //error
        return -1;
    }
    //body
    std::string str;
    while(!in.eof()) {
        std::getline(in, str);
        if(!in.eof()) {
            str += "\n";
        }
        ret = send(cs->client_fd, str.c_str(), str.length(), 0);
        if(ret == -1) {
            perror("send content");
            result = -1;
            break;
        }
    }
    in.close();
    //std::cout << "http_response end" << std::endl;
    return result;
}

int Server::get_random_host() {
    srand((unsigned)time(NULL));
    int host_index;
    int weight;
    if(proxyAlgorithm == Utility::random) {
        weight = rand() % (total_weight + 1);
        //std::cout << weight << std::endl;
        for(int i = 0; i < proxy_server_num; i++) {
            auto host = host_server_list[i];
            weight -= host->weight;
            if(weight <= 0) {
                return i;
            }
        }
    } else if(proxyAlgorithm == Utility::round_robin){
        host_index = round_ptr;
        round_ctr++;
        if(round_ctr > host_server_list[round_ptr]->weight) {
            round_ctr = 0;
            round_ptr++;
            if(round_ptr >= proxy_server_num) {
                round_ptr = 0;
                round_ctr = 0;
            }
        }
        return host_index;
    }
    return -1;



}