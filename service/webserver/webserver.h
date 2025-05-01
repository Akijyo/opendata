#include "cpublic.h"
#include "fileframe/include/logfile.h"
#include "include/Iconnection.h"
#include "include/msql.h"
#include "include/threadpool.h"
#include "procheart/include/procheart.h"
#include "stringop/include/split.h"
#include "stringop/include/stringop.h"
#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

class st_client // 客户端结构体
{
  public:
    std::string ip;      // 客户端ip
    std::string sendmsg; // 发送的消息
    std::string recvmsg; // 接收的消息
};

class st_msg // 接收/发送消息队里的结构体
{
  public:
    int cfd;         // 客户端的socketfd
    std::string msg; // 消息内容
    st_msg(int cfd, std::string msg) : cfd(cfd), msg(msg)
    {
    }
};

class webserver
{
  private:
    logfile &lg;            // 日志文件对象，通过依赖注入构造对象
    ThreadPool *threadpool; // 线程池对象，通过依赖注入构造对象

    std::deque<std::shared_ptr<st_msg>> readQueue; // 接收消息队列
    std::mutex readQueueMutex;                     // 读队列互斥锁

    std::deque<std::shared_ptr<st_msg>> writeQueue; // 发送消息队列
    std::mutex writeQueueMutex;                     // 写队列互斥锁

    std::map<int, st_client> clientMap; // 客户端的socketfd和客户端信息的映射

    int sendPipe[2]; // 管道，用于工作线程通知发送线程响应消息

    std::atomic<bool> isRunning; // 运行标志，当false时需要整个类程序退出

    bool setNonBlocking(int fd); // 设置套接字非阻塞
    //----------------------------------------------------------------------
    bool acceptClient(int listenfd, int repfd); // recvThread中的3.3子函数，接受客户端连接
    bool recvHttpGet(int cfd, int repfd);       // recvThread中的3.5子函数，接收客户端发来的http请求
    //----------------------------------------------------------------------
    bool getMsgFromQueue(int sepfd);            // sendThread中的3.2子函数，用于从发送队列中取出消息并且监听读事件
    bool returnMsgToClient(int cfd, int sepfd); // sendThread中的3.3子函数，向客户端回复消息
    //----------------------------------------------------------------------
    void working(); // 工作线程函数，负责从接收队列中取数据，再经过“业务处理”得到的最终数据加入到发送队列中
    //----------------------------------------------------------------------
    bool getHttpValue(const std::string &get, const std::string &key,
                      std::string &value);                        // 解析http请求报文，获取请求头中的key对应的value
    void service(std::string &httpGet, std::string &willSendMsg); // 业务处理函数，负责处理http请求

  public:
    int recvPipe[2]; // 用在接收线程中，用于检测是否收到程序退出的消息

    explicit webserver(logfile &lg, ThreadPool *threadpool) : lg(lg), threadpool(threadpool)
    {
        this->isRunning = true;
        pipe(this->recvPipe); // 创建管道
        pipe(this->sendPipe); // 创建管道
    }

    void recvThread(int listenPort); // 接收信息的线程函数
    void sendThread();               // 发送信息的线程函数

    ~webserver()
    {
        this->threadpool->waitAllTasksDone();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        close(this->recvPipe[0]);
        close(this->recvPipe[1]);
        close(this->sendPipe[0]);
        close(this->sendPipe[1]);
        for (auto &it : this->clientMap)
        {
            close(it.first); // 关闭所有客户端的socket
        }
    }
};

bool webserver::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        return false;
    }
    return true;
}

