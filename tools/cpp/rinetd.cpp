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
procHeart ph;
// 进程心跳时间，单位秒
int phtimeout;
// epoll描述符
int epfd = -1;
// 客户端-内网反向代理服务器的套接字映射map
unordered_map<int, int> proxyMap; //**程序中的cfd和pfd是对应的，函数处理时他们可以互为对方 */
// 套接字-数据缓冲区的映射map
map<int, string> socketBufferMap;
// 外-内反向代理程序的连接的监听套接字和通信套接字
int cmdListenSocket;  // 这个套接字不加入epoll
int cmdConnectScoket; // 这个套接字加入epoll，目的是检测命令通道的传输连接是否被对端断开
int cmdPort;          // 与内网反向代理程序连接的端口

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
// 3.连接内网程序
bool connectInside();
// 4.创建epoll描述符并将路由数组的监听socket和内网连接socket加入epoll
bool initEpollFd();
// 5.开启epoll事件循环
bool epollLoop();
// 5.3.1子函数，处理客户端的连接，将连接目的发给内网代理程序，然后等待内网代理程序建立连接，形成
// 客户端-外网反向代理程序服务器-内网反向代理服务器 的连接 当自己连接目标服务器时，自身已经作为客户端
bool acceptAndConnect(route &it);
// 5.4.a如果命令通道断开连接
bool cmdDisconnect();
// 5.4接受来自客户端或者内网服务器的数据，保存在套接字对应的缓冲区中，等待epoll写事件的发送
bool recvData(int curfd);
// 5.5子函数，发送数据到写事件的socket中
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
    cmdPort = atoi(argv[1]);
    // 防止信号影响
    closeiosignal(false);
    // 让2号信号和15号信号都调用EXIT函数
    // 2号信号是Ctrl+C，15号信号是kill命令
    signal(2, EXIT);
    signal(15, EXIT);

    // 打开日志文件
    if (!lg.open(argv[2], ios::out))
    {
        cout << "打开日志文件失败！" << endl;
        return 0;
    }
    phtimeout = atoi(argv[4]);
    // 初始化进程心跳
    ph.addProcInfo(getpid(), argv[0], phtimeout);
    // 1.解析转发路由
    if (!parseRoute(argv[3]))
    {
        lg.writeLine("解析路由配置文件失败");
        return 0;
    }
    // 2.初始化对客户端端口的监听
    if (!initRouteFd())
    {
        EXIT(-1);
    }
    // 3.连接内网反向代理程序
    if (!connectInside())
    {
        EXIT(-1);
    }
    // 4.初始化epoll描述符
    if (!initEpollFd())
    {
        EXIT(-1);
    }
    // 5.开启epoll事件循环
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
        // 2.1.创建通信套接字
        int lfd = socket(AF_INET, SOCK_STREAM, 0);
        if (lfd == -1)
        {
            lg.writeLine("初始化路由数组中，创建socket失败");
            return false;
        }
        // 端口复用
        int opt = 1;
        unsigned int len = sizeof(opt);
        setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
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
            lg.writeLine("初始化监听客户端失败：ip：%s，port：%s", it.dstip.c_str(), to_string(it.srcport).c_str());
            return false;
        }
        // 4.设置改路由的监听socket为非阻塞
        setnonblocking(it.socketfd);
    }
    return true;
}

// 3.连接内网程序
bool connectInside()
{
    // 3.1创建监听套接字
    cmdListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (cmdListenSocket == -1)
    {
        lg.writeLine("连接内网程序中，创建socket失败");
        return false;
    }
    // 端口复用
    int opt = 1;
    unsigned int len = sizeof(opt);
    setsockopt(cmdListenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, len);
    // 3.2绑定本地ip和端口
    sockaddr_in laddr;
    laddr.sin_family = AF_INET;                // IPv4
    laddr.sin_port = htons(cmdPort);           // 小端转大端,绑定传入的端口
    laddr.sin_addr.s_addr = htonl(INADDR_ANY); // 本地任意IP
    int bindret = bind(cmdListenSocket, (sockaddr *)&laddr, sizeof(laddr));
    // 3.3设置监听
    int listenret = listen(cmdListenSocket, 5);
    if (bindret == -1 || listenret == -1)
    {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &laddr.sin_addr, ip_str, INET_ADDRSTRLEN);
        lg.writeLine("初始化监听内网失败：ip：%s，port：%s", ip_str, to_string(cmdPort).c_str());
        return false;
    }
    // 3.4阻塞等待内网代理程序连接
    sockaddr_in inaddr;
    socklen_t addrlen = sizeof(inaddr);
    cmdConnectScoket = accept(cmdListenSocket, (sockaddr *)&inaddr, &addrlen);
    if (cmdConnectScoket == -1)
    {
        lg.writeLine("等待内网代理程序连接失败");
        return false;
    }
    lg.writeLine("与内网建立命令通道成功：%d", cmdConnectScoket);
    // 3.5设置非阻塞
    setnonblocking(cmdConnectScoket);
    return true;
}

