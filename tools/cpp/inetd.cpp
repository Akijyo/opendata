#include "cpublic.h"
#include "fileframe/include/logfile.h"
#include "procheart/include/procheart.h"
#include "stringop/include/stringop.h"
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>
#include <vector>
using namespace std;

// 创建日志文件对象
logfile lg;
// 创建进程心跳对象
// procHeart ph;
// 进程心跳时间，单位秒
int phtimeout;
// epoll描述符
int epfd = -1;
// 客户端-目标服务器的套接字映射map
unordered_map<int, int> proxyMap;
// 套接字-数据缓冲区的映射map
map<int, string> socketBufferMap;

// 转发的网络参数
class route
{
  public:
    int srcport;  // 源端口
    string dstip; // 目标ip
    int dstport;  // 目标端口
    int socketfd; // 转发该ip的监听文件描述符
};
vector<route> routes; // 转发的网络参数列表

// 程序接受信号退出的函数
void EXIT(int sig);
// 程序的帮助文档
void help();
// 设置非阻塞
bool setnonblocking(int fd);
// 1.解析转发路由
bool parseRoute(string jsonFile);
// 2.初始化路由数组的文件描述符
bool initRouteFd();
// 3.创建epoll描述符并将路由数组的监听socket加入epoll
bool initEpollFd();
// 4.开启epoll事件循环
bool epollLoop();
// 4.3.1子函数，处理客户端的连接，的同时自己也要连接目标服务器，形成 客户端-代理程序-目标服务器的连接
bool acceptAndConnect(route &it);
// 4.4子函数，接收数据并且将数据保存在该socket的对应缓冲区中，等待epoll写事件的发送
bool recvData(int curfd);
// 4.5子函数，发送数据到写事件的socket中
bool sendData(int curfd);

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        help();
        return 0;
    }
    // 防止信号影响
    closeiosignal(false);
    // 让2号信号和15号信号都调用EXIT函数
    // 2号信号是Ctrl+C，15号信号是kill命令
    signal(2, EXIT);
    signal(15, EXIT);

    // 打开日志文件
    if (!lg.open(argv[1], ios::out))
    {
        cout << "打开日志文件失败！" << endl;
        return 0;
    }
    // 1.填充route数组的port和ip
    if (!parseRoute(argv[2]))
    {
        return 0;
    }
    // 2.初始化对源端口的监听
    if (!initRouteFd())
    {
        EXIT(-1);
    }
    // 3.创建epoll描述符并将路由数组的监听socket加入epoll
    if (!initEpollFd())
    {
        EXIT(-1);
    }
    // 至此路由数组全部初始化完毕，并且开启epoll管理监听

    // 4.开启epoll事件循环
    if (!epollLoop())
    {
        EXIT(-1);
    }
    return 0;
}

// 1.解析转发路由
bool parseRoute(string jsonFile)
{
    ifstream ifs;
    ifs.open(jsonFile);
    if (!ifs.is_open())
    {
        lg.writeLine("打开路由配置文件失败:%s", jsonFile.c_str());
        return false;
    }
    nlohmann::json root = nlohmann::json::parse(ifs);
    if (!root.is_array())
    {
        lg.writeLine("路由配置文件不是json数组格式");
        return false;
    }
    try
    {
        for (auto &it : root)
        {
            route subobj;
            subobj.srcport = it["srcport"];
            subobj.dstip = it["dstip"];
            subobj.dstport = it["dstport"];
            routes.push_back(subobj);
        }
    }
    catch (const std::exception &e)
    {
        lg.writeLine("解析路由配置文件失败:%s", e.what());
        return false;
    }
    return true;
}

// 2.初始化路由数组的文件描述符
bool initRouteFd()
{
    for (auto &it : routes)
    {
        // 1.创建通信套接字
        int lfd = socket(AF_INET, SOCK_STREAM, 0);
        if (lfd == -1)
        {
            lg.writeLine("创建socket失败");
            return false;
        }
        // 补充路由结构体的文件描述符
        it.socketfd = lfd;

        sockaddr_in caddr;
        caddr.sin_family = AF_INET;                // IPv4
        caddr.sin_port = htons(it.srcport);        // 小端转大端,绑定传入的端口
        caddr.sin_addr.s_addr = htonl(INADDR_ANY); // 本地任意IP
        // 2.绑定本地IP和端口
        int bindret = bind(it.socketfd, (sockaddr *)&caddr, sizeof(caddr));
        // 3.监听来自客户端的套接字
        int listenret = listen(it.socketfd, 128);
        if (bindret == -1 || listenret == -1)
        {
            lg.writeLine("初始化监听失败：ip：%s，port：%s", it.dstip.c_str(), to_string(it.srcport).c_str());
            return false;
        }
        // 4.设置改路由的监听socket为非阻塞
        setnonblocking(it.socketfd);
    }
    return true;
}

