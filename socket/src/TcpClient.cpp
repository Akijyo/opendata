#include "../include/TcpClient.h"
using namespace std;

TcpClient::TcpClient()
{
    this->cfd = socket(AF_INET, SOCK_STREAM, 0); // 创建通信套接字
    if (this->cfd == -1)
    {
        // cout << "创建通信套接字失败" << endl;
    }
    // 忽略 SIGPIPE 信号
    signal(SIGPIPE, SIG_IGN);
    //******send函数的SIGPIPE信号处理，对这个信号忽略，否则下面调用send发送函数时因为
    //******服务器端关闭连接，会产生SIGPIPE信号，导致主程序退出
    this->count = 0;

    // 设置非阻塞
    // this->setnonblocking(this->cfd);
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

    // 2.连接服务器，非阻塞模式下，connect函数会立即返回失败（-1），不会阻塞，并将errno设置为EINPROGRESS
    int ret;
    ret = connect(this->cfd, (sockaddr *)&saddr, sizeof(saddr));
    if (ret == -1)
    {
        if (errno != EINPROGRESS)
        {
            // cout << "连接失败" << endl;
            return false;
        }
    }
    // 3.判断是否连接成功
    pollfd fds;
    fds.fd = this->cfd;
    fds.events = POLLOUT;
    poll(&fds, 1, -1);
    if (fds.revents != POLLOUT)
    {
        // cout << "连接失败" << endl;
        return false;
    }
    return true;
}

/**
 * @brief 接收消息，带消息类型，解决粘包问题
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
    char msgType;
    this->typeTOchar(type, msgType);
    // 3.定义完整的发送缓冲区
    string buffer;
    buffer.reserve(sizeof(len) + sizeof(msgType) + msg.size());
    // 4.将消息长度、消息类型、消息内容依次添加到缓冲区
    buffer.append((char *)&len, sizeof(len));
    buffer.push_back(msgType);
    buffer.append(msg);

    // 5.由于是非阻塞IO，所以需要循环发送
    //&buffer[0]直接操控string的内存，不会有拷贝，并且传入的是char*类型指针，
    // 如果是&bufffer，传入的是string类型的引用(指针)，会和void*类型不匹配，从而报错。
    //&buffer[0]可用buffer.data()代替
    if (!this->sendn(this->cfd, &buffer[0], buffer.size()))
    {
        return false;
    }
    return true;
}

/**
 * @brief 接收消息，带消息类型，解决粘包问题
 *
 * @param msg 接收的消息
 * @param type 接收服务端发来的消息类型
 * @return string
 */
bool TcpClient::recvMsgWithType(string &msg, MessageType &type)
{
    // 1.接收消息长度
    int len;
    if (!this->readn(this->cfd, &len, sizeof(len)))
    {
        // cout << "接收消息长度失败" << endl;
        return false;
    }
    len = ntohl(len); // 大端转小端

    // 2.接收消息类型
    char msgType;
    if (!this->readn(this->cfd, &msgType, sizeof(msgType)))
    {
        // cout << "接收消息类型失败" << endl;
        return false;
    }
    this->charTOtype(msgType, type); // 将消息类型转换为枚举类型

    // 3.接收消息内容
    msg.resize(len - 1);

    // 里面的len-1是因为上面的消息类型占了一个字节，减去。
    //&msg[0]直接操控string的内存，不会有拷贝，并且传入的是char*类型指针，
    // 如果是&msg，传入的是string类型的引用(指针)，会和void*类型不匹配，从而报错。
    if (!this->readn(this->cfd, &msg[0], len - 1))
    {
        // cout << "接收消息内容失败" << endl;
        return false;
    }
    return true;
}

/////////////////////////////////////////////////////////
////////////////////二进制读取////////////////////////////
/////////////////////////////////////////////////////////
bool TcpClient::sendMsgBin(void *msg, size_t n, MessageType type)
{
    // 1.小端转大端，长度为信息大小+信息类型的大小
    int len = htonl(n + 1);
    // 2.获取消息类型
    char msgType;
    this->typeTOchar(type, msgType);
    // 3.定义完整的发送缓冲区
    string buffer;
    buffer.reserve(sizeof(len) + sizeof(msgType) + n);
    // 4.将消息长度、消息类型、消息内容依次添加到缓冲区
    buffer.append((char *)&len, sizeof(len));
    buffer.push_back(msgType);
    buffer.append((char *)msg, n);
    // 5.由于是非阻塞IO，所以需要循环发送
    if (!this->sendn(this->cfd, &buffer[0], buffer.size()))
    {
        return false;
    }
    return true;
}

bool TcpClient::recvMsgBin(void *buffer, MessageType &type)
{
    // 1.接收消息长度
    int len;
    if (!this->readn(this->cfd, &len, sizeof(len)))
    {
        // cout << "接收消息长度失败" << endl;
        return false;
    }
    len = ntohl(len); // 大端转小端
    // 2.接收消息类型
    char msgType;
    if (!this->readn(this->cfd, &msgType, sizeof(msgType)))
    {
        // cout << "接收消息类型失败" << endl;
        return false;
    }
    this->charTOtype(msgType, type); // 将消息类型转换为枚举类型
    // 3.接收消息内容
    if (!this->readn(this->cfd, buffer, len - 1))
    {
        // cout << "接收消息内容失败" << endl;
        return false;
    }
    return true;
}

bool TcpClient::recvMsgBin(void *buffer, MessageType &type, size_t &size)
{
    // 1.接收消息长度
    int len;
    if (!this->readn(this->cfd, &len, sizeof(len)))
    {
        // cout << "接收消息长度失败" << endl;
        return false;
    }
    len = ntohl(len); // 大端转小端
    size = len - 1;   // 消息内容的大小
    // 2.接收消息类型
    char msgType;
    if (!this->readn(this->cfd, &msgType, sizeof(msgType)))
    {
        // cout << "接收消息类型失败" << endl;
        return false;
    }
    this->charTOtype(msgType, type); // 将消息类型转换为枚举类型

    // 3.接收消息内容
    if (!this->readn(this->cfd, buffer, size))
    {
        // cout << "接收消息内容失败" << endl;
        return false;
    }
    return true;
}