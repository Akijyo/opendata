#pragma once
#include "../../cpublic.h"

// 信号量类，对信号量操作的封装
// 操作指南：
// 1. 初始化信号量：init(key,value,sem_flag) key为信号量的键值 value为信号量的初始值 sem_flag为信号量的标志
// 2. P操作：sem_p(value) value为P操作的值 默认为-1
// 3. V操作：sem_v(value) value为V操作的值 默认为1
// 4. 获取信号量值：getvalue()
// 5. 销毁信号量：destroy()
class mysem
{
    private:
    int semid;
    union semun //用于操作信号量的共同体
    {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    };
    short sem_flag;//信号量标志 设置为0用于生产者消费者问题 设置为SEM_UNDO用于互斥锁
    public:
    mysem():semid(-1){};//构造函数 标志-1表示未初始化
    mysem(const mysem &ms)=delete;
    mysem &operator=(const mysem &ms)=delete;
    bool init(int key,unsigned short value=1,short sem_flag=SEM_UNDO);//   初始化信号量
    
    bool sem_p(short value=-1);//P操作

    bool sem_v(short value=1);//V操作

    int getvalue();//获取信号量值

    bool destroy();//销毁信号量
};