//这是测试二进制数据传输的服务端文件
#include "include/TcpServer.h"
using namespace std;

struct student
{
    int id;
};

void working(TcpServer &server, shared_ptr<clientNode> client)
{
    if (client->is_active)
    {
        // 接收消息
        MessageType type;
        struct student stu;
        if(!server.recvMsgBin(&stu, client, type))
        {
            return;
        }
        // 解析消息类型
        if (type == MessageType::Heart)
        {
            cout << client->cfd << "（心跳包）"  << endl;
            client->resetCount();
            server.sendMsgBin(client, &stu, sizeof(stu), MessageType::Heart);
        }
        else
        {
            server.sendMsgBin(client, &stu, sizeof(stu), MessageType::Data);
            cout << client->cfd << "（数据包）" << endl;
        }
    }
}

void selectThread(TcpServer &server, ThreadPool *pool)
{
    server.epollSocket(pool, [&server](shared_ptr<clientNode> client) -> void * {
        working(server, client);
        return nullptr;
    });
}

int main()
{
    // 创建服务器对象
    TcpServer server;

    // 1.创建监听
    server.createListen(8888);

    // 2.创建线程池
    ThreadPool *pool = ThreadPool::getThreadPool(50, 1000);

    // 3.启动 select 监听线程
    pool->addTask([&server, pool]() -> void * {
        selectThread(server, pool);
        return nullptr;
    });

    // 4.启动心跳包检查线程
    pool->addTask([&server]() -> void * {
        server.heartbeatThread(5);
        return nullptr;
    });
    // 5.等待全部线程完成
    pool->waitAllTasksDone();
    return 0;
}