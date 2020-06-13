//
// Created by zheyu on 5/28/20.
//

#include "ThreadPool.h"

ThreadPool::ThreadPool(int min_thread_num, int max_thread_num, int add_step) {
    this->min_thread_num = min_thread_num;
    this->max_thread_num = max_thread_num;
    this->add_step = add_step;
    this->busy_thread_num = 0;
    this->delete_thread_num = 0;
    this->tasks_num = 0;
    this->workers_num = 0;
    this->workers_head = new Worker();
    this->workers_tail = new Worker();
    this->workers_tail->prev = workers_head;
    this->workers_head->next = workers_tail;
    this->tasks_head = new Task();
    this->tasks_tail = new Task();
    this->tasks_head->next = tasks_tail;
    this->tasks_tail->prev = tasks_head;
    this->terminate = false;
    this->logger = Logger::get_instance(LOG_DIR, Log::ALL);

    //base case
    if(min_thread_num <= 0) {
        logger->add_log(new Log("Threadpool: argument error: minimal thread number: " + std::to_string(min_thread_num) + " <= 0(illegal)", Log::ERROR));
        std::cerr << "Threadpool(): argument error: minimal thread number: " << min_thread_num << " <= 0(illegal)" << std::endl;
        exit(-1);
    }
    if(min_thread_num > max_thread_num) {
        logger->add_log(new Log("Threadpool: argument error: mininal thread number > maximal thread number", Log::ERROR));
        std::cerr << "Threadpool(): argument error: mininal thread number > maximal thread number" << std::endl;
        exit(-1);
    }
    if(add_step <= 0) {
        logger->add_log(new Log("Threadpool: argument error: add step: " + std::to_string(add_step) + " <= 0(illegal)", Log::ERROR));
        std::cerr << "Threadpool(): argument error: add step: " << add_step << " <= 0(illegal)" << std::endl;
        exit(-1);
    }

    //init mutex and condition
    lock = PTHREAD_MUTEX_INITIALIZER;
    empty_queue_cond = PTHREAD_COND_INITIALIZER;
    //create worker
    for(auto i = 0; i < min_thread_num; ++i) {
        Worker *worker = new Worker(this);
        Utility::add_node(worker, workers_tail);
        workers_num++;
    }
    //create manager
    manager = new Manager(this);
}
ThreadPool::~ThreadPool() {}

void ThreadPool::add_task(Task *task) {
    pthread_mutex_lock(&lock);
    Utility::add_node(task, tasks_tail);
    tasks_num++;
    pthread_cond_signal(&empty_queue_cond);
    pthread_mutex_unlock(&lock);
    logger->add_log(new Log("ThreadPool: A new task added to the queue, total tasks in the queue: " + std::to_string(tasks_num) + ".", Log::DEBUG));
}

Worker::Worker(ThreadPool *pool) {
    int ret = -1;
    prev = nullptr;
    next = nullptr;
    ret = pthread_create(&pid, nullptr, Worker::init_worker, pool);
    if(ret != 0) {
        std::cerr << "Worker(): worker create failed" << std::endl;
        exit(-1);
    }
}

void* Worker::init_worker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    Logger *logger = Logger::get_instance(LOG_DIR, Log::ALL);
    logger->add_log(new Log("Worker: " + std::to_string(pthread_self()) + " has been created", Log::DEBUG));
    while(true) {
        pthread_mutex_lock(&pool->lock);
        if(pool->tasks_num == 0) {
            if(pool->delete_thread_num > 0) {
                break;
            }
            pthread_cond_wait(&pool->empty_queue_cond, &pool->lock);
        }
        if(pool->delete_thread_num > 0) {
            pool->delete_thread_num--;
            pthread_mutex_unlock(&pool->lock);
            break;
        }
        logger->add_log(new Log("Worker: " + std::to_string(pthread_self()) + " get one task.", Log::INFO));
        pool->busy_thread_num++;
        Task *task = Utility::remove_node(pool->tasks_head, pool->tasks_tail);
        if(task == nullptr) {
            pthread_mutex_unlock(&pool->lock);
            continue;
        }
        pool->tasks_num--;
        pthread_mutex_unlock(&pool->lock);
        task->func(task->arg);
        pool->busy_thread_num--;
        logger->add_log(new Log("Worker: " + std::to_string(pthread_self()) + " finish one task.", Log::DEBUG));
    }
    logger->add_log(new Log("Worker: " + std::to_string(pthread_self()) + " has been removed", Log::INFO));
    pthread_exit(nullptr);
}

Manager::Manager(ThreadPool *pool) {
    int ret = -1;
    ret = pthread_create(&pid, nullptr, Manager::init_manager, pool);
    if(ret != 0) {
        std::cerr << "Worker(): worker create failed" << std::endl;
        exit(-1);
    }
}

void * Manager::init_manager(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    Logger *logger = Logger::get_instance(LOG_DIR, Log::ALL);
    logger->add_log(new Log("Manager: " + std::to_string(pthread_self()) + " has been created", Log::INFO));
    while(true) {
        sleep(DEFAULT_SLEEP_TIME);
        if(pool->terminate) {
            break;
        }
        float busy_thread_num = pool->busy_thread_num;
        float workers_num = pool->workers_num;
        logger->add_log(new Log("Manager: " + std::to_string(pthread_self()) + " number of busy threads: " + std::to_string((int)busy_thread_num) + " number of total threads: " + std::to_string((int)workers_num) + ".", Log::DEBUG));
        //add thread
        if(busy_thread_num / workers_num > ADD_THREAD_RATE && workers_num + pool->add_step <= pool->max_thread_num) {
            logger->add_log(new Log("Manager: " + std::to_string(pthread_self()) + " add new threads: " + std::to_string(pool->add_step), Log::DEBUG));
            for(int i = 0; i < pool->add_step; ++i) {
                Worker *worker = new Worker(pool);
                Utility::add_node(worker, pool->workers_tail);
                pool->workers_num++;
            }
        } else if((float)pool->busy_thread_num / (float)pool->workers_num < DEL_THREAD_RATE && workers_num - pool->add_step >= pool->min_thread_num) {   //remove thread
            logger->add_log(new Log("Manager: " + std::to_string(pthread_self()) + " remove threads: " + std::to_string(pool->add_step), Log::DEBUG));
            pool->delete_thread_num += pool->add_step;
            pthread_cond_broadcast(&pool->empty_queue_cond);
        }
    }
    pthread_exit(nullptr);
}