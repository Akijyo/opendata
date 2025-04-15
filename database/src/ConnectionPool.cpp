#include "../include/ConnectionPool.h"
#include <fstream>
#include <iostream>
#include <memory>

// 获取连接池对象,单例模式，私有化构造函数，只能通过该函数获取连接池对象，并且每次返回都是同一个地址，即同一个对象
ConnectionPool *ConnectionPool::GetConnectionPool(std::string file)
{
    static ConnectionPool pool(file);
    return &pool;
}

void ConnectionPool::AddConnection() // 添加连接的封装函数
{
    std::shared_ptr<IConnection> connection = std::make_shared<mysql>();
    connection->connect(host, user, passwd, db, port); // 连接数据库

    this->pool.push_back(connection); // 将连接放入连接池
    connection->refreshTime();        // 刷新时间
}

void ConnectionPool::ProduceConnection() // 生产连接的线程函数
{
    while (!this->stop) // 当析构函数将stop置为false时，循环结束
    {
        std::unique_lock<std::mutex> lock(this->mtx); // 加锁
        // 当连接池中的连接数大于连接池最小连接数或者连接池中的连接数加伤正使用的连接数大于设定的最大连接数，则线程阻塞
        while ((this->pool.size() >= min_size || this->pool.size() + this->busy_size > this->max_size) && !this->stop)
        {
            this->producer_cv.wait(lock);
            // 当析构时线程被唤醒，但线程恰巧在wait之后，此时stop为true，线程结束
            if (this->stop) // 当析构函数将stop置为true时，线程结束
                return;
        }
        if (this->stop) // 为了线程回收多加判断
            break;
        this->AddConnection();          // 为连接池添加一个链接
        this->consumer_cv.notify_one(); // 唤醒消费者线程
    }
}

void ConnectionPool::DestroyConnection()
{
    while (!this->stop) // 当析构函数将stop置为false时，循环结束
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 每隔0.5秒检查一次
        std::unique_lock<std::mutex> lock(this->mtx);                // 加锁
        while (this->pool.size() > this->min_size && !this->stop) // 只有当连接池中连接数量大于最小连接数时才会销毁连接
        {
            // 仅仅只需队头出列判断，因为队头的存活时长一点比后面连接的存活时长长
            std::shared_ptr<IConnection> connection = this->pool.front();
            if (connection->getAliveTime() > this->max_idle_time) // 判断连接是否超时
            {
                this->pool.pop_front();
            }
            else // 若队头连接未超时，则后面的连接也不会超时
            {
                break;
            }
        }
    }
}

// 获取连接，并且实现自定义删除器，让外部调用时无需多余操作，链接会自动返回连接池
std::shared_ptr<IConnection> ConnectionPool::getConnect()
{
    // 加锁
    std::unique_lock<std::mutex> lock(mtx);

    // 等待可用连接
    while (pool.empty() && !stop)
    {
        if (consumer_cv.wait_for(lock, std::chrono::milliseconds(time_out)) == std::cv_status::timeout)
        {
            if (pool.empty())
            {
                return nullptr;
            }
        }
    }

    if (stop)
        return nullptr;

    // 获取池中连接
    std::shared_ptr<IConnection> conn = pool.front();
    pool.pop_front();
    busy_size++;

    // 创建返回的智能指针
    // **重要**
    // 智能指针的智能指针，保证了原有连接的引用计数不会被销毁
    auto deleter = std::make_shared<std::shared_ptr<IConnection>>(conn);

    // 创建带自定义删除器的智能指针
    // 根据原始连接conn.get()创建一个新的智能指针，当外部最后一个引用被销毁时，会触发删除器
    // 由于有deleter的引用计数，原有连接不会被销毁。
    std::shared_ptr<IConnection> returnConn(conn.get(), [this, deleter](IConnection *) {
        std::unique_lock<std::mutex> lock(mtx); // 加锁
        pool.push_back(*deleter);               // 将连接放回连接池
        busy_size--;
        (*deleter)->refreshTime();
        this->consumer_cv.notify_one();
    });

    producer_cv.notify_one();
    return returnConn;
}

ConnectionPool::ConnectionPool(std::string file) // 初始化连接池
{
    // 解析json配置文件
    nlohmann::json config;
    std::ifstream config_file(file);
    if (config_file.is_open())
    {
        config_file >> config;
        host = config["host"];
        user = config["user"];
        passwd = config["passwd"];
        db = config["db"];
        port = config["port"];
        max_size = config["max_size"];
        min_size = config["min_size"];
        time_out = config["time_out"];
        max_idle_time = config["max_idle_time"];
    }
    else
    {
        std::cerr << "无法打开配置文件" << std::endl;
        exit(1);
    }
    stop = false;
    for (unsigned int i = 0; i < min_size; i++)
    {
        AddConnection();
    }
    produce_thread = std::thread(&ConnectionPool::ProduceConnection, this); // 创建生产者线程
    destroy_thread = std::thread(&ConnectionPool::DestroyConnection, this); // 创建消费者线程
    //********这里不使用线程分离，线程分离后程序会出现无法停止的情况（未解决）
}

ConnectionPool::~ConnectionPool()
{
    //std::cout << "析构函数开始" << std::endl;
    stop = true;
    producer_cv.notify_all(); // 唤醒生产者线程,防止生产者线程阻塞无法回收
    consumer_cv.notify_all(); // 唤醒消费者线程,防止消费者线程阻塞无法回收
    if (produce_thread.joinable())
    {
        produce_thread.join(); // 回收生产者线程
    }
    if (destroy_thread.joinable())
    {
        destroy_thread.join(); // 回收消费者线程
    }
    while (!pool.empty()) // 回收连接池中的连接
    {
        std::shared_ptr<IConnection> connection = pool.front();
        pool.pop_front();
    }
    //std::cout << "析构函数结束" << std::endl;
}