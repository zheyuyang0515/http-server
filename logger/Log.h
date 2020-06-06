//
// Created by zheyu on 6/4/20.
//

#ifndef HTTP_SERVER_LOG_H
#define HTTP_SERVER_LOG_H

#include <string>
enum Level {
    ALL, DEBUG, INFO, WARNING, ERROR, FATAL
};
class Log {
public:
    Log *prev;
    Log *next;
    std::string content;
    Level level;
public:
    Log(std::string content, Level level) {
        next = nullptr;
        prev = nullptr;
        this->content = content;
        this->level = level;
    }
    Log() {
        next = nullptr;
        prev = nullptr;
    }
};
#endif //HTTP_SERVER_LOG_H
