#include "cpublic.h"
#include "fileframe/include/logfile.h"
#include "procheart/include/procheart.h"
#include "stringop/include/stringop.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
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
procHeart ph;
// 进程心跳时间，单位秒
int phtimeout;
// epoll描述符
int epfd = -1;
// 外网反向代理程序(cfd)-目标内网服务器(pfd)的套接字映射map
unordered_map<int, int> proxyMap; //**程序中的cfd和pfd是对应的，函数处理时他们可以互为对方 */
// 套接字-数据缓冲区的映射map
map<int, string> socketBufferMap;
// 外-内反向代理程序的连接的通信套接字
int cmdConnectScoket; // 这个套接字加入epoll，目的是检测命令通道的传输连接是否被对端断开
int cmdPort;          // 与外网反向代理程序连接的端口
string cmdIp;         // 与外网反向代理程序连接的ip

// 程序接受信号退出的函数
void EXIT(int sig);
// 程序的帮助文档
void help();
// 设置非阻塞
bool setnonblocking(int fd);
// 1.连接外网反向代理程序
bool connectOutside();
// 2.创建epoll描述符并将命令通道的socket加入epoll
bool initEpollFd();
// 3.开启epoll事件循环
bool epollLoop();
// 3.3子函数，判断外网代理服务器是否发出连接申请，命令通道仅仅由外网代理程序单方面发送数据到内网代理程序
// 内网代理程序接受到该信息后可以从信息中获取要连接的内网服务器ip和端口，然后以此同时对目标服务器发送连接申请
// 又对外网代理程序发送连接申请，正式形成 外网代理程序-内网代理程序-目标服务器的连接
bool connection();
// 3.4接受来自外网代理程序或者内网目标服务器的数据，保存在套接字对应的缓冲区中，等待epoll写事件的发送
bool recvData(int curfd);
// 3.5子函数，发送数据到写事件的socket中
bool sendData(int curfd);

// 通用函数：关闭两段的套接字并且删除它们的存储信息
void closeAndDelete(int cfd);

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        help();
        return 0;
    }
    cmdIp = argv[1];
    cmdPort = atoi(argv[2]);
    // 防止信号影响
    closeiosignal(false);
    // 让2号信号和15号信号都调用EXIT函数
    // 2号信号是Ctrl+C，15号信号是kill命令
    signal(2, EXIT);
    signal(15, EXIT);

    // 打开日志文件
    if (!lg.open(argv[3], ios::out))
    {
        cout << "打开日志文件失败！" << endl;
        return 0;
    }
    phtimeout = atoi(argv[4]);
    // 初始化进程心跳
    ph.addProcInfo(getpid(), argv[0], phtimeout);
    // 1.连接外网反向代理程序
    if (!connectOutside())
    {
        EXIT(-1);
    }
    // 2.初始化epoll描述符，并且将内网外命令通道的文件描述符加入epoll
    if (!initEpollFd())
    {
        EXIT(-1);
    }
    // 3.开启epoll事件循环
    if (!epollLoop())
    {
        EXIT(-1);
    }
    return 0;
}

// 1.连接外网反向代理程序
bool connectOutside()
{
    // 1.1创建socket
    cmdConnectScoket = socket(AF_INET, SOCK_STREAM, 0);
    if (cmdConnectScoket == -1)
    {
        lg.writeLine("创建连接外网代理程序的socket失败");
        return false;
    }
    // 1.2构建sockaddr_in结构体
    sockaddr_in cmdaddr;
    cmdaddr.sin_family = AF_INET;
    cmdaddr.sin_port = htons(cmdPort);
    inet_pton(AF_INET, cmdIp.c_str(), &cmdaddr.sin_addr.s_addr);
    // 1.3连接外网反向代理程序，阻塞连接
    if (connect(cmdConnectScoket, (sockaddr *)&cmdaddr, sizeof(cmdaddr)) == -1)
    {
        lg.writeLine("连接外网反向代理程序失败");
        return false;
    }
    // 1.3设置非阻塞
    setnonblocking(cmdConnectScoket);
    lg.writeLine("与外网建立命令通道成功：%d", cmdConnectScoket);
    return true;
}

