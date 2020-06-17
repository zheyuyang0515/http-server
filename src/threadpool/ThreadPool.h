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
#define DEFAULT_SLEEP_TIME 10       //adjust the number of worker every this seconds
#define ADD_THREAD_RATE 0.8         //add new threads into the pool when (the number of busy threads in the pool) / (the number of threads in the pool) >= this rate
#define DEL_THREAD_RATE 0.3         //remove threads from the pool when(the number of busy threads in the pool) / (the number of threads in the pool) <= this rate
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
    bool terminate;             //indicate if the threadpool needs to be terminated

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
     */
    ThreadPool(int min_thread_num, int max_thread_num, int add_step);
    ~ThreadPool();
};



#endif //THREAD_POOL_THREADPOOL_H
