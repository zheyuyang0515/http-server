//
// Created by zheyu on 5/31/20.
//

#ifndef THREADPOOL_LOGGER_H
#define THREADPOOL_LOGGER_H
#include <glog/logging.h>
#include <string>
#define EXECUTABLE_NAME "ThreadPool"
#define LOG_DIR "../logs/"
class Logger {
private:
    //Logger();
    Logger(std::string dir, std::string executable_name) {
        this->dir = dir;
        this->executable_name = executable_name;
        std::string info_dir = dir + "info_log";
        std::cout << info_dir << std::endl;
        google::SetLogDestination(google::GLOG_INFO, info_dir.c_str());
        std::string warning_dir = dir + "warning_log";
        google::SetLogDestination(google::GLOG_WARNING, warning_dir.c_str());
        std::string error_dir = dir + "error_log";
        google::SetLogDestination(google::GLOG_ERROR, error_dir.c_str());
        std::string fatal_dir = dir + "fatal_log";
        google::SetLogDestination(google::GLOG_FATAL, fatal_dir.c_str());
        google::InitGoogleLogging(executable_name.c_str());
    };
    ~Logger() {
        google::ShutdownGoogleLogging();
    };
public:
    std::string dir;
    std::string executable_name;
    static Logger* get_instance() {
        static Logger logger(LOG_DIR, EXECUTABLE_NAME);
        return &logger;
    }
    void logging_info(std::string content) {
        LOG(INFO) << content;
    }
    void logging_warning(std::string content) {
        LOG(WARNING) << content;
    }
    void logging_error(std::string content) {
        LOG(ERROR) << content;
    }
    void logging_fatal(std::string content) {
        LOG(FATAL) << content;
    }
};
#endif //THREADPOOL_LOGGER_H
