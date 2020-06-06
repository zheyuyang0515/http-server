//
// Created by zheyu on 6/4/20.
//
/**
 * Logger class:
 */
#ifndef HTTP_SERVER_LOGGER_H
#define HTTP_SERVER_LOGGER_H


#include <string>
#include "Log.h"
#include "../Util/Utility.h"
#include <fstream>
#include <unistd.h>
#include <time.h>
#include <iostream>

class Logger {
private:
    std::string dir;
    Level level;
    int log_sum;
    Log *dummy;
    Log *tail;
    static void* init_logger(void *arg);
    Logger(std::string dir, Level level) {
        this->dummy = new Log();
        this->tail = new Log();
        this->dummy->next = tail;
        this->tail->prev = dummy;
        this->dir = dir;
        this->level = level;
        this->log_mutex = PTHREAD_MUTEX_INITIALIZER;
        this->log_cond = PTHREAD_COND_INITIALIZER;
        this->log_sum = 0;
        this->terminate = false;
        pthread_t pid;
        int ret = pthread_create(&pid, nullptr, Logger::init_logger, this);
        if(ret == -1) {
            perror("Logger: logger create failed");
            exit(-1);
        }
    };
public:
    pthread_mutex_t log_mutex;
    pthread_cond_t log_cond;
    bool terminate;
    /**
     * @def singleton get_instance function. The arguments are only valid when first call this function.
     * @param dir: log file directory
     * @param level: ignore the severity which equals or lower than this argument
     * @return logger instance pointer
     */
    static Logger* get_instance(std::string dir, Level level) {
        static Logger logger(dir, level);
        return &logger;
    }
    /**
     *
     * @param log: add log to the queue
     */
    void add_log(Log *log);
    /**
     * remove one log from the queue
     * @return: the head of the queue
     */
    Log* remove_log();

    /**
     * @def get current time stamp
     */
    static char* get_cur_time() {
        time_t timet;
        char *cur_time = new char[20];
        struct tm time_struct;
        time(&timet);
        time_struct = *localtime(&timet);
        strftime(cur_time, 20, "%Y-%m-%d %H:%M:%S", &time_struct);
        return cur_time;
    }



};


#endif //HTTP_SERVER_LOGGER_H
