#pragma once
#include "socket.h"

class TcpServer : public Socket
{
  private:
    int fd;                                                          // 监听描述符
    int epfd;                                                        // epoll描述符
    std::unordered_map<int, std::shared_ptr<clientNode>> client_map; // 客户端结构体map
    std::shared_mutex clientMap_mtx;                                 // 客户端结构体map的读写锁
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

    // 接受消息，二进制，同时提供获取大小的参数
    bool recvMsgBin(void *buffer, std::shared_ptr<clientNode> client, MessageType &type, size_t &size);
};