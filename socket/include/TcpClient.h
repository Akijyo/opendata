#pragma once
#include "socket.h"

class TcpClient final : public client, public Socket
{
  private:
    using client::cfd;
    using client::count;

  public:
    // 构造函数,创建一个通信套接字
    TcpClient();
    // 析构函数，关闭套接字释放资源
    ~TcpClient();
    // 连接服务器
    bool connectServer(std::string ip, unsigned short port);
    // 发送消息，带消息类型
    bool sendMsgWithType(std::string msg, MessageType type = MessageType::Data);
    // 接收消息，带消息类型
    bool recvMsgWithType(std::string &msg, MessageType &type);
    // 发送消息，二进制
    bool sendMsgBin(void *msg, size_t n, MessageType type = MessageType::Data);
    // 接收消息，二进制
    bool recvMsgBin(void *buffer, MessageType &type);
    // 接受消息，二进制，同时提供获取大小的参数
    bool recvMsgBin(void *buffer, MessageType &type, size_t &size);
};