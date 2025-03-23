// 这是一个用于控制进程的工具，可以用于定期启动进程，监控进程状态等
#include <csignal>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>
#include <chrono>
#include <cstdlib>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "本程序是服务程序的调度程序，周期性启动服务程序或者shell脚本。" << endl;
        cout << "Usage: " << argv[0] << " <timeval> <program> <arg>..." << endl;
        cout << "Example: " << argv[0]
             << " 60 /home/akijyo/桌面/code/c++/opendata/bin/opendata "
                "/temp/data/数据集合/01中国气象站点数据/中国气象站点参数.ini /temp/output/ /temp/log/mylog.log"
             << endl;

        cout << "-----------------------------" << endl;
        cout << "timeval: 间隔时间，单位秒" << endl;
        cout << "----被调度的程序运行结束后，在timetvl秒后会被processctrl重新启动。" << endl;
        cout << "----如果被调度的程序是周期性的任务，timetvl设置为运行周期。" << endl;
        cout << "----如果被调度的程序是常驻内存的服务程序，timetvl设置小于5秒。" << endl;
        cout << "program: 被调度的程序，使用绝对路径" << endl;
        cout << "arg: 被调度的程序的参数" << endl;
        cout << "注意，本程序不会被kill杀死，但可以用kill -9强行杀死。" << endl;
        return 0;
    }

    //关闭io和信号，防止被打扰，同样的由此程序启动的程序也会被影响关闭
    for (int i = 0; i < 64; i++)
    {
        //close(i);
        signal(i, SIG_IGN);
    }
    // 生成子进程，父进程退出，将子进程托管给1号进程，这样父进程退出后将解放shell终端，子进程成为后台进程
    if (fork() != 0)
    {
        exit(0);
    }
    signal(SIGCHLD, SIG_DFL);

    //定义存放托管程序参数的数组
    char *prgar[argc];
    for (int i = 2; i < argc; i++)
    {
        prgar[i - 2] = argv[i];
    }
    prgar[argc - 2] = nullptr;

    while (true)
    {
        //fork返回0代表子进程，大于0代表父进程
        //创建孙子进程（fork==0），孙子进程执行目标程序，子进程等待孙子进程结束
        if (fork() == 0)
        {
            execv(argv[2], prgar);
            exit(0);
        }
        else
        {
            wait(nullptr);//阻塞调度程序进程，等待孙子进程结束
            this_thread::sleep_for(chrono::seconds(atoi(argv[1])));
        }
    }
    return 0;
}

/*
一、进程关系
初始进程（父进程）
    启动时解析命令行参数，检查合法性。
    关闭所有文件描述符（0-63）并忽略所有信号。
    调用 fork() 生成第一个子进程（守护进程），随后初始进程退出。

守护进程（子进程）
    被 init（1号进程）接管，成为后台进程。
    设置 SIGCHLD 信号为默认处理（允许回收子进程）。
    进入主循环，每次循环中：
    调用 fork() 生成新的子进程（任务进程）。
    任务进程调用 execv() 执行目标程序。
    守护进程通过 wait(nullptr) 等待任务进程结束。
    休眠指定时间间隔后重复循环。

任务进程（孙子进程）
    由守护进程的 fork() 创建，负责执行目标程序。
    通过 execv() 替换为新的程序映像。
    执行结束后被守护进程回收。
*/