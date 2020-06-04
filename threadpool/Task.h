//
// Created by zheyu on 5/28/20.
//
/**
 * Class Task: the class of task which needs to be handled by the thread pool
 */
#ifndef THREAD_POOL_TASK_H
#define THREAD_POOL_TASK_H
class Task {
public:
    Task *prev;
    Task *next;
    void *arg;   //argument of the callback function
    void (*func)(void *arg);   //callback function
public:
    Task(void (*func)(void *), void *arg);
    /**
     * constructor to initiate dummy nodes
     */
    Task() {
        prev = nullptr;
        next = nullptr;
    }
};


#endif //THREAD_POOL_TASK_H
