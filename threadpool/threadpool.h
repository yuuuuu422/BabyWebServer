//
// Created by theoyu on 5/1/22.
//

#ifndef BABYWEBSERVER_THREADPOOL_H
#define BABYWEBSERVER_THREADPOOL_H

#include "../lock/locker.h"
#include "list"
#include "stdio.h"

template<typename T>
class threadpool{
public:
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    //添加任务
    bool append(T* request);

private:
    //子线程执行的代码，静态函数
    static void* worker(void* arg);
    void run();

private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    bool m_stop;                //是否结束线程
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests):m_thread_number(thread_number), m_max_requests(max_requests),m_stop(false), m_threads(NULL) {
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads) {
        throw std::exception();
    }
    for (int i = 0; i < thread_number; ++i)
    {
        printf("create the %dth thread\n",i+1);
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template< typename T >
threadpool< T >::~threadpool() {
    delete [] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T *request) {
    m_queuelocker.lock();
    if(m_workqueue.size()>m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    //进入队列
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    //信号量++
    m_queuestat.post();
    return true;
}
template<typename T>
void* threadpool<T>::worker( void* arg ){
    //static调用不了非static熟悉，需要拿到threadpool对象
    threadpool* pool = ( threadpool* )arg;
    pool->run();
    return pool;
}

template< typename T >
void threadpool< T >::run() {
    while (!m_stop) {
        //信号量--
        m_queuestat.wait();
        m_queuelocker.lock();
        if ( m_workqueue.empty() ) {
            m_queuelocker.unlock();
            continue;
        }
        //从消息队列中拿出来
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if ( !request ) {
            continue;
        }
        request->process();
    }

}
#endif //BABYWEBSERVER_THREADPOLL_H
