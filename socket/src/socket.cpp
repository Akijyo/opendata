#include "../include/socket.h"
using namespace std;

// 重置计数
bool client::resetCount()
{
    this->count = 0;
    return true;
}

// 增加计数
bool client::addCount(int num)
{
    this->count += num; // 默认增加计数为1
    return true;
}

// 获取计数
int client::getCount()
{
    return this->count;
}

clientNode::clientNode(int cfd)
{
    this->cfd = cfd;
    this->count = 0;
    this->is_active = true;
    this->is_working = false;
}

// 设置非阻塞
bool Socket::setnonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        return false;
    }
    return true;
}

/**
 * @brief 读取n个字节，用于ET模式，是一个辅助函数，与上面的recvMsgWithType配合使用
 * 下面三个参数和recv这个linux库函数一致
 * @param fd 文件描述符
 * @param buffer 读取缓冲区
 * @param n 读取字节数
 * @return true
 * @return false
 */
bool Socket::readn(int fd, void *buffer, size_t n)
{
    char *ptr = static_cast<char *>(buffer);
    size_t total = 0;
    while (total < n)
    {
        ssize_t ret = recv(fd, ptr + total, n - total, 0);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
        }
        else if (ret == 0)
        {
            return false;
        }
        total += ret;
    }
    return true;
}

/**
 * @brief 写n个字节，用于ET模式，是一个辅助函数，与上面的sendMsgWithType配合使用
 * 下面三个参数和send这个linux库函数一致
 * @param fd 文件描述符
 * @param buffer 写入缓冲区
 * @param n 写入字节数
 * @return true
 * @return false
 */
bool Socket::sendn(int fd, void *buffer, size_t n)
{
    char *ptr = static_cast<char *>(buffer);
    size_t total = 0;
    while (total < n)
    {
        ssize_t ret = send(fd, ptr + total, n - total, 0);
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            if (errno == EINTR)
            {
                continue;
            }
            cout << "发送消息失败" << endl;
            return false;
        }
        total += ret;
    }
    return true;
}

void Socket::typeTOchar(MessageType type, char &msgType)
{
    if (type == MessageType::Heart)
    {
        msgType = 'H';
    }
    else if (type == MessageType::Login)
    {
        msgType = 'L';
    }
    else if (type == MessageType::Top)
    {
        msgType = 'T';
    }
    else
    {
        msgType = 'D';
    }
}

void Socket::charTOtype(char msgType, MessageType &type)
{
    if (msgType == 'H')
    {
        type = MessageType::Heart;
    }
    else if (msgType == 'L')
    {
        type = MessageType::Login;
    }
    else if (msgType == 'T')
    {
        type = MessageType::Top;
    }
    else
    {
        type = MessageType::Data;
    }
}