//
// Created by zheyu on 5/29/20.
//

#include "Task.h"

Task::Task(void (*func)(void * arg), void *arg) {
    prev = nullptr;
    next = nullptr;
    this->func = func;
    this->arg = arg;
}