void webserver::recvThread(int listenPort)
{
    // R1.初始化监听套接字
    // R1.1.创建监听套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        lg.writeLine("创建listenfd失败");
        return;
    }
    // R1.2.设置非阻塞
    this->setNonBlocking(listenfd);
    // R1.3.设置端口复用
    int opt = 1;
    unsigned int len = sizeof(opt);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
    // R1.4完善sockaddr_in结构体
    sockaddr_in laddr;
    laddr.sin_family = AF_INET;                // IPv4
    laddr.sin_port = htons(listenPort);        // 小端转大端,绑定传入的端口
    laddr.sin_addr.s_addr = htonl(INADDR_ANY); // 本地任意IP
    // R1.5绑定本地ip和端口
    int bindret = bind(listenfd, (sockaddr *)&laddr, sizeof(laddr));
    // R1.6.监听来自客户端的套接字
    int listenret = listen(listenfd, 128);
    if (bindret == -1 || listenret == -1)
    {
        lg.writeLine("初始化监听失败：ip：%s，port：%s", inet_ntoa(laddr.sin_addr), std::to_string(listenPort).c_str());
        return;
    }

    // R2.初始化epoll
    // R2.1 创建epoll描述符
    int repfd = epoll_create1(0);
    if (repfd == -1)
    {
        lg.writeLine("创建epoll描述符失败");
        return;
    }
    // R2.2 将监听套接字和读线程管道加入epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if (epoll_ctl(repfd, EPOLL_CTL_ADD, listenfd, &ev) == -1)
    {
        return;
    }
    ev.events = EPOLLIN;
    ev.data.fd = this->recvPipe[0];
    if (epoll_ctl(repfd, EPOLL_CTL_ADD, this->recvPipe[0], &ev) == -1)
    {
        return;
    }

    // R3.进入epoll事件循环
    struct epoll_event events[1024];
    int size = sizeof(events) / sizeof(events[0]);
    while (true)
    {
        // R3.1等待epoll事件
        int eventCount = epoll_wait(repfd, events, size, -1);
        if (eventCount == -1)
        {
            lg.writeLine("epoll_wait失败");
            return;
        }
        // R3.2遍历epoll事件
        for (int i = 0; i < eventCount; i++)
        {
            int curfd = events[i].data.fd;
            // R3.3判断是否是监听描述符有连接事件
            if (curfd == listenfd)
            {
                if (!this->acceptClient(listenfd, repfd))
                {
                    lg.writeLine("接收客户端连接失败");
                    continue;
                }
                continue;
            }
            // R3.4.判断是否是读线程管道有数据可读
            // 如果读线程管道有事件，说明外部程序的退出信号已经到达，需要该次事件去通知发送线程退出，通知发送线程的方法是通过管道
            if (curfd == this->recvPipe[0])
            {
                lg.writeLine("（接收线程：）即将退出...");
                // R3.4.1设置标志为不运行
                this->isRunning = false;
                // R3.4.2通知发送线程该退出
                write(this->sendPipe[1], "e", 1);
                // R3.4.3关闭epoll
                close(repfd);
                // 这是一步经典的线程同步退出方法，在对方线程接受到该消息之后，下一步会判断运行标注，判断false之后会立刻退出
                return; // 退出
            }
            // R3.5判断是否是客户端发送来数据
            // 由于epoll中一共就三种fd，本别是监听套接字，管道，客户端套接字，上面两个if已经处理完前面两个，这里只可能是客户端套接字
            if (events[i].events & EPOLLIN)
            {
                if (!this->recvHttpGet(curfd, repfd))
                {
                    continue; // 连接断开或者没有数据的情况
                }
            }
        }
    }
}

bool webserver::acceptClient(int listenfd, int repfd)
{
    // R3.3.1说明源端口处有客户端连接，处理连接
    // R3.3.2创建客户端地址结构体
    sockaddr_in caddr;
    socklen_t addrlen = sizeof(caddr);
    // R3.3.3接受客户端连接
    int clientfd = accept(listenfd, (sockaddr *)&caddr, &addrlen);

    if (clientfd == -1) // 非阻塞accept，如果没有连接请求，返回-1（失败），errno=EAGAIN或者EWOULDBLOCK
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return true;
        }
        else
        {
            return false; // 其他错误
        }
    }
    // R3.3.4设置非阻塞
    this->setNonBlocking(clientfd);
    // R3.3.5保存到客户端套接字-客户端信息的映射中
    this->clientMap[clientfd].ip = inet_ntoa(caddr.sin_addr);
    // R3.3.6将客户端的套接字加入到该线程的epoll中，去监听读事件
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = clientfd;
    if (epoll_ctl(repfd, EPOLL_CTL_ADD, clientfd, &ev) == -1)
    {
        return false;
    }
    lg.writeLine("（接收线程：）客户端（socket=%d）连接成功,IP:%s, port:%d", clientfd, inet_ntoa(caddr.sin_addr),
                 ntohs(caddr.sin_port));
    return true;
}

