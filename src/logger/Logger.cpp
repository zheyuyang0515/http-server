//
// Created by zheyu on 6/4/20.
//

#include "Logger.h"

void Logger::add_log(Log *log) {
    if(log->level <= level) return;
    pthread_mutex_lock(&log_mutex);
    Utility::add_node(log, tail);
    log_sum++;
    pthread_cond_signal(&log_cond);
    pthread_mutex_unlock(&log_mutex);
}

Log* Logger::remove_log() {
    return Utility::remove_node(dummy, tail);
}

void* Logger::init_logger(void *arg){
    Logger *logger = (Logger *)arg;
    std::ofstream out;
    //open log file
    out.open(logger->dir, std::ios::out | std::ios::app);
    if(!out) {
        perror("Open log file failed, please make sure have 'logs' directory in the workspace");
        exit(-1);
    }
    //used to convert enum to string
    std::string levels[] = {"", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
    while(true) {
        pthread_mutex_lock(&logger->log_mutex);
        if(logger->log_sum == 0) {
            //recv terminate signal
            if(logger->terminate) {
                pthread_mutex_unlock(&logger->log_mutex);
                break;
            }
            //wait for condition
            pthread_cond_wait(&logger->log_cond, &logger->log_mutex);
        }
        //recv terminate signal
        if(logger->terminate) {
            pthread_mutex_unlock(&logger->log_mutex);
            break;
        }
        if(logger->log_sum > 0) {
            //write log
            Log *log = logger->remove_log();
            //convert time
            char* cur_time = get_cur_time();
            out << "[" << cur_time << " " << levels[log->level] << "] " << log->content << std::endl;
            logger->log_sum--;
            delete cur_time;
            delete log;
        }
        pthread_mutex_unlock(&logger->log_mutex);
    }
    out.close();
    pthread_exit(nullptr);
}

