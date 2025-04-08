#include "../include/TcpServer.h"
using namespace std;

// 构造函数
TcpServer::TcpServer()
{
    // 创建套接字
    this->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->fd == -1)
    {
        cout << "套接字创建失败" << endl;
    }
    // 忽略 SIGPIPE 信号
    signal(SIGPIPE, SIG_IGN);
    //******send函数的SIGPIPE信号处理，对这个信号忽略，否则下面调用send发送函数时因为
    //******服务器端关闭连接，会产生SIGPIPE信号，导致主程序退出

    // 设置非阻塞
    int flags = fcntl(this->fd, F_GETFL, 0);
    fcntl(this->fd, F_SETFL, flags | O_NONBLOCK);
    // 创建epoll描述符
    this->epfd = epoll_create(1);
}
// 析构函数
TcpServer::~TcpServer()
{
    if (this->fd != -1)
    {
        close(fd); // 关闭套接字
    }
    if (this->epfd != -1)
    {
        close(this->epfd); // 关闭epoll描述符
    }
    unique_lock<shared_mutex> lock(this->clientMap_mtx);
    {
        for (auto &it : this->client_map)//释放client_map中的所有资源
        {
            close(it.second->cfd);
        }
    }
}

bool TcpServer::createListen(unsigned short port)
{
    // 1.创建通信套接字
    sockaddr_in caddr;
    caddr.sin_family = AF_INET;         // IPv4
    caddr.sin_port = htons(port);       // 小端转大端,绑定传入的端口
    caddr.sin_addr.s_addr = INADDR_ANY; // 本地任意IP

    // 2.绑定本地IP和端口
    if (bind(this->fd, (sockaddr *)&caddr, sizeof(caddr)) == -1)
    {
        cout << "绑定失败" << endl;
        return false;
    }
    else
    {
        cout << "绑定成功,IP:" << inet_ntoa(caddr.sin_addr) << ", port:" << ntohs(caddr.sin_port) << endl;
    }

    // 3.监听来自客户端的套接字
    if (listen(this->fd, 128) == -1)
    {
        cout << "监听失败" << endl;
        return false;
    }
    else
    {
        cout << "套接字监听成功" << endl;
    }
    // 4.将监听套接字加入到epoll监听中
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;//边缘触发模式
    ev.data.fd = this->fd;
    if (epoll_ctl(this->epfd, EPOLL_CTL_ADD, this->fd, &ev) == -1)
    {
        cout << "监听描述符添加epoll失败" << endl;
        return false;
    }
    return true;
}

/**
 * @brief 程序的核心函数，完成以下功能
 * 1. 创建epoll监听
 * 2. 遍历监听到的描述符
 * 3. 如果是监听描述符，则开启一个accept线程，接受连接，并将连接描述符加入到epoll监听中
 * 4. 如果是连接描述符，则开启一个新线程，处理连接
 *
 * @param pool 连接池
 * @param callback 外部传入的回调函数，用于处理连接
 * @return true
 * @return false
 */
bool TcpServer::epollSocket(ThreadPool *pool, function<void *(shared_ptr<clientNode>)> callback)
{
    struct epoll_event events[1024];
    int size = sizeof(events) / sizeof(events[0]);
    while (1)
    {
        // 1. 创建epoll监听
        int num = epoll_wait(this->epfd, events, size, -1);
        if (num == -1)
        {
            cout << "epoll监听失败" << endl;
            return false;
        }
        // 2. 遍历监听到的描述符
        for (int i = 0; i < num; i++)
        {
            int curfd = events[i].data.fd;
            // 1. 如果是监听描述符:
            if (curfd == this->fd)
            {
                //   1.1. 开启新线程，接受连接
                pool->addTask([this]() -> void * {
                    this->acceptConn();
                    return nullptr;
                });
                //   1.2. 将连接描述符加入到epoll监听中(已在acceptConn中加入)
            }
            else
            {
                // 2. 如果是连接描述符:
                {
                    // 上锁保护client_map
                    shared_lock<shared_mutex> lock(this->clientMap_mtx);
                    // 查找连接描述符，一致性检查，以client_map为准
                    if (this->client_map.find(curfd) == this->client_map.end())
                    {
                        cout << "未找到连接描述符" << endl;
                        continue;
                    }
                    else
                    {
                        // 获取client_map中的client结点
                        shared_ptr<clientNode> client = this->client_map[curfd];
                        if (client->is_working)
                        {
                            continue;
                        }
                        // 将任务添加到线程池
                        client->is_working = true;
                        pool->addTask([callback, client]() -> void * {
                            callback(client);
                            client->is_working = false;
                            return nullptr;
                        });
                    }
                }
            }
        }
    }
    return true;
}
bool TcpServer::heartbeatThread(const int time)
{
    while (1)
    {
        // 遍历map容器中的每一个client结点，
        // 如果超过3次心跳包没有收到，就断开对那个客户端的连接，并且从容器中删除
        {
            unique_lock<shared_mutex> lock(this->clientMap_mtx);
            for (auto it = this->client_map.begin(); it != this->client_map.end();)
            {
                //计数+1
                it->second->addCount();
                //如果计数超过3次，就断开连接
                if (it->second->getCount() > 3)
                {
                    cout << it->second->cfd << "（心跳包）客户端断开连接" << endl;
                    it->second->is_active = false;
                    it->second->is_working = false;
                    this->closeConn(it->second);
                    it = this->client_map.erase(it);
                }
                else
                {
                    it++;
                }
            }
        }
        sleep(time);
    }
    return true;
}