// 4.创建epoll描述符并将路由数组的监听socket和内网连接socket加入epoll
bool initEpollFd()
{
    // 4.1创建epoll描述符
    epfd = epoll_create1(0);
    // 4.2将路由数组的监听socket加入epoll
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
    // 4.3将内网连接socket加入epoll
    // 将这个连接加入epoll的目的是检测内网程序的连接是否断开
    // 连接断开会触发读时间并且recv接收到0字节
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = cmdConnectScoket;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, cmdConnectScoket, &ev) == -1)
    {
        return false;
    }
    return true;
}

// 5.开启epoll事件循环
bool epollLoop()
{
    struct epoll_event events[1024];
    int size = sizeof(events) / sizeof(events[0]);
    while (true)
    {
        // 5.1等待epoll事件
        int eventCount = epoll_wait(epfd, events, size, 15);
        if (eventCount == -1)
        {
            lg.writeLine("epoll_wait失败");
            return false;
        }
        ph.updateHeart(); // 更新进程心跳
        // 5.2遍历epoll事件
        for (int i = 0; i < eventCount; i++)
        {
            int curfd = events[i].data.fd;
            // 5.3判断是否是监听描述符有连接事件
            int j = 0;
            for (j = 0; j < routes.size(); j++)
            {
                if (curfd == routes[j].socketfd)
                {
                    // 5.3.1说明源端口处有客户端连接，处理连接
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
            // 5.4.epoll检测到读事件
            // 也就是收到了源端口期望对目标服务器的数据
            if (events[i].events & EPOLLIN)
            {
                // 5.4.a如果命令通道断开连接
                if (curfd == cmdConnectScoket)
                {
                    if (!cmdDisconnect())
                    {
                        EXIT(-1); // 连接断开
                    }
                }
                if (!recvData(curfd))
                {
                    continue; // 连接断开或者没有数据的情况
                }
            }
            // 5.5.epoll检测到写事件
            // 代理程序将数据转发给目标服务器
            if (events[i].events & EPOLLOUT)
            {
                if (!sendData(curfd))
                {
                    continue; // 连接断开或者没有数据的情况
                }
            }
            // 5.a如果对端错误的断开连接
            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP)
            {
                // 关闭连接，并且删除客户端-目标服务器的连接信息
                closeAndDelete(curfd);
            }
        }
    }
    return true;
}

// 5.3.1子函数，处理客户端的连接，的同时自己也要连接目标服务器，形成 客户端-代理程序-目标服务器的连接
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
    // 3.通过命令通道，向内网代理服务器发送连接的目的和端口
    // 3.1组织连接信息的json
    nlohmann::json json;
    json["dstip"] = it.dstip;
    json["dstport"] = it.dstport;
    string msg = json.dump();
    // 3.2发送连接信息
    if (send(cmdConnectScoket, msg.c_str(), msg.size(), 0) == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 发送缓冲区已满，稍后再试
            return false;
        }
        lg.writeLine("在接受客户端连接时，通过命令通道向内网代理程序发送连接信息失败");
        close(cfd);
        EXIT(-1);
    }
    //---------------------------------------------------------------------------------------------------------
    // 4.等待内网代理服务器的连接
    // 4.1创建内网代理服务器地址结构体
    sockaddr_in paddr;
    socklen_t addrlen2 = sizeof(paddr);
    // 4.2阻塞等待内网代理服务器连接
    int pfd = accept(cmdListenSocket, (sockaddr *)&paddr, &addrlen2);
    if (pfd == -1)
    {
        lg.writeLine("等待内网代理程序连接数据通道失败");
        close(cfd);
        return false;
    }
    // 4.3对向 客户端-内网代理服务器的这条连接设置非阻塞
    setnonblocking(cfd);
    setnonblocking(pfd);
    // 5.形成 客户端-内网代理服务器的套接字映射
    proxyMap[cfd] = pfd;
    proxyMap[pfd] = cfd;
    // 6.将客户端的文件描述和连接目标服务器的文件描述符加入epoll监听
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
    return true;
}

