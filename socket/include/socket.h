// 这个文件定义类服务端和客户端封装类的前置类，或者叫基本类
#pragma once
#include "../../public/cpublic.h"
#include "../../threadpool/include/threadpool.h"
#include <atomic>

// 定义两种数据的类型，一种是心跳包，一种是数据包
enum class MessageType
{
    Heart, // 心跳包
    Login, // 登录
    Top,   // 文件报头
    File,  // 文件数据
    Data //字符串数据
};

// 定义基本的客户端结构体
class client
{
  public:
    int cfd;                // 通信描述符
    std::atomic<int> count; // 心跳包计数

    // 重置计数
    bool resetCount();
    // 增加计数
    bool addCount(int num = 1);
    // 获取计数
    int getCount();
};

// 用于服务端类的客户端结点
class clientNode : public client
{
  public:
    std::atomic<bool> is_active;  // 心跳包检查完已经超时的标志
    std::atomic<bool> is_working; // 是否正在工作
    clientNode(int cfd);
};

class Socket
{
  protected:
    // 设置非阻塞
    bool setnonblocking(int fd);

    // 读取n个字节，用于ET模式
    bool readn(int fd, void *buf, size_t n);

    // 写n个字节，用于ET模式
    bool sendn(int fd, void *buf, size_t n);

    // 解析消息类型
    void typeTOchar(MessageType type, char &msgType);
    void charTOtype(char msgType, MessageType &type);
};