// recvThread中的3.5子函数，接收客户端发来的http请求
bool webserver::recvHttpGet(int cfd, int repfd)
{
    char buffer[10240];
    int buflen;
    memset(buffer, 0, sizeof(buffer));

    // R3.5.1接收客户端传来的数据
    buflen = recv(cfd, buffer, sizeof(buffer), 0);
    if (buflen <= 0)
    {
        // R3.5.1a连接断开
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 暂时没有数据，非错误情况，由非阻塞io造成
            return false; // 原continue
        }
        lg.writeLine("（接收线程：）客户端（socket=%d,ip=%s）连接断开", cfd, this->clientMap[cfd].ip.c_str());
        // 从clientMap中和读epoll中删除这个客户端
        this->clientMap.erase(cfd);
        epoll_ctl(repfd, EPOLL_CTL_DEL, cfd, NULL);
        return false; // 原continue
    }
    lg.writeLine("（接收线程：）接收客户端（socket=%d）的%d字节数据", cfd, buflen);
    // R3.5.2成功接收数据，将数据添加到对应socket的接受缓冲区中
    this->clientMap[cfd].recvmsg.append(buffer, buflen);
    // R3.5.3判断是否是http请求报文
    if (this->clientMap[cfd].recvmsg.ends_with("\r\n\r\n"))
    {
        lg.writeLine("（接收线程：）接收到来自客户端（socket=%d,ip=%s）的一次完整请求", cfd,
                     this->clientMap[cfd].ip.c_str());
        // R3.5.4将接收到的http请求放入消息队列中
        std::shared_ptr<st_msg> msgPtr = std::make_shared<st_msg>(cfd, this->clientMap[cfd].recvmsg);
        this->readQueueMutex.lock();
        this->readQueue.push_back(msgPtr);
        this->readQueueMutex.unlock();
        // R3.5.5开启工作线程处理核心http业务
        this->threadpool->addTask([this]() -> void { this->working(); });
        // R3.5.6清空接收缓冲区
        this->clientMap[cfd].recvmsg.clear();
    }
    else
    {
        // R3.5.3a不是http请求报文，关闭对端
        if (this->clientMap[cfd].recvmsg.size() > 2000)
        {
            lg.writeLine("（接收线程：）客户端（socket=%d,ip=%s）发送的http请求报文过长，关闭连接", cfd,
                         this->clientMap[cfd].ip.c_str());
            close(cfd);
            this->clientMap.erase(cfd);
            epoll_ctl(repfd, EPOLL_CTL_DEL, cfd, NULL);
        }
        return false;
    }
    return true;
}

void webserver::working()
{
    // 1.从消息队列中取出消息
    this->readQueueMutex.lock();
    std::shared_ptr<st_msg> msgPtr = this->readQueue.front();
    this->readQueue.pop_front();
    this->readQueueMutex.unlock();
    // 2.业务处理
    std::string sendbuf;
    this->service(msgPtr->msg, sendbuf);
    std::string willSendMsg;
    willSendMsg += "HTTP/1.1 200 OK\r\n"
                   "Server: webserver\r\n"
                   "Content-Type: text/html;charset=utf-8\r\n"
                   "Content-Length: " +
                   std::to_string(sendbuf.size()) + "\r\n\r\n" += sendbuf;
    // 3.将结果放入发送队列中，并且通知发送线程处理
    std::shared_ptr<st_msg> sendMsgPtr = std::make_shared<st_msg>(msgPtr->cfd, willSendMsg);
    this->writeQueueMutex.lock();
    this->writeQueue.push_back(sendMsgPtr);
    this->writeQueueMutex.unlock();
    // 4.通知发送线程
    write(this->sendPipe[1], (char *)"s", 1);
}

void webserver::sendThread()
{
    // S1.创建epoll描述符
    int sepfd = epoll_create1(0);
    if (sepfd == -1)
    {
        lg.writeLine("读线程创建epoll描述符失败");
        return;
    }
    // S2.将发送管道加入epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = this->sendPipe[0];
    if (epoll_ctl(sepfd, EPOLL_CTL_ADD, this->sendPipe[0], &ev) == -1)
    {
        return;
    }
    // S3.进入epoll事件循环
    struct epoll_event events[1024];
    int size = sizeof(events) / sizeof(events[0]);
    while (true)
    {
        int eventCount = epoll_wait(sepfd, events, size, -1);
        if (eventCount == -1)
        {
            lg.writeLine("读线程epoll_wait失败");
            return;
        }
        // S3.1遍历epoll事件
        for (int i = 0; i < eventCount; i++)
        {
            int curfd = events[i].data.fd;
            // S3.2判断是否是发送管道有数据可读，说明工作线程通知发送线程有数据需要发送
            if (curfd == this->sendPipe[0])
            {
                // S3.2.1a如果程序即将退出
                if (!this->isRunning)
                {
                    lg.writeLine("发送线程即将退出...");
                    close(sepfd);
                    return;
                }
                char ch;
                read(this->sendPipe[0], &ch, 1);
                if (!this->getMsgFromQueue(sepfd))
                {
                    continue;
                }
                continue;
            }
            // S3.3即将为客户端返回数据
            if (events[i].events & EPOLLOUT)
            {
                if (!this->returnMsgToClient(curfd, sepfd))
                {
                    continue;
                }
            }
        }
    }
}