// 2.创建epoll描述符并将命令通道的socket加入epoll
bool initEpollFd()
{
    // 2.1创建epoll描述符
    epfd = epoll_create1(0);
    // 2.2将与外网连接的命令通道的socket加入epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = cmdConnectScoket;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, cmdConnectScoket, &ev) == -1)
    {
        return false;
    }
    return true;
}

// 3.开启epoll事件循环
bool epollLoop()
{
    struct epoll_event events[1024];
    int size = sizeof(events) / sizeof(events[0]);
    while (true)
    {
        // 3.1等待epoll事件
        int eventCount = epoll_wait(epfd, events, size, 15);
        if (eventCount == -1)
        {
            lg.writeLine("epoll_wait失败");
            return false;
        }
        ph.updateHeart(); // 更新进程心跳
        // 3.2遍历epoll事件
        for (int i = 0; i < eventCount; i++)
        {
            int curfd = events[i].data.fd;
            // 3.3判断命令通道是否有连接事件
            if (curfd == cmdConnectScoket)
            {
                connection();
                continue;
            }
            //---------------------------------------------
            // 代理转发的正式处理
            // 3.4.epoll检测到读事件
            // 也就是收到了源端口期望对目标服务器的数据
            if (events[i].events & EPOLLIN)
            {
                if (!recvData(curfd))
                {
                    continue; // 连接断开或者没有数据的情况
                }
            }
            // 3.5.epoll检测到写事件
            // 代理程序将数据转发给目标服务器
            if (events[i].events & EPOLLOUT)
            {
                if (!sendData(curfd))
                {
                    continue; // 连接断开或者没有数据的情况
                }
            }
            // 3.a如果对端错误的断开连接
            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP)
            {
                // 关闭连接，并且删除客户端-目标服务器的连接信息
                lg.writeLine("套接字端(socket=%d)遇到错误", curfd);
                closeAndDelete(curfd);
            }
        }
    }
    return true;
}