bool TcpServer::acceptConn()
{
    // 1.创建客户端地址结构体
    sockaddr_in caddr;
    socklen_t addrlen = sizeof(caddr);

    // 2.接受客户端连接
    // 由于是非阻塞+ET，所以需要循环接受，不然高并发下会漏连接
    while (true)
    {
        int cfd = accept(this->fd, (sockaddr *)&caddr, &addrlen);
        //非阻塞accept，如果没有连接请求，返回-1（失败），errno=EAGAIN或者EWOULDBLOCK
        if (cfd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                cout << "等待客户端连接失败" << endl;
                return false;
            }
        }
        // 3.设置非阻塞
        int flags = fcntl(cfd, F_GETFL, 0);
        fcntl(cfd, F_SETFL, flags | O_NONBLOCK);
        // 4.打印客户端连接信息
        cout << "客户端连接成功,IP:" << inet_ntoa(caddr.sin_addr) << ", port:" << ntohs(caddr.sin_port) << endl;
        // 5.将客户端连接信息加入到client_map中
        shared_ptr<clientNode> client = make_shared<clientNode>(cfd);
        {
            unique_lock<shared_mutex> lock(this->clientMap_mtx);
            this->client_map.insert(pair<int, shared_ptr<clientNode>>(cfd, client));
        }
        // 6. 将连接描述符加入到epoll监听中
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;//边缘触发模式
        ev.data.fd = cfd;
        if (epoll_ctl(this->epfd, EPOLL_CTL_ADD, cfd, &ev) == -1)
        {
            cout << "连接描述符添加epoll失败" << endl;
            return false;
        }
    }
    return true;
}

bool TcpServer::closeConn(shared_ptr<clientNode> client)
{
    // 从epoll监听中删除
    if (epoll_ctl(this->epfd, EPOLL_CTL_DEL, client->cfd, NULL) == -1)
    {
        cout << "epoll删除连接描述符失败" << endl;
        return false;
    }
    if (close(client->cfd) == -1)
    {
        cout << "关闭连接失败" << endl;
        return false;
    }
    return true;
}

/**
 * @brief 接收消息，带消息类型，解决粘包问题
 *
 * @param cfd 通信文件描述符
 * @param msg 发送的消息
 * @param type 发送的消息类型
 * @return true
 * @return false
 */
bool TcpServer::sendMsgWithType(shared_ptr<clientNode> client, string msg, MessageType type)
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
    if (!this->sendn(client->cfd, &buffer[0], buffer.size()))
    {
        return false;
    }
    return true;
}

/**
 * @brief 接收消息，带消息类型，解决粘包问题
 *
 * @param cfd 通信文件描述符
 * @param type 接收客户端发来的消息类型
 * @return string
 */
bool TcpServer::recvMsgWithType(string &msg, shared_ptr<clientNode> client, MessageType &type)
{
    // 1.接收消息长度
    int len;
    if (!this->readn(client->cfd, &len, sizeof(len)))
    {
        cout << "接收消息长度失败" << endl;
        return false;
    }
    len = ntohl(len); // 大端转小端

    // 2.接收消息类型
    char msgType;
    if (!this->readn(client->cfd, &msgType, sizeof(msgType)))
    {
        cout << "接收消息类型失败" << endl;
        return false;
    }
    this->charTOtype(msgType, type); // 将消息类型转换为枚举类型

    // 3.接收消息内容
    msg.resize(len - 1);

    // 里面的len-1是因为上面的消息类型占了一个字节，减去。
    //&msg[0]直接操控string的内存，不会有拷贝，并且传入的是char*类型指针，
    // 如果是&msg，传入的是string类型的引用(指针)，会和void*类型不匹配，从而报错。
    if (!this->readn(client->cfd, &msg[0], len - 1))
    {
        cout << "接收消息内容失败" << endl;
        return false;
    }
    return true;
}

/////////////////////////////////////////////////////////
////////////////////二进制读取////////////////////////////
/////////////////////////////////////////////////////////
bool TcpServer::sendMsgBin(shared_ptr<clientNode> client, void *msg, size_t n, MessageType type)
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
    if (!this->sendn(client->cfd, &buffer[0], buffer.size()))
    {
        return false;
    }
    return true;
}

bool TcpServer::recvMsgBin(void *buffer, std::shared_ptr<clientNode> client, MessageType &type)
{
    // 1.接收消息长度
    int len;
    if (!readn(client->cfd, &len, sizeof(len)))
    {
        cout << "接收消息长度失败" << endl;
        return false;
    }
    len = ntohl(len); // 大端转小端
    // 2.接收消息类型
    char msgType;
    if (!readn(client->cfd, &msgType, sizeof(msgType)))
    {
        cout << "接收消息类型失败" << endl;
        return false;
    }
    this->charTOtype(msgType, type); // 将消息类型转换为枚举类型
    // 3.接收消息内容
    if (!readn(client->cfd, buffer, len - 1))
    {
        cout << "接收消息内容失败" << endl;
        return false;
    }
    return true;
}