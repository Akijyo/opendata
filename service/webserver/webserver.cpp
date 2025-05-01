#include "webserver.h"
#include "include/threadpool.h"
#include <chrono>
#include <cstdlib>
#include <thread>

using namespace std;

// 创建日志文件全局对象
logfile lg;
// 创建全局线程池对象
ThreadPool *threadpool = ThreadPool::getThreadPool(10, 80);
// 创建全局webserver对象
webserver web(lg, threadpool);

void EXIT(int sig);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "usage: " << argv[0] << " <port> <logfile>" << endl;
        cout << "example:/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 60"
             << " /home/akijyo/桌面/code/c++/opendata/bin/webserver"
             << " 8080"
                " /temp/log/webserver.log"
             << endl;

        cout << "port:服务器监听端口" << endl;
        cout << "logfile:日志文件名" << endl;
        cout << "---------------------------------------------------" << endl;
        cout << "本程序实现了一个外部访问数据的web服务器" << endl;
        cout << "通过浏览器等http客户端使用url访问" << endl;
        exit(0);
    }
    closeiosignal(false);

    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    if (!lg.open(argv[2]))
    {
        cout << "日志文件打开失败" << endl;
        return 0;
    }
    thread recvThread(&webserver::recvThread, &web, atoi(argv[1]));
    thread sendThread(&webserver::sendThread, &web);
    // 等待线程结束
    recvThread.join();
    sendThread.join();
    return 0;
}

void EXIT(int sig)
{
    // 不要显式调用析构函数
    // web.~webserver(); // 删除这一行

    // 通知webserver退出
    write(web.recvPipe[1], (char *)"e", 1);

    // 记录退出信息
    lg.writeLine("收到退出信号 %d，webserver正在退出", sig);

    // 给线程一些时间处理退出操作
    sleep(1);

    // 直接退出程序
    exit(0);
}