bool webserver::getMsgFromQueue(int sepfd)
{
    std::shared_ptr<st_msg> msgPtr;
    this->writeQueueMutex.lock();
    while (!this->writeQueue.empty())
    {
        // S3.2.1从发送队里中取出消息
        msgPtr = this->writeQueue.front();
        this->writeQueue.pop_front();
        // S3.2.2将要发送的信息保存在对应客户端的map中的发送缓冲区中
        this->clientMap[msgPtr->cfd].sendmsg.append(msgPtr->msg);
        // S3.2.3将对应客户端的socket加入epoll中，去监听写事件
        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = msgPtr->cfd;
        if (epoll_ctl(sepfd, EPOLL_CTL_ADD, msgPtr->cfd, &ev) == -1)
        {
            return false;
        }
    }
    this->writeQueueMutex.unlock();
    return true;
}

bool webserver::returnMsgToClient(int cfd, int sepfd)
{
    // S3.3.1根据map获取当前socket的对应发送缓冲区的引用
    std::string &buffer = this->clientMap[cfd].sendmsg;
    // S3.3.2给客户端发回响应，并且记录下已发送字节数
    int writen = send(cfd, buffer.data(), buffer.size(), 0);
    if (writen <= 0)
    {
        // S3.3.2a连接断开
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 发送缓冲区已满，稍后再试
            return false; // 原continue
        }
        lg.writeLine("（响应线程：）客户端（socket=%d,ip=%s）连接断开", cfd, this->clientMap[cfd].ip.c_str());
        // 从clientMap中和读epoll中删除这个客户端
        this->clientMap.erase(cfd);
        epoll_ctl(sepfd, EPOLL_CTL_DEL, cfd, NULL);
        return false; // 原continue
    }
    // S3.3.2b.发送成功，打印日志
    lg.writeLine("（响应线程：）成功向客户端（socket=%d,ip=%s）响应%d字节数据", cfd, this->clientMap[cfd].ip.c_str(),
                 writen);
    // S3.3.3将已经发送的数据从缓冲区中删去
    buffer.erase(0, writen);
    // S3.3.4如果改文件描述符的发送缓冲区已经为空，说明已经全部发送完毕，将该fd从epoll中删除
    if (buffer.empty())
    {
        // 从epoll中删除写事件
        struct epoll_event ev;
        ev.events = EPOLLIN; // 添加写事件
        ev.data.fd = cfd;
        if (epoll_ctl(sepfd, EPOLL_CTL_DEL, cfd, &ev) == -1)
        {
            return false;
        }
    }
    return true;
}

// 解析http请求报文，获取请求头中的key对应的value
bool webserver::getHttpValue(const std::string &get, const std::string &key, std::string &value)
{
    // http://192.168.150.128:8080/api?username=wucz&passwd=wuczpwd
    // GET /api?username=wucz&passwd=wuczpwd HTTP/1.1
    // Host: 192.168.150.128:8080
    // Connection: keep-alive
    // Upgrade-Insecure-Requests: 1
    // .......

    size_t pos = get.find(key + "=");
    if (pos == std::string::npos)
    {
        return false; // 没有该key
    }

    // 这里已经找到了key的位置，pos指向key的开始位置
    // 然后移动key的字符串长度加一个等于号长长度的位置
    pos += key.length() + 1;

    // 根据上面的位置一路到下一个&符号
    size_t endPos = get.find('&', pos);
    if (endPos == std::string::npos)
    {
        // 说明是最后一个参数，需要去查找空格
        endPos = get.find(' ', pos);
    }
    // 获取value
    if (endPos == std::string::npos)
    {
        // No more parameters, take the rest of the string
        value = get.substr(pos);
    }
    else
    {
        // Extract value until the next parameter
        value = get.substr(pos, endPos - pos);
    }
    return true;
}

