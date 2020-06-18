//
// Created by zheyu on 6/17/20.
//
#include "Utility.h"

int Utility::parse_conf_file(struct init_struct *is) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *root = nullptr;
    tinyxml2::XMLElement *element = nullptr;
    tinyxml2::XMLElement *children = nullptr;
    int ret = -1;
    ret = doc.LoadFile(CONF_DIR);
    if(ret != 0) {
        perror("Open config file failed, please check if it is exist");
        return -1;
    }
    root = doc.RootElement();
    if(root == nullptr) {
        std::cerr << "Config file format error: could not find 'server' node" << std::endl;
        return -1;
    }
    //parse general
    element = root->FirstChildElement("general");
    if(element == nullptr) {
        std::cerr << "Config file format error: could not find 'general' node" << std::endl;
        return -1;
    }
    //ip
    children = element->FirstChildElement("listen-ip");
    if(children == nullptr) {
        std::cerr << "Config file format error: could not find 'listen-ip' node" << std::endl;
        return -1;
    } else {
        is->ip = children->GetText();
    }
    //port
    children = element->FirstChildElement("listen-port");
    if(children == nullptr) {
        std::cerr << "Config file format error: could not find 'listen-port' node" << std::endl;
        return -1;
    } else {
        is->port = std::stoi(children->GetText());
    }
    //max-connection
    children = element->FirstChildElement("max-connection");
    if(children == nullptr) {
        is->max_connection = DEFAULT_MAX_CONNECTION;
        std::cerr << "Config file format warning: could not find 'max-connection' node, set it to " << DEFAULT_MAX_CONNECTION << " automatically."<< std::endl;
    } else {
        is->max_connection = std::stoi(children->GetText());
    }

    //max-request
    children = element->FirstChildElement("max-request");
    if(children == nullptr) {
        is->max_request = DEFAULT_MAX_REQUEST;
        std::cerr << "Config file format warning: could not find 'max-request' node, set it to " << DEFAULT_MAX_REQUEST << " automatically."<< std::endl;
    } else {
        is->max_request = std::stoi(children->GetText());
    }

    //worker
    element = root->FirstChildElement("worker");
    if(element == nullptr) {
        std::cerr << "Config file format error: could not find 'worker' node" << std::endl;
        return -1;
    }
    //min-worker
    children = element->FirstChildElement("min-worker");
    if(children != nullptr && std::stoi(children->GetText()) < 1) {
        std::cerr << "Config file format warning: number of minimal worker can not be assigned to less than 1, set it to 1 automatically" << std::endl;
    }
    is->min_worker = children == nullptr ? DEFAULT_MIN_WORKER : std::stoi(children->GetText());
    //max-worker
    children = element->FirstChildElement("max-worker");
    if(children == nullptr) {
        int cpu_core = get_nprocs_conf();
        if(cpu_core < 1) {
            cpu_core = 1;
        }
        is->max_worker = cpu_core;
        std::cerr << "Config file format warning: number of maximal worker can not be empty, set it to the number of your device's cpu logic core: " << cpu_core << "automatically" << std::endl;
    } else {
        if(std::stoi(children->GetText()) < is->min_worker) {
            std::cerr << "Config file format warning: number of maximal worker: " << children->GetText() << "can not be less than the number of minimal worker: " << is->min_worker << ", set it to " << is->min_worker << "automatically" << std::endl;
            is->max_worker = is->min_worker;
        } else {
            is->max_worker = std::stoi(children->GetText());
        }
    }
    //add-step
    children = element->FirstChildElement("add-step");
    if(children == nullptr) {
        is->add_step = 1;
        std::cerr << "Config file format warning: could not find 'add-step' node, set it to 1 automatically" << std::endl;
    } else {
        is->add_step = std::stoi(children->GetText());
    }
    //adjust-time
    children = element->FirstChildElement("adjust-time");
    if(children == nullptr || std::stoi(children->GetText()) < DEFAULT_ADJUST_TIME) {
        is->adjust_time = DEFAULT_ADJUST_TIME;
        std::cerr << "Config file format warning: could not find 'adjust-time' node or adjust time less than "<< DEFAULT_ADJUST_TIME <<" seconds, set it to" << DEFAULT_ADJUST_TIME << "automatically" << std::endl;
    } else {
        is->adjust_time = std::stoi(children->GetText());
    }
    //add-worker-rate
    children = element->FirstChildElement("add-worker-rate");
    if(children == nullptr || std::stof(children->GetText()) > 1) {
        is->add_worker_rate = DEFAULT_ADD_WORKER_RATE;
        std::cerr << "Config file format warning: could not find 'add_worker_rate' node or add worker rate larger than 1, set it to" << DEFAULT_ADD_WORKER_RATE << "automatically" << std::endl;
    } else {
        is->add_worker_rate = std::stof(children->GetText());
    }
    //delete-worker-rate
    children = element->FirstChildElement("delete-worker-rate");
    if(children == nullptr || std::stof(children->GetText()) <= 0) {
        is->add_worker_rate = DEFAULT_DELETE_WORKER_RATE;
        std::cerr << "Config file format warning: could not find 'delete-worker-rate' node or delete worker rate less or equal than 0, set it to" << DEFAULT_DELETE_WORKER_RATE << "automatically" << std::endl;
    } else {
        is->add_worker_rate = std::stof(children->GetText());
    }
    //page
    element = root->FirstChildElement("page");
    if(element == nullptr) {
        std::cerr << "Config file format error: could not find 'page' node" << std::endl;
        return -1;
    }
    //main-page
    children = element->FirstChildElement("main-page");
    if(children == nullptr) {
        is->main_page = DEFAULT_MAIN_PAGE;
        std::cerr << "Config file format warning: could not find 'main-page' node, set it to " << DEFAULT_MAIN_PAGE << " automatically." << std::endl;
    } else {
        is->main_page = children->GetText();
    }
    //error-404-page
    children = element->FirstChildElement("main-page");
    if(children == nullptr) {
        is->error_404_page = DEFAULT_ERROR_404_PAGE;
        std::cerr << "Config file format warning: could not find 'error-404-page' node, set it to " << DEFAULT_ERROR_404_PAGE << " automatically." << std::endl;
    } else {
        is->error_404_page = children->GetText();
    }
    //log
    element = root->FirstChildElement("log");
    if(element == nullptr) {
        std::cerr << "Config file format error: could not find 'log' node" << std::endl;
        return -1;
    }
    //severity-ignore
    children = element->FirstChildElement("severity-ignore");
    if(children == nullptr) {
        is->severity_ignore = DEFAULT_SERVERITY_IGNORE;
        std::cerr << "Config file format warning: could not find 'severity-ignore' node, set it to " << DEFAULT_SERVERITY_IGNORE << " automatically." << std::endl;
    } else {
        if(std::stoi(children->GetText()) > Log::FATAL || std::stoi(children->GetText()) < Log::ALL) {
            is->severity_ignore = DEFAULT_SERVERITY_IGNORE;
            std::cerr << "Config file format warning: 'severity-ignore' node value error, set it to " << DEFAULT_SERVERITY_IGNORE << " automatically." << std::endl;
        }
        is->severity_ignore = Log::Level(std::stoi(children->GetText()));
    }
}
