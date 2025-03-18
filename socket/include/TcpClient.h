#pragma once
#include "../../public/cpublic.h"
#include "../../threadpool/include/threadpool.h"
#include <atomic>
#include <string>

enum class MessageType
{
    Heart,
    Data
};

class TcpClient
{
  private:
    int cfd;
    std::atomic<int> count;

  public:
    TcpClient();                                                                  // 构造函数,创建一个通信套接字
    ~TcpClient();                                                                 // 析构函数
    bool connectServer(std::string ip, unsigned short port);                      // 连接服务器
    bool sendMsgWithType(std::string msg, MessageType type = MessageType::Data); // 发送消息，带消息类型
    bool recvMsgWithType(std::string &msg, MessageType &type);                    // 接收消息，带消息类型
    bool sendMsgBin(void *msg, size_t n, MessageType type = MessageType::Data);   // 发送消息，二进制
    bool recvMsgBin(void *buffer, MessageType &type);                            // 接收消息，二进制
    bool resetCount();                                                            // 重置计数
    bool addCount(int num = 1);                                                   // 增加计数
    int getCount();                                                               // 获取计数
};
