//
// Created by zheyu on 5/28/20.
//

#ifndef THREAD_POOL_THREADPOOL_H
#define THREAD_POOL_THREADPOOL_H
#include <iostream>
#include <pthread.h>
#include "Task.h"
#include <unistd.h>
#include "../logger/Logger.h"
#include "../util/Utility.h"
class ThreadPool;
/**
 * Class Worker: the class of thread in the thread pool
 */
class Worker {
private:
    pthread_t pid;
private:
    /**
     * pthread callback
     * @param arg
     * @return
     */
    static void *init_worker(void * arg);

public:
    Worker *prev;   //prev pointer
    Worker *next;   //next pointer
public:
    Worker(ThreadPool *pool);
    /**
     * constructor to initiate dummy node
     */
    Worker() {
        prev = nullptr;
        next = nullptr;
    };
    ~Worker(){};
};
/**
 * Class Manager: the of manager thread which could monitor and modify the number of workers in the thread pool
 */
class Manager {
private:
    pthread_t pid;
private:
    /**
     * pthread callback
     * @param arg
     * @return
     */
    static void *init_manager(void * arg);
public:
    Manager(ThreadPool *pool);
    ~Manager(){};
};
/**
 * Class ThreadPool: the class of the thread pool
 */
class ThreadPool {
private:
    Manager *manager;       //manager thread
    Logger *logger;
public:
    pthread_mutex_t lock;       //mutex
    pthread_mutex_t thread_lock;        //manipulate number of thread
    pthread_cond_t empty_queue_cond;    //condition
    Task *tasks_head;     //task_head dummy queue
    Task *tasks_tail;        //task_tail dummy queue
    Worker *workers_head;     //worker_head dummy queue(threads)
    Worker *workers_tail;     //worker_tail dummy queue(threads)
    int tasks_num, workers_num;        //queue size of tasks and workers
    int min_thread_num, max_thread_num, add_step;
    int delete_thread_num;      //number of threads need to be deleted at present
    int busy_thread_num;        //number of busy thread
    int adjust_time;            //indicate the time interval when the manager part will reassign the number of thread in the thread pool
    bool terminate;             //indicate if the threadpool needs to be terminated
    float add_worker_rate;      //indicate the rate in decimal, the manager will add workers into the thread pool if the number of busy workers reaches this rate.
    float delete_worker_rate;   //indicate the rate in decimal, the manager will delete workers into the thread pool if the number of busy workers reaches this rate.
public:

    /**
     * add a task into the queue
     * @param task: the task which needs to be added into the queue
     */
    void add_task(Task *task);

public:
    /**
     * @param min_thread_num: the minimal number of threads in the pool
     * @param max_thread_num: the maximal number of threads int the pool
     * @param add_step: the number of threads will be modified(add/delete) in each adjustment period
     * @param adjust_time: indicate the time interval when the manager part will reassign the number of thread in the thread pool
     * @param add_worker_rate: indicate the rate in decimal, the manager will add workers into the thread pool if the number of busy workers reaches this rate.
     * @param delete_worker_rate: indicate the rate in decimal, the manager will delete workers into the thread pool if the number of busy workers reaches this rate.
     */
    ThreadPool(int min_thread_num, int max_thread_num, int add_step, int adjust_time, float add_worker_rate, float delete_worker_rate);
    ~ThreadPool();
};



#endif //THREAD_POOL_THREADPOOL_H
