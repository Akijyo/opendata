#pragma once
#include "../../public/cpublic.h"
#include "../../threadpool/include/threadpool.h"

// 定义两种数据的类型，一种是心跳包，一种是数据包
enum class MessageType
{
    Heart, // 心跳包
    Data   // 数据包
};
class TcpServer;

// 定义连接接受到的客户端的结构体，包含通信描述符，计数（心跳包判断），互斥锁，是否运行（心跳包超时）
class clientNode
{
  public:
    int cfd;                      // 通信描述符
    std::atomic<int> count;       // 心跳包计数
    std::atomic<bool> is_active;  // 心跳包检查完已经超时的标志
    std::atomic<bool> is_working; // 是否正在工作

    clientNode(int cfd)
    {
        this->cfd = cfd;
        this->count = 0;
        this->is_active = true;
        this->is_working = false;
    }

    bool resetCount() // 重置计数
    {
        this->count = 0;
        return true;
    }
    bool addCount(int num = 1) // 增加计数
    {
        this->count += num;
        return true;
    }
    int getCount() // 获取计数
    {
        return this->count;
    }
};

class TcpServer
{
  private:
    int fd;                                                          // 监听描述符
    std::unordered_map<int, std::shared_ptr<clientNode>> client_map; // 客户端结构体map
    std::shared_mutex clientMap_mtx;                                 // 客户端结构体map的读写锁
    int epfd;                                                        // epoll描述符

    // 读取n个字节，用于ET模式
    bool readn(int fd, void *buf, size_t n);

    // 写n个字节，用于ET模式
    bool sendn(int fd, void *buf, size_t n);

  public:
    TcpServer();                            // 构造函数,仅仅构造一个套接字文件描述符
    ~TcpServer();                           // 析构函数,关闭套接字文件描述符
    bool createListen(unsigned short port); // 创建监听,并讲客户端结点加入到clients数组中
    bool epollSocket(ThreadPool *pool, std::function<void *(std::shared_ptr<clientNode>)> callback); // epoll监听
    bool acceptConn();                                  // 接受连接，返回一个通信描述符的智能指针，可用于多线程
    bool closeConn(std::shared_ptr<clientNode> client); // 关闭连接
    // 心跳包检查,默认每5秒检查一次
    bool heartbeatThread(const int time = 5);
    // 发送消息，带消息类型
    bool sendMsgWithType(std::shared_ptr<clientNode> client, std::string msg, MessageType type = MessageType::Data);
    // 接收消息，带消息类型
    bool recvMsgWithType(std::string &msg, std::shared_ptr<clientNode> client, MessageType &type);

    // 发送二进制数据
    bool sendMsgBin(std::shared_ptr<clientNode> client, void *buffer, size_t n, MessageType type = MessageType::Data);

    // 接收二进制数据
    bool recvMsgBin(void *buffer, std::shared_ptr<clientNode> client, MessageType &type);
};