// 3.创建epoll描述符并将路由数组的监听socket加入epoll
bool initEpollFd()
{
    // 1.创建epoll描述符
    epfd = epoll_create1(0);
    // 2.将路由数组的监听socket加入epoll
    for (auto &it : routes)
    {
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = it.socketfd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, it.socketfd, &ev) == -1)
        {
            return false;
        }
    }
    return true;
}

// 4.开启epoll事件循环
bool epollLoop()
{
    struct epoll_event events[1024];
    int size = sizeof(events) / sizeof(events[0]);
    while (true)
    {
        // 1.等待epoll事件
        int eventCount = epoll_wait(epfd, events, size, -1);
        if (eventCount == -1)
        {
            lg.writeLine("epoll_wait失败");
            return false;
        }
        // 2.遍历epoll事件
        for (int i = 0; i < eventCount; i++)
        {
            int curfd = events[i].data.fd;
            // 3.判断是否是监听描述符有连接事件
            int j = 0;
            for (j = 0; j < routes.size(); j++)
            {
                if (curfd == routes[j].socketfd)
                {
                    // 3.1说明源端口处有客户端连接，处理连接
                    if (!acceptAndConnect(routes[j]))
                    {
                        break;
                    }
                }
            }
            if (j < routes.size())
            {
                continue; // 说明上面已经把监听描述符处理完了，不继续向下执行
            }
            //---------------------------------------------
            // 代理转发的正式处理
            // 4.epoll检测到读事件
            // 也就是收到了源端口期望对目标服务器的数据
            if (events[i].events & EPOLLIN)
            {
                if (!recvData(curfd))
                {
                    continue; // 连接断开或者没有数据的情况
                }
            }
            // 5.epoll检测到写事件
            // 代理程序将数据转发给目标服务器
            if (events[i].events & EPOLLOUT)
            {
                if (!sendData(curfd))
                {
                    continue; // 连接断开或者没有数据的情况
                }
            }
        }
    }
    return true;
}

// 4.3.1子函数，处理客户端的连接，的同时自己也要连接目标服务器，形成 客户端-代理程序-目标服务器的连接
// 当自己连接目标服务器时，自身已经作为客户端
bool acceptAndConnect(route &it)
{
    // 1.创建客户端地址结构体
    sockaddr_in caddr;
    socklen_t addrlen = sizeof(caddr);
    // 2.接受客户端连接
    int cfd = accept(it.socketfd, (sockaddr *)&caddr, &addrlen);
    // 非阻塞accept，如果没有连接请求，返回-1（失败），errno=EAGAIN或者EWOULDBLOCK
    if (cfd == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return true; // 没有连接请求，返回true
        }
        else
        {
            return false;
        }
    }
    //---------------------------------------------------------------------------------------------------------
    // 3.自己作为客户端连接目标服务器
    // 3.1创建通信套接字
    int pfd = socket(AF_INET, SOCK_STREAM, 0);
    if (pfd == -1)
    {
        lg.writeLine("创建socket失败");
        close(cfd);
        return false;
    }
    // 设置非阻塞
    setnonblocking(pfd);
    // 3.2初始化sockaddr_in结构体
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(it.dstport);                           // 连接目标端口
    inet_pton(AF_INET, it.dstip.c_str(), &saddr.sin_addr.s_addr); // 连接目标ip，小端转大端
    // 3.3连接目标服务器，非阻塞模式下，connect函数会立即返回失败（-1），不会阻塞，并将errno设置为EINPROGRESS
    // 这里必须设置非阻塞，否则会阻塞在connect函数上，导致epoll时间循环被阻塞
    // 可以引入线程解决该问题
    int ret;
    ret = connect(pfd, (sockaddr *)&saddr, sizeof(saddr));
    if (ret == -1)
    {
        if (errno != EINPROGRESS)
        {
            lg.writeLine("客户端（socket=%d）连接目标服务器失败", cfd);
            close(cfd);
            return false;
        }
    }
    // 3.4判断是否连接成功
    pollfd fds;
    fds.fd = pfd;
    fds.events = POLLOUT;
    poll(&fds, 1, -1);
    if (fds.revents != POLLOUT)
    {
        lg.writeLine("客户端（socket=%d）连接目标服务器失败", cfd);
        close(cfd);
        return false;
    }
    // 此时双方已经连接成功，已经获取了客户端--代理服务器--目标服务器的连接描述符
    // 使用map关联客户端和目标服务器
    // 4.形成 客户端-目标服务器的套接字映射
    proxyMap[cfd] = pfd;
    proxyMap[pfd] = cfd;
    // 5.将客户端的文件描述和连接目标服务器的文件描述符加入epoll监听
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = cfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) == -1)
    {
        return false;
    }
    ev.events = EPOLLIN;
    ev.data.fd = pfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, pfd, &ev) == -1)
    {
        return false;
    }
    return true;
}

