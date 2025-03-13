#pragma once
#include "../../cpublic.h"
//这是一个循环队列，用于存储数据

#define MAX_LENGTH 10
template <class T>
class SQueue
{
private:
    bool isinited;//是否初始化
    T data[MAX_LENGTH];
    int headptr;//头指针
    int tailptr;//尾指针
    unsigned int length;
public:
    SQueue(const SQueue<T> &sq) = delete;
    SQueue<T> &operator=(const SQueue<T> &sq) = delete;
    SQueue();
    void init();
    void push(T t);
    void pop();
    unsigned int size();
    bool empty();
    bool full();
    T& front();
    void print();
    void clear();
};

template <class T>
SQueue<T>::SQueue()
{
    this->init();
}

template <class T>
void SQueue<T>::init()
{
    if(isinited!=true)
    {
        headptr=0;
        tailptr=MAX_LENGTH-1;
        length=0;
        isinited=true;
        memset(data,0,sizeof(data));
    }
}

template <class T>
void SQueue<T>::push(T t)
{
    if(length<MAX_LENGTH)
    {
        tailptr=(tailptr+1)%MAX_LENGTH;
        data[tailptr]=t;
        length++;
    }
    else
    {
        std::cout<<"Error: SQueue is full!"<<std::endl;
    }
}

template <class T>
void SQueue<T>::pop()
{
    if(length>0)
    {
        headptr=(headptr+1)%MAX_LENGTH;
        length--;
    }
    else
    {
        std::cout<<"Error: SQueue is empty!"<<std::endl;
    }
}

template<class T>
unsigned int SQueue<T>::size()
{
    return this->length;
}

template<class T>
bool SQueue<T>::empty()
{
    return this->length==0;
}

template<class T>
bool SQueue<T>::full()
{
    return this->length==MAX_LENGTH;
}

template<class T>
T& SQueue<T>::front()
{
    return data[headptr];
}

template<class T>
void SQueue<T>::print()
{
    for(unsigned int i=0;i<length;i++)
    {
        std::cout<<data[(this->headptr+i)%MAX_LENGTH]<<" ";
    }
    std::cout<<std::endl;
}

template<class T>
void SQueue<T>::clear()
{
    headptr=0;
    tailptr=MAX_LENGTH-1;
    length=0;
    memset(data,0,sizeof(data));
}