// 3.3子函数，判断外网代理服务器是否发出连接申请，命令通道仅仅由外网代理程序单方面发送数据到内网代理程序
// 内网代理程序接受到该信息后可以从信息中获取要连接的内网服务器ip和端口，然后以此同时对目标服务器发送连接申请
// 又对外网代理程序发送连接申请，正式形成 外网代理程序-内网代理程序-目标服务器的连接
bool connection()
{
    char buffer[1024];
    int buflen = 0;
    // 3.3.1接受来自命令通道的数据，也就是外网代理程序发来的ip和端口的json字符串
    // 这里json字符串可能分包/粘包，现在暂时不解决
    buflen = recv(cmdConnectScoket, buffer, sizeof(buffer), 0);
    if (buflen <= 0)
    {
        // 3.3.1a连接断开
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 暂时没有数据，非错误情况，由非阻塞io造成
            return false; // 原continue
        }
        lg.writeLine("命令通道断开连接");
        EXIT(-1);
    }
    // 3.3.2成功读取数据，解析json字符串
    string dstip;
    int dstport;
    string jsonstr(buffer, buflen);
    try
    {
        nlohmann::json json = nlohmann::json::parse(jsonstr);
        dstip = json["dstip"];
        dstport = json["dstport"];
        lg.writeLine("命令通道接收到连接请求，目标ip=%s，端口=%d", dstip.c_str(), dstport);
    }
    catch (const std::exception &e)
    {
        lg.writeLine("json解析失败");
        return false;
    }
    //---------------------------------------------------------------------------------
    // 3.3.3连接内网的目标服务器
    // 创建内网目标服务器的socket
    int pfd = socket(AF_INET, SOCK_STREAM, 0);
    if (pfd == -1)
    {
        lg.writeLine("创建内网目标服务器的socket失败");
        return false;
    }
    // 设置非阻塞
    setnonblocking(pfd);
    // 构建sockaddr_in结构体
    sockaddr_in paddr;
    paddr.sin_family = AF_INET;
    paddr.sin_port = htons(dstport);
    inet_pton(AF_INET, dstip.c_str(), &paddr.sin_addr.s_addr);
    // 连接内网的目标服务器
    if (connect(pfd, (sockaddr *)&paddr, sizeof(paddr)) == -1)
    {
        if (errno != EINPROGRESS)
        {
            lg.writeLine("连接内网目标服务器失败");
            close(pfd);
            return false;
        }
    }
    // 是否连接成功
    pollfd fds;
    fds.fd = pfd;
    fds.events = POLLOUT;
    poll(&fds, 1, -1);
    if (fds.revents != POLLOUT)
    {
        lg.writeLine("连接内网目标服务器失败");
        close(pfd);
        return false;
    }
    //---------------------------------------------------------------------------------
    // 3.3.4搭建外网代理程序的传输通道
    // 创建外网代理程序的socket
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1)
    {
        lg.writeLine("创建外网代理程序数据通道的socket失败");
        close(pfd);
        return false;
    }
    // 设置非阻塞
    setnonblocking(cfd);
    // 构建sockaddr_in结构体
    sockaddr_in cddr;
    cddr.sin_family = AF_INET;
    cddr.sin_port = htons(cmdPort);
    inet_pton(AF_INET, cmdIp.c_str(), &cddr.sin_addr.s_addr);
    // 连接外网反向代理程序
    if (connect(cfd, (sockaddr *)&cddr, sizeof(cddr)) == -1)
    {
        if (errno != EINPROGRESS)
        {
            lg.writeLine("连接外网代理程序数据通道失败");
            close(cfd);
            close(pfd);
            return false;
        }
    }
    // 是否连接成功
    pollfd cfds;
    cfds.fd = cfd;
    cfds.events = POLLOUT;
    poll(&cfds, 1, -1);
    if (cfds.revents != POLLOUT)
    {
        lg.writeLine("连接外网代理程序数据通道失败");
        close(cfd);
        close(pfd);
        return false;
    }
    //---------------------------------------------------------------------------------
    // 3.3.5形成 外网代理服务器-内网目标服务器 的套接字映射
    proxyMap[cfd] = pfd; // 外网代理程序-内网目标服务器
    proxyMap[pfd] = cfd; // 内网目标服务器-外网代理程序
    // 3.3.6将外网代理程序的文件描述和连接目标服务器的文件描述符加入epoll监听
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = cfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) == -1)
    {
        closeAndDelete(cfd);
        closeAndDelete(pfd);
        return false;
    }
    ev.events = EPOLLIN;
    ev.data.fd = pfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, pfd, &ev) == -1)
    {
        closeAndDelete(cfd);
        closeAndDelete(pfd);
        return false;
    }
    // 至此，已经形成了 外网代理程序-内网代理程序-内网目标服务器的连接关系
    return true;
}

// 3.4接受来自外网代理程序或者内网目标服务器的数据，保存在套接字对应的缓冲区中，等待epoll写事件的发送
bool recvData(int curfd)
{
    char rdbuffer[10240];
    int buflen;
    // 3.4.1接收数据
    buflen = recv(curfd, rdbuffer, sizeof(rdbuffer), 0);
    if (buflen <= 0)
    {
        // 3.4.1a连接断开
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 暂时没有数据，非错误情况，由非阻塞io造成
            return false; // 原continue
        }
        // 关闭外网代理程序-内网目标服务器的通信描述符并且移除资源
        closeAndDelete(curfd);
        return false; // 原continue
    }
    // 3.4.2成功读取数据，将读到的数据保存在当前套接字的缓冲区中
    lg.writeLine("来自%d的%d字节的数据", curfd, buflen);
    // 3.4.2.1将数据添加进对端的缓冲区
    int pfd = proxyMap[curfd]; // 获取对端socket
    socketBufferMap[pfd].append(rdbuffer, buflen);
    // 3.4.2.2修改该通信描述符的epoll事件，添加EPOLLOUT事件，等待下一次epoll检测发送
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT; // 添加写事件
    ev.data.fd = pfd;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, pfd, &ev) == -1)
    {
        return false;
    }
    return true;
}