// 5.4.a如果命令通道断开连接
bool cmdDisconnect()
{
    char buffer[1024];
    int nread = recv(cmdConnectScoket, buffer, sizeof(buffer), 0);

    if (nread == 0)
    {
        // 确实是连接断开
        lg.writeLine("命令通道断开连接");
        return false;
    }
    else if (nread < 0)
    {
        // 错误情况
        lg.writeLine("命令通道出错: %s", strerror(errno));
        return false;
    }
    else
    {
        // 意外收到数据
        lg.writeLine("命令通道意外收到%d字节数据", nread);
        // 可以选择忽略或退出
    }
    return true;
}

// 5.4接受来自客户端或者内网服务器的数据，保存在套接字对应的缓冲区中，等待epoll写事件的发送
bool recvData(int curfd)
{
    char rdbuffer[10240];
    int buflen;
    // 5.4.1接收数据
    buflen = recv(curfd, rdbuffer, sizeof(rdbuffer), 0);
    if (buflen <= 0)
    {
        // 5.4.1a连接断开
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 暂时没有数据，非错误情况，由非阻塞io造成
            return false; // 原continue
        }
        lg.writeLine("客户端（socket=%d）连接断开", curfd);
        // 关闭客户端-目标服务器的通信描述符并且移除资源
        closeAndDelete(curfd);
        return false; // 原continue
    }
    // 5.4.2成功读取数据，将读到的数据保存在当前套接字的缓冲区中
    lg.writeLine("来自%d的%d字节的数据", curfd, buflen);
    // 5.4.2.1将数据添加进对端的缓冲区
    int pfd = proxyMap[curfd]; // 获取对端socket
    socketBufferMap[pfd].append(rdbuffer, buflen);
    // 5.4.2.2修改该通信描述符的epoll事件，添加EPOLLOUT事件，等待下一次epoll检测发送
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT; // 添加写事件
    ev.data.fd = pfd;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, pfd, &ev) == -1)
    {
        return false;
    }
    return true;
}

// 5.5子函数，发送数据到写事件的socket中
bool sendData(int curfd)
{
    // 5.5.1.根据map获取当前socket的对应缓冲区的引用
    string &buffer = socketBufferMap[curfd];
    // 5.5.2.发送数据，并且记录下已经发送的字节
    int writen = send(curfd, buffer.data(), buffer.size(), 0);
    if (writen <= 0)
    {
        // 5.5.2a连接断开
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 发送缓冲区已满，稍后再试
            return false; // 原continue
        }
        lg.writeLine("客户端（socket=%d）连接断开", curfd);
        // 关闭客户端-目标服务器的通信描述符并且移除资源
        closeAndDelete(curfd);
        return false; // 原continue
    }
    // 5.5.2b.发送成功，打印日志
    lg.writeLine("向%d发送了%d字节的数据", curfd, writen);
    // 5.5.3.删除缓冲区中已经发送的内容
    buffer.erase(0, writen);
    // 5.5.4.如果发送缓冲区已经为空
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
    cout << "本程序是反向代理的外网部署程序，用于接收内网服务器请求并将客户端数据反向发送到内网服务器中" << endl;
    cout << "Usage: rinetd <cmdport> <logfile> <jsonfile> <timeout>" << endl;
    cout << "example: ./rinetd 5180 /temp/log/rinetd.log "
            "/home/akijyo/桌面/code/c++/opendata/tools/others/rinetdconf.json 60"
         << endl;
    cout << "cmdport：内网反向代理服务器连接端口" << endl;
    cout << "logfile：用于保存转发运行日志的文件" << endl;
    cout << "jsonfile：用于保存转发源端口(srcport)，目标服务器ip(dstip)，目标服务器端口(dstport)的json数组文件" << endl;
    cout << "timeout：进程心跳超时的时间" << endl;
    cout << "PS：该程序应该由root用户启动，以获取代理最大权限。" << endl;
}

void EXIT(int sig)
{
    // 关闭所有的套接字
    for (auto &it : routes)
    {
        close(it.socketfd);
    }
    close(epfd);
    close(cmdListenSocket);
    close(cmdConnectScoket);
    for (auto &it : proxyMap)
    {
        close(it.first);
        close(it.second);
    }
    lg.writeLine("程序被信号中断，sig=%d", sig);
    exit(0);
}