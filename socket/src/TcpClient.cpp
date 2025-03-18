#include "../include/TcpClient.h"

using namespace std;

TcpClient::TcpClient()
{
    this->cfd = socket(AF_INET, SOCK_STREAM, 0); // 创建通信套接字
    if (this->cfd == -1)
    {
        cout << "创建通信套接字失败" << endl;
    }
    // 忽略 SIGPIPE 信号
    signal(SIGPIPE, SIG_IGN);
    //******send函数的SIGPIPE信号处理，对这个信号忽略，否则下面调用send发送函数时因为
    //******服务器端关闭连接，会产生SIGPIPE信号，导致主程序退出

    this->count = 0;
}

TcpClient::~TcpClient()
{
    if (this->cfd != -1)
    {
        close(this->cfd); // 关闭通信套接字
    }
}

bool TcpClient::connectServer(string ip, unsigned short port)
{
    // 1.创建通信套接字
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &saddr.sin_addr.s_addr);

    // 2.连接服务器
    int ret;
    ret = connect(this->cfd, (sockaddr *)&saddr, sizeof(saddr));
    if (ret == -1)
    {
        cout << "连接失败" << endl;
        return false;
    }
    cout << "连接成功" << endl;
    return true;
}

/**
 * @brief 发送消息，带消息类型，解决粘包问题
 *
 * @param msg 发送的消息
 * @param type 发送的消息类型
 * @return true
 * @return false
 */
bool TcpClient::sendMsgWithType(string msg, MessageType type)
{
    // 1.小端转大端，长度为信息大小+信息类型的大小
    int len = htonl(msg.size() + 1);

    // 2.获取消息类型
    char msgType = (type == MessageType::Heart) ? 'H' : 'D';

    // 3.定义完整的发送缓冲区
    string buffer;
    buffer.reserve(sizeof(len) + sizeof(msgType) + msg.size());

    // 4.将消息长度、消息类型、消息内容依次添加到缓冲区
    buffer.append((char *)&len, sizeof(len));
    buffer.push_back(msgType);
    buffer.append(msg);

    // 5.发送消息
    if (send(this->cfd, buffer.c_str(), buffer.size(), 0) < 0)
    {
        cout << "发送消息失败" << endl;
        return false;
    }
    return true;
}

/**
 * @brief 接收消息，带消息类型，解决粘包问题
 *
 * @param type 消息类型
 * @return string
 */
bool TcpClient::recvMsgWithType(string &msg, MessageType &type)
{
    // 1.接收消息长度
    int len;
    int ret = recv(this->cfd, &len, sizeof(len), 0);
    if (ret < 0)
    {
        cout << "接收消息长度失败" << endl;
        return false;
    }
    else if (ret == 0)
    {
        cout << "服务器断开连接" << endl;
        return false;
    }
    // 2.将获取的长度大端转小端
    len = ntohl(len);

    // 3.接收消息类型
    char msgType;
    ret = recv(this->cfd, &msgType, sizeof(msgType), 0);
    if (ret < 0)
    {
        cout << "接收消息类型失败" << endl;
        return false;
    }
    else if (ret == 0)
    {
        cout << "服务器断开连接" << endl;
        return false;
    }
    type = (msgType == 'H') ? MessageType::Heart : MessageType::Data;

    // 4.接收消息内容，在此只接收len-1个字节，这个-1减去前面接收的消息类型，并且在此解决tcp粘包问题
    msg.resize(len - 1);
    ret = recv(this->cfd, &msg[0], len - 1, 0);
    if (ret < 0)
    {
        cout << "接收消息失败" << endl;
        return false;
    }
    else if (ret == 0)
    {
        cout << "服务器断开连接" << endl;
        return false;
    }
    return true;
}

/**
 * @brief 发送消息，二进制
 *
 * @param msg 发送的消息
 * @param n 发送的字节数
 * @param type 发送的消息类型
 * @return true
 * @return false
 */
bool TcpClient::sendMsgBin(void *msg, size_t n, MessageType type)
{
    // 1.小端转大端，长度为信息大小+信息类型的大小
    int len = htonl(n + 1);

    // 2.获取消息类型
    char msgType = (type == MessageType::Heart) ? 'H' : 'D';

    // 3.定义完整的发送缓冲区
    string buffer;
    buffer.reserve(sizeof(len) + sizeof(msgType) + n);

    // 4.将消息长度、消息类型、消息内容依次添加到缓冲区
    buffer.append((char *)&len, sizeof(len));
    buffer.push_back(msgType);
    buffer.append((char *)msg, n);

    // 5.发送消息
    if (send(this->cfd, buffer.c_str(), buffer.size(), 0) < 0)
    {
        cout << "发送消息失败" << endl;
        return false;
    }
    return true;
}

/**
 * @brief 接收消息，二进制
 *
 * @param buffer 接收的消息
 * @param type 消息类型
 * @return true
 * @return false
 */
bool TcpClient::recvMsgBin(void *buffer, MessageType &type)
{
    // 1.接收消息长度
    int len;
    int ret = recv(this->cfd, &len, sizeof(len), 0);
    if (ret < 0)
    {
        cout << "接收消息长度失败" << endl;
        return false;
    }
    else if (ret == 0)
    {
        cout << "服务器断开连接" << endl;
        return false;
    }
    // 2.将获取的长度大端转小端
    len = ntohl(len);

    // 3.接收消息类型
    char msgType;
    ret = recv(this->cfd, &msgType, sizeof(msgType), 0);
    if (ret < 0)
    {
        cout << "接收消息类型失败" << endl;
        return false;
    }
    else if (ret == 0)
    {
        cout << "服务器断开连接" << endl;
        return false;
    }
    type = (msgType == 'H') ? MessageType::Heart : MessageType::Data;

    // 4.接收消息内容，在此只接收len-1个字节，这个-1减去前面接收的消息类型，并且在此解决tcp粘包问题
    ret = recv(this->cfd, buffer, len - 1, 0);
    if (ret < 0)
    {
        cout << "接收消息失败" << endl;
        return false;
    }
    else if (ret == 0)
    {
        cout << "服务器断开连接" << endl;
        return false;
    }
    return true;
}

// 重置计数
bool TcpClient::resetCount()
{
    this->count = 0;
    return true;
}

// 增加计数
bool TcpClient::addCount(int num)
{
    this->count += num; // 默认增加计数为1
    return true;
}

// 获取计数
int TcpClient::getCount()
{
    return this->count;
}