//
// Created by zheyu on 6/4/20.
//

#ifndef HTTP_SERVER_UTILITY_H
#define HTTP_SERVER_UTILITY_H

#include <string>
#include "../tinyxml2/tinyxml2.h"
#include "../logger/Log.h"
#include <iostream>
#include <sys/sysinfo.h>
#include <vector>
#define LOG_DIR "logs/log"
#define CONF_DIR "conf/server.xml"

#define DEFAULT_MAX_CONNECTION 128
#define DEFAULT_MIN_WORKER 1
#define DEFAULT_ADJUST_TIME 1
#define DEFAULT_MAIN_PAGE "index.html"
#define DEFAULT_ERROR_404_PAGE "404.html"
#define DEFAULT_SERVERITY_IGNORE Log::ALL
#define DEFAULT_MAX_REQUEST 1024
#define DEFAULT_ADD_WORKER_RATE 0.8
#define DEFAULT_DELETE_WORKER_RATE 0.3
class Utility {
public:
    enum proxy_algorithm {round_robin, random};
    struct host_server_struct {
        std::string ip;
        int port;
        int weight;
    };
    struct init_struct {
        std::string ip;
        int port;
        int max_connection;
        int min_worker;
        int max_worker;
        int add_step;
        int adjust_time;
        int max_request;
        std::string main_page;
        std::string error_404_page;
        float add_worker_rate;
        float delete_worker_rate;
        Log::Level severity_ignore;
        bool reverse_proxy_mode = false;
        int host_num;
        //std::vector<std::string> host_ips;
        //std::vector<int> host_ports;
        std::vector<host_server_struct *> host_server_list;
        int total_weight;
        proxy_algorithm proxyAlgorithm;
    };
    /**
     * @def add a node to the tail of the queue
     * @param node: the node which needs to be added into the queue
     * @param nodes: the tail dummy node of a list
     */
    template <class T>
    static void add_node(T *node, T *nodes) {
        T *temp = nodes->prev;
        nodes->prev = node;
        node->next = nodes;
        node->prev = temp;
        temp->next = node;
    }

    /**
     * @def remove and return node from the head of the queue
     * @tparam T
     * @param head: the head dummy node of a list
     * @param tail: the tail dummy node of a list
     * @return: the removed node
     */
    template<class T>
    static T* remove_node(T *head, T *tail) {
        if(head->next == tail) {
            return nullptr;
        }
        T *result = head->next;
        head->next = result->next;
        head->next->prev = head;
        return result;
    }
    /**
     * @def parse the configuration file of the server
     * @param is: save the data which is fetched from config file
     * @return: 0 on success, -1 on error
     */
    static int parse_conf_file(struct init_struct *is);

};
#endif //HTTP_SERVER_UTILITY_H
