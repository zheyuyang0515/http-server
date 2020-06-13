//
// Created by zheyu on 6/1/20.
//

#include "Server.h"

#define DEFAULT_LISTEN 128
#define PIC_DIR "../res/pic"
#define HTML_DIR "../res/html"
//init pic_type
const std::unordered_set<std::string> Server::pic_type_set {"jpg", "jpeg", "gif", "ico", "png", "bmp"};
Server::Server(std::string ip, int port, int max_request, ThreadPool *pool) {
    this->max_request = max_request;
    this->pool = pool;
    int ret = -1;   //result
    char* error_msg;    //error message
    struct sockaddr_in serveraddr;      //server and client sockaddr
    struct epoll_event event;   //epoll_event
    int opt_status = 1;        //option used in setsockopt()

    //initiate logger obj
    logger = Logger::get_instance(LOG_DIR, Log::ALL);

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
    ret = listen(socket_fd, DEFAULT_LISTEN);
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
        //std::cout << "event_sum: " << event_sum << std::endl;
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
    if(ret == 0 || (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        if(ret == 0) {
            logger->add_log(new Log("Server: Client disconnected. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ".", Log::INFO));
        } else {
            error_msg = strerror(errno);
            logger->add_log(new Log("Server: Recv data from client socket failed. IP: " + std::string(inet_ntoa(clientaddr.sin_addr)) + ", PORT: " + std::to_string(ntohs(clientaddr.sin_port)) + ". (" + std::string(error_msg) + ")", Log::ERROR));
        }
        return nullptr;
    }
    return result;
}

void Server::handle_request(void *arg) {
    int ret = -1;
    struct task_struct *ts = (struct task_struct *)arg;
    struct client_struct *cs = ts->cs;
    Logger *logger = ts->logger;
    int epfd = ts->epfd;
    char* error_msg;
    do{
        //parse http request header
        struct http_header_get_struct *header_struct = new http_header_get_struct();
        header_struct->header_map = new std::unordered_map<std::string, std::string>;
        header_struct->arg_map = new std::unordered_map<std::string, std::string>;
        ret = Server::parse_http_header(header_struct, cs, logger, epfd);
        if(ret == -1) {
            //disconnect client
            ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cs->client_fd, nullptr);
            if(ret == -1) {
                error_msg = strerror(errno);
                logger->add_log(new Log("Server: Remove fd from epfd(" + std::string(error_msg) + ").", Log::WARNING));
            }
            close(cs->client_fd);
            delete cs;
            break;
        }
        std::unordered_map<std::string, std::string>::iterator it;  //map iterator
        std::string req_method = "UNKNOWN";
        std::string req_url = "UNKNOWN";
        std::string user_agent = "UNKNOWN";
        //create response message
        //transform to lower case
        std::transform(req_url.begin(), req_url.end(), req_url.begin(), ::tolower);
        //picture
        if((it = header_struct->header_map->find("suffix")) != header_struct->header_map->end() && pic_type_set.count(it->second) > 0) {
            ret = http_response(PIC_DIR, header_struct->header_map, cs, logger, Server::PIC);
            if(ret == -1) {
                ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cs->client_fd, nullptr);
                if(ret == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Remove fd from epfd(" + std::string(error_msg) + ").", Log::WARNING));
                }
                close(cs->client_fd);
                break;
            }
        } else {
            //HTML
            ret = http_response(HTML_DIR, header_struct->header_map, cs, logger, Server::HTML);
            if(ret == -1) {
                ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cs->client_fd, nullptr);
                if(ret == -1) {
                    error_msg = strerror(errno);
                    logger->add_log(new Log("Server: Remove fd from epfd(" + std::string(error_msg) + ").", Log::WARNING));
                }
                close(cs->client_fd);
                break;
            }
        }
        if((it = header_struct->header_map->find("req_method")) != header_struct->header_map->end()) {
            req_method = it->second;
        }
        if((it = header_struct->header_map->find("req_url")) != header_struct->header_map->end()) {
            req_url = it->second;
        }
        if((it = header_struct->header_map->find("User-Agent")) != header_struct->header_map->end()) {
            user_agent = it->second;
        }
        logger->add_log(new Log("Server: New Request " + req_method + " " + req_url + " from: " + std::string(inet_ntoa(cs->clientaddr.sin_addr)) + ". Browser: " + user_agent + ", Status: " + std::to_string(ret), Log::DEBUG));
        if((it = header_struct->header_map->find("Connection")) == header_struct->header_map->end()) {
            close(cs->client_fd);
        }
    } while(false);
    delete ts;
    delete cs;
}

