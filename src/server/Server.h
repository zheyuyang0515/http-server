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
class Server {
private:
    enum request_type {PIC, HTML};
    enum request_method {GET, POST};
    int socket_fd;      //server fd
    int epfd;           //epoll fd
    Logger *logger;     //logger
    int max_request;    //maximal number of requests the server could handle in one epoll_wait loop
    ThreadPool *pool;   //thread pool
    static const std::unordered_set<std::string> pic_type_set;    //save all pic suffix(bmp, jpeg, png, gif, jpg, ico)
    static const std::unordered_set<std::string> text_type_set;    //save all pic suffix(html, css)
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
    //static int readn(char *buff, int n, struct client_struct *cs, Logger *logger, int epfd);
    static int readn(char *buff, int n, int client_fd);
    /**
     * handle request
     * @param ts: struct task_struct
     */
    static void handle_request(void *ts);
private:
    struct http_header_get_struct {
        std::unordered_map<std::string, std::string> header_map;    //header information(k-v)
        std::unordered_map<std::string, std::string> arg_map;       //argument in the url(k-v): this map is not being used currently
    };
    static void free_http_header_get_struct(struct http_header_get_struct *h) {
        delete h;
    }
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