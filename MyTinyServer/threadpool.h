#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<pthread.h>
#include <list>
#include <exception>
#include "locker.h"
#include <cstdio>



//线程池类，定义成模板类，为了代码复用，模板参数T是任务类
template<typename T>
class threadpool{
public:
    threadpool(int thread_number = 8,int max_requests = 10000);
    ~threadpool();
    //添加任务的方法
    bool append(T* request);
private:
    //静态函数，怎么访问非静态成员？
    static void* worker(void * arg);
    void run(); //线程池启动

private:
    //线程数量
    int m_thread_number;

    //线程池数组，大小位m_thread_number
    pthread_t *m_threads;

    //请求队列中最多允许的,等待处理请求数量
    int m_max_requests;

    //请求队列
    std::list<T*>m_workqueue;

    //互斥锁
    locker m_queuelocker;

    //信号量，判断是否有任务需要处理
    sem m_queuestat;

    //是否结束线程
    bool m_stop;
};

template<typename T>
threadpool<T>::threadpool(int thread_number,int max_requests):
    m_thread_number(thread_number),m_max_requests(max_requests),
    m_stop(false),m_threads(NULL){
    if((thread_number<=0)||(m_max_requests<=0)){
        throw std::exception();
    }
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads){
        throw std::exception();
    }

    //创建thread_number个线程，并将他们设置为线程脱离（自己释放资源）
    for(int i = 0 ;i < m_thread_number;i++){
        printf("create the %dth thread\n",i);
        if(pthread_create(m_threads + i,NULL,worker,this) !=0){
            delete []m_threads;
            throw std::exception();
        }
        if(pthread_detach(m_threads[i])){
            delete []m_threads;
            throw std::exception();
        }
    }
}

//析构函数
template<typename T>
threadpool<T>::~threadpool(){
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request){
    // 操作工作队列时一定要加锁，因为它被所有线程共享。
    m_queuelocker.lock();
    if(m_workqueue.size() > m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post(); //信号量加1，队列中取数据，根据信号量判断是阻塞还是运行
    return true;
}

template <typename T>
void * threadpool<T>::worker(void* arg){
    threadpool* pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run(){
    while(!m_stop){
        m_queuestat.wait();
        m_queuelocker.lock();
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        //有数据
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();

        if(!request){
            continue;
        }
        //做任务
        request->process();
    }
}
#endif 