// 4.4子函数，接收数据并且将数据保存在该socket的对应缓冲区中，等待epoll写事件的发送
bool recvData(int curfd)
{
    char rdbuffer[10240];
    int buflen;
    // 4.1接收数据
    buflen = recv(curfd, rdbuffer, sizeof(rdbuffer), 0);
    if (buflen <= 0)
    {
        // 4.1a连接断开
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 暂时没有数据，非错误情况，由非阻塞io造成
            return false; // 原continue
        }
        lg.writeLine("客户端（socket=%d）连接断开", curfd);
        // 获取当前客户端fd对应的目标服务器fd
        int pfd = proxyMap[curfd];
        // 关闭两段的套接字
        close(curfd);
        close(pfd);
        // 从map中删除客户端套接字和目标服务器的套接字记录
        proxyMap.erase(curfd);
        proxyMap.erase(pfd);
        // 删除套接字-缓冲区的映射
        socketBufferMap.erase(curfd);
        socketBufferMap.erase(pfd);
        // 从epoll中删除
        epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);
        epoll_ctl(epfd, EPOLL_CTL_DEL, pfd, NULL);
        return false; // 原continue
    }
    // 4.2成功读取数据，将读到的数据保存在当前套接字的缓冲区中
    lg.writeLine("来自%d的%d字节的数据", curfd, buflen);
    // 4.2.1将数据添加进缓冲区
    socketBufferMap[curfd].append(rdbuffer, buflen);
    // 4.2.2修改该通信描述符的epoll事件，添加EPOLLOUT事件，等待下一次epoll检测发送
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT; // 添加写事件
    ev.data.fd = curfd;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, curfd, &ev) == -1)
    {
        return false;
    }
    return true;
}

// 4.5子函数，发送数据到写事件的socket中
bool sendData(int curfd)
{
    // 1.根据map获取当前socket的对应缓冲区的引用
    string &buffer = socketBufferMap[curfd];
    // 2.发送数据，并且记录下已经发送的字节
    int writen = send(curfd, buffer.data(), buffer.size(), 0);
    // 3.删除缓冲区中已经发送的内容
    buffer.erase(0, writen);
    // 4.如果发送缓冲区已经为空
    if (buffer.empty())
    {
        // 从epoll中删除写事件
        struct epoll_event ev;
        ev.events = EPOLLIN; // 添加写事件
        ev.data.fd = curfd;
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, curfd, &ev) == -1)
        {
            return false;
        }
    }
    return true;
}

bool setnonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        return false;
    }
    return true;
}

void help()
{
    cout << "本程序是正向代理程序，用于将本地的请求转发到远程服务器上" << endl;
    cout << "Usage: inetd <logfile> <jsonfile> <timeout>" << endl;
    cout << "logfile：用于保存转发运行日志的文件" << endl;
    cout << "jsonfile：用于保存转发源端口(srcport)，目标服务器ip(dstip)，目标服务器端口(dstport)的json数组文件" << endl;
    cout << "timeout：进程心跳超时的时间" << endl;
}

void EXIT(int sig)
{
    for(auto &it : routes)
    {
        close(it.socketfd);
    }
    close(epfd);
    for (auto &it : proxyMap)
    {
        close(it.first);
        close(it.second);
    }
    lg.writeLine("程序被信号中断，sig=%d", sig);
    exit(0);
}