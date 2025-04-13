#pragma once
#include <iostream>
#include <deque>
#include <mysql/mysql.h>
#include "Iconnection.h"
#include "msql.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include "nlohmann/json.hpp"
class ConnectionPool
{
  private:
    ConnectionPool(std::string &file); // 私有化构造函数,禁止外部创建对象,需要传入数据库的配置文件路径

    std::string host;                    // 主机地址
    std::string user;                    // 用户名
    std::string passwd;                  // 密码
    std::string db;                      // 数据库
    unsigned int port;                   // 端口号
    unsigned int max_size;               // 连接池最大连接数
    unsigned int min_size;               // 连接池最小连接数
    std::atomic<unsigned int> busy_size; // 当前连接数
    int time_out;                        // 超时时间
    long long max_idle_time;             // 最大空闲时间
    std::mutex mtx;// 互斥锁
    std::condition_variable producer_cv;// 生产者条件变量
    std::condition_variable consumer_cv;// 消费者条件变量
    std::deque<std::shared_ptr<IConnection>> pool;// 连接池,使用队列存放mysql连接
    std::atomic<bool> stop;// 停止标志,stop为false时,生产者线程和消费者线程继续运行，stop为true时，生产者线程和消费者线程退出
    std::thread produce_thread;// 生产者线程，线程一直持续直到连接池析构
    std::thread destroy_thread;// 消费者线程，线程一直持续直到连接池析构
    void ProduceConnection(); // 生产连接的线程函数
    void DestroyConnection(); // 销毁连接的线程函数
    void AddConnection();     // 添加连接的封装函数
public:
    ~ConnectionPool(); // 析构函数
    ConnectionPool(const ConnectionPool &) = delete;
    ConnectionPool &operator=(const ConnectionPool &) = delete;
    static ConnectionPool *GetConnectionPool(std::string &file); // 获取连接池对象,构造单例模式用的
    std::shared_ptr<IConnection> getConnect();          // 获取连接
};