// 业务处理函数，负责处理http请求
void webserver::service(std::string &httpGet, std::string &willSendMsg)
{
    // 1.连接数据库
    std::shared_ptr<IConnection> connection = std::make_shared<mysql>();
    connection->connect("127.0.0.1", "root", "13690919281qq", "idc", 3307);
    // 2.查找判断用户名，密码是否正确
    // 2.1获取http请求中的用户名和密码参数
    std::string username, passwd;
    this->getHttpValue(httpGet, "username", username);
    this->getHttpValue(httpGet, "passwd", passwd);
    // 2.2判断是否存在该用户名和密码
    std::string sql = "select ip from T_USERINFO where username='" + username + "' and passwd='" + passwd + "'";
    if (!connection->query(sql))
    {
        // 2.2aSQL执行错误
        std::string errorResponse = "<html><body><h1>Error 500: Internal Server Error</h1></body></html>";
        willSendMsg = errorResponse;
        return;
    }
    // 检查是否有记录返回
    if (!connection->next())
    {
        // 没有记录 - 用户名或密码错误
        std::string errorResponse = "<html><body><h1>Error 401: Unauthorized</h1>"
                                    "<p>Invalid username or password.</p></body></html>";
        willSendMsg = errorResponse;
        return;
    }
    // 3.判断ip是否在绑定ip范围中
    std::optional<std::string> ip = connection->value(0);
    if (!ip.has_value())
    {
        std::string errorResponse = "<html><body><h1>Error 403: Forbidden</h1>"
                                    "<p>IP not in range.</p></body></html>";
        willSendMsg = errorResponse;
        return;
    }
    // 3.a如果ip不在范围内，返回403，这里省略数行代码
    //  4.判断用户是否有权限使用接口
    // 4.1获取http请求中的接口名参数
    std::string intername;
    this->getHttpValue(httpGet, "intername", intername);
    // 4.2判断用户是否有权限使用接口
    sql = "select count(*) from T_USERCTRL where username='" + username + "' and intername='" + intername + "'";
    
    if (!connection->query(sql))
    {
        // 4.2aSQL执行错误
        std::string errorResponse = "<html><body><h1>Error 500: Internal Server Error</h1></body></html>";
        willSendMsg = errorResponse;
        return;
    }
    if (!connection->next())
    {
        // 4.2b没有权限
        std::string errorResponse = "<html><body><h1>Error 403: Forbidden</h1>"
                                    "<p>No permission to use this interface.</p></body></html>";
        willSendMsg = errorResponse;
        return;
    }
    // 5.根据接口名，查找接口的配置参数
    sql = "select selectsql,columns,param from T_INTERCFG where intername='" + intername + "'";
    if (!connection->query(sql))
    {
        // 5.1SQL执行错误
        std::string errorResponse = "<html><body><h1>Error 500: Internal Server Error</h1></body></html>";
        willSendMsg = errorResponse;
        return;
    }
    if (!connection->next())
    {
        // 5.2没有接口配置
        std::string errorResponse = "<html><body><h1>Error 404: Not Found</h1>"
                                    "<p>Interface not found.</p></body></html>";
        willSendMsg = errorResponse;
        return;
    }
    // 5.3获取接口的配置参数
    std::string selectsql = connection->value(0).value();
    std::string columns = connection->value(1).value();
    std::string param = connection->value(2).value_or("");
    // 5.4分割查询目标表的字段columns
    ccmdstr columnsSplit(columns, ",");
    if (param.size() != 0)
    {
        // 5.5分割查询条件param
        ccmdstr paramSplit(param, ",");
        // 5.6从http请求中获取查询条件的值
        std::vector<std::string> paramValues;
        for (int i = 0; i < paramSplit.size(); i++)
        {
            std::string value;
            this->getHttpValue(httpGet, paramSplit[i], value);
            paramValues.push_back(value);
        }
        // 5.7拼接查询sql语句，把查询条件的值替换到sql语句中
        for (auto &it : paramValues)
        {
            replaceStr(selectsql, "?", it, false);
        }
    }

    // 6.查找，组织查询的数据
    // 6.1执行查询sql语句
    if (!connection->query(selectsql))
    {
        // 6.1aSQL执行错误
        std::string errorResponse = "<html><body><h1>Error 500: Internal Server Error</h1></body></html>";
        willSendMsg = errorResponse;
        return;
    }
    // 6.2获取查询结果
    nlohmann::json root;
    while (connection->next())
    {
        nlohmann::json item;
        for (int i = 0; i < columnsSplit.size(); i++)
        {
            item[columnsSplit[i]] = connection->value(columnsSplit[i]).value_or("");
        }
        root.push_back(item);
    }
    willSendMsg = root.dump(4);
}