// 3.5子函数，发送数据到写事件的socket中
bool sendData(int curfd)
{
    // 3.5.1.根据map获取当前socket的对应缓冲区的引用
    string &buffer = socketBufferMap[curfd];
    // 3.5.2.发送数据，并且记录下已经发送的字节
    int writen = send(curfd, buffer.data(), buffer.size(), 0);
    if (writen <= 0)
    {
        // 3.5.2a连接断开
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 发送缓冲区已满，稍后再试
            return false; // 原continue
        }
        // 关闭客户端-目标服务器的通信描述符并且移除资源
        closeAndDelete(curfd);
        return false; // 原continue
    }
    // 3.5.2b.发送成功，打印日志
    lg.writeLine("向%d发送了%d字节的数据", curfd, writen);
    // 3.5.3.删除缓冲区中已经发送的内容
    buffer.erase(0, writen);
    // 3.5.4.如果发送缓冲区已经为空
    if (buffer.empty())
    {
        // 从epoll中删除写事件
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = curfd;
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, curfd, &ev) == -1)
        {
            return false;
        }
    }
    return true;
}

// 通用函数：关闭两段的套接字并且删除它们的存储信息
void closeAndDelete(int cfd)
{
    // 查找是否有映射关系
    auto it = proxyMap.find(cfd);
    // 没有则只处理客户端一个通信描述符
    if (it == proxyMap.end())
    {
        //lg.writeLine("套接字端(socket=%d)连接断开", cfd);
        // 关闭通信描述符
        close(cfd);
        // 删除缓冲区信息
        socketBufferMap.erase(cfd);
        // 从epoll中删除
        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
        return;
    }
    else // 说明存在映射关系
    {
        // 获取当前客户端fd对应的目标服务器fd
        int pfd = proxyMap[cfd];
        lg.writeLine("套接字端(socket=%d)连接断开", cfd);
        lg.writeLine("套接字端(socket=%d)连接断开", pfd);
        // 关闭两段的套接字
        close(cfd);
        close(pfd);
        // 从map中删除客户端套接字和目标服务器的套接字记录
        proxyMap.erase(cfd);
        proxyMap.erase(pfd);
        // 删除套接字-缓冲区的映射
        socketBufferMap.erase(cfd);
        socketBufferMap.erase(pfd);
        // 从epoll中删除
        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
        epoll_ctl(epfd, EPOLL_CTL_DEL, pfd, NULL);
    }
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
    cout << "本程序是反向代理的内网部署程序，用于搭建外网代理程序-内网目标服务器的中间桥梁，形成外网代理程序-"
            "内网代理程序-内网目标服务器的反向代理模型"
         << endl;
    cout << "Usage: rinetdin <cmdip> <cmdport> <logfile> <timeout>" << endl;
    cout << "example: ./rinetdin 127.0.0.1 5180 /temp/log/rinetdin.log 60" << endl;
    cout << "cmdip：外网反向代理程序ip" << endl;
    cout << "cmdport：外网反向代理服务器连接端口" << endl;
    cout << "logfile：用于保存转发运行日志的文件" << endl;
    cout << "timeout：进程心跳超时的时间" << endl;
    cout << "PS：该程序应该由root用户启动，以获取代理最大权限。" << endl;
}

void EXIT(int sig)
{
    close(epfd);
    close(cmdConnectScoket);
    for (auto &it : proxyMap)
    {
        close(it.first);
        close(it.second);
    }
    lg.writeLine("程序被信号中断，sig=%d", sig);
    exit(0);
}