int Server::parse_http_header(struct http_header_get_struct *header_struct, Server::client_struct *cs, Logger *logger, int epfd) {
    std::unordered_map<std::string, std::string> *header_map = header_struct->header_map;     //store header result
    //std::unordered_map<std::string, std::string> *arg_map = header_struct->arg_map; //store arguments result
    char* in_buff;    //temp read buffer
    std::string key = "";   //key
    std::string value = "";  //value
    bool is_reading_key = true;     //track pointer traverse the data, is the pointer belong to key(true) or value(false) currently
    int count = 0;
    std::string first_line_key[] = {"req_method", "req_url", "http_version"};
    //save suffix
    std::string suffix = "";
    bool dot = false;
    //read first line
    while(true) {
        in_buff = readn(1, cs, logger, epfd);
        if(in_buff == nullptr || strlen(in_buff) <= 0 || strcmp(in_buff, "\r") == 0 || strcmp(in_buff, "\n") == 0) {
            if(value.length() > 0) {
                header_map->insert(std::pair<std::string, std::string>(first_line_key[count], value));
            }
            value = "";
            //skip '\n'
            if(in_buff != nullptr && strcmp(in_buff, "\r") == 0) {
                delete in_buff;
                in_buff = readn(1, cs, logger, epfd);
            }
            delete in_buff;
            break;
        }
        if(strcmp(in_buff, " ") == 0) {
            if(value.length() > 0) {
                header_map->insert(std::pair<std::string, std::string>(first_line_key[count], value));
            }
            if(count == 1) {
                if(suffix.length() == 0) {
                    header_map->insert(std::pair<std::string, std::string>("suffix", "html"));
                } else {
                    header_map->insert(std::pair<std::string, std::string>("suffix", suffix));
                }
                dot = false;
            }
            count++;
            value = "";
            delete in_buff;
            continue;
        }
        if(dot) {
            suffix += std::string(in_buff);
        }
        if(count == 1 && strcmp(in_buff, ".") == 0) {
            dot = true;
        }
        value += std::string(in_buff);
        delete in_buff;
    }
    //read rest of the data
    while(true) {
        in_buff = readn(1, cs, logger, epfd);
        if(in_buff == nullptr || strlen(in_buff) <= 0) {
            delete in_buff;
            break;
        }
        if(strcmp(in_buff, "\r") == 0 || strcmp(in_buff, "\n") == 0) {
            //skip '\n'
            if(strcmp(in_buff, "\r") == 0) {
                delete in_buff;
                in_buff = readn(1, cs, logger, epfd);
            }
            if(key.length() > 0 && value.length() > 0) {
                header_map->insert(std::pair<std::string, std::string>(key, value));
            }
            key = "";
            value = "";
            is_reading_key = true;
            delete in_buff;
            continue;
        }
        if(strcmp(in_buff, ":") == 0 && is_reading_key) {
            is_reading_key = false;
            delete in_buff;
            //skip a space
            delete readn(1, cs, logger, epfd);
            continue;
        }
        if(is_reading_key == true) {
            key += std::string(in_buff);
        } else {
            value += std::string(in_buff);
        }
        delete in_buff;
    }
    return header_map->size() == 0 ? -1 : 1;
}

int Server::http_response(std::string dir, std::unordered_map<std::string, std::string> *map, Server::client_struct *cs, Logger *logger,
                          Server::request_type type) {
    std::ifstream in;
    std::unordered_map<std::string, std::string>::iterator it;  //map iterator
    std::string req_url = "UNKNOWN";
    std::string http_version = "HTTP/1.1";
    std::string status = "200 OK";
    std::string suffix = "html";
    //keep alive
    bool keep_alive = false;
    int result = 200;
    int ret = -1;
    if((it = map->find("req_url")) != map->end()) {
        req_url = it->second;
    }
    if((it = map->find("http_version")) != map->end()) {
        http_version = it->second;
    }
    if((it = map->find("Connection")) != map->end() && it->second == "keep-alive") {
        keep_alive = true;
    }
    if((it = map->find("suffix")) != map->end()) {
        suffix = it->second;
    }
    dir = req_url == "/" ? dir + "/index.html" : dir + req_url;
    std::string out_buff;
    //check if file exist
    while(type == Server::PIC) {
        //response pic
        in.open(dir, std::ios::binary);
        //resource not exist
        if(!in) {
            dir = std::string(HTML_DIR) + "/404.html";
            type = Server::HTML;
            status = "404 Not Found";
            result = 404;
            break;
        }
        //get size of a file
        struct stat stat_buff;
        stat(dir.c_str(), &stat_buff);
        int length = stat_buff.st_size;
        std::cout << length << std::endl;
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
        out_buff = "Content-Type: image/" + suffix + "\r\n";
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
    out_buff = "Content-Type: text/" + suffix + ", charset=UTF-8\r\n";
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
    std::cout << length << std::endl;
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
    int count = 0;
    while(!in.eof()) {
        std::getline(in, str);
        if(!in.eof()) {
            str += "\n";
        }
        count += str.length();
        ret = send(cs->client_fd, str.c_str(), str.length(), 0);
        if(ret == -1) {
            perror("send");
            result = -1;
            break;
        }
    }
    in.close();
    return result;
}
