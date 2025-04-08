#include "cpublic.h"
#include "fileframe/include/logfile.h"
#include "include/TcpServer.h"
#include "include/socket.h"
#include "include/threadpool.h"
#include "procheart/include/procheart.h"
using namespace std;
// 创建日志文件对象
logfile lg;
// 创建进程心跳对象
procHeart ph;
// 进程心跳超时时间
int timeout;
// 创建线程池对象
ThreadPool *pool = ThreadPool::getThreadPool(5, 1000);
// 创建tcp服务器对象
TcpServer server;

// 客户端传入的参数合集
class argss
{
  public:
    int clientType;     // 客户端类型 1.上传 2.下载，默认为1
    string client_path; // 本地文件的目录 /data /data/aaa
    string server_path; // 服务器文件的目录 /data /data/aaa
    string fullpath;    // 客户端传入的文件的完整路径
    string modify_time; // 客户端传入的文件的修改时间
    int filesize;       // 客户端传入的文件的大小
};
// 定义一个全局变量，用于绑定客户端和客户端传入的参数
unordered_map<shared_ptr<clientNode>, argss> clientArgsMap;
mutex mapMtx;
// 开启一个线程，用于定期检测map中的客户端是否超时，以便及时清理内存
void clearMap();

// 程序的信号退出处理函数
void EXIT(int sig);
// 处理通讯的核心回调函数
void handle(shared_ptr<clientNode> client);
// 处理客户端登录的函数，也就是接收客户端参数
void clientLogin(shared_ptr<clientNode> client, argss &fileConnect, string &msg);
// 接收客户端上传文件的核心处理函数
void clientFileTop(shared_ptr<clientNode> client, string &msg);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "Usage: fileserver <logfile>" << endl;
        cout << "example: ./fileserver /temp/log/fileserver.log" << endl;
        cout << "logfile：用于保存文件服务端运行日志的文件" << endl;
        return 0;
    }
    closeiosignal(false);
    signal(2, EXIT);
    signal(15, EXIT);
    // 打开日志文件
    if (!lg.open(argv[1], ios::out | ios::app))
    {
        cerr << "打开日志文件失败" << endl;
        return 0;
    }
    // 初始化服务端
    server.createListen(8888);
    // 开启epoll监听线程
    pool->addTask([]() -> void {
        server.epollSocket(pool, [](shared_ptr<clientNode> client) -> void * {
            handle(client);
            return nullptr;
        });
    });
    // 开启心跳包检查线程，每二十秒检测一次
    pool->addTask([]() -> void { server.heartbeatThread(20); });
    // 开启清理map线程
    pool->addTask([]() -> void { clearMap(); });
    pool->waitAllTasksDone();
    return 0;
}

void handle(shared_ptr<clientNode> client)
{
    if (client->is_active)
    {
        // 接收消息
        MessageType type;
        string msg;
        server.recvMsgWithType(msg, client, type);
        lg.writeLine("接收到来着客户端%d的信息:%s", client->cfd, msg.c_str());
        argss fileConnect;
        // 心跳包的处理逻辑
        if (type == MessageType::Heart)
        {
            // 重置心跳包计数
            client->resetCount();
            // 发送心跳包，不往回发，程序很难写
            // server.sendMsgBin(client, msg.data(), msg.size(), MessageType::Heart);
            lg.writeLine("客户端%d心跳包", client->cfd);
        }
        // 客户端传入参数的解析
        else if (type == MessageType::Login)
        {
            // string msgStr(static_cast<char *>(msg));
            clientLogin(client, fileConnect, msg);
        }
        // 解析文件报头
        else if (type == MessageType::Top)
        {
            // string msgStr(static_cast<char *>(msg));
            clientFileTop(client, msg);
        }
    }
}

void clientLogin(shared_ptr<clientNode> client, argss &fileConnect, string &msg)
{
    nlohmann::json loginJson;
    // 解析json字符串
    try
    {
        loginJson = nlohmann::json::parse(msg);
    }
    catch (const std::exception &e)
    {
        lg.writeLine("解析json字符串失败:%s", e.what());
        return;
    }
    fileConnect.clientType = loginJson["clientType"];
    fileConnect.client_path = loginJson["client_path"];
    fileConnect.server_path = loginJson["server_path"];
    mapMtx.lock();
    clientArgsMap[client] = fileConnect;
    mapMtx.unlock();
    // 发送登录成功的消息
    string loginStr = "login success";
    server.sendMsgWithType(client, loginStr, MessageType::Login);
    lg.writeLine("接收客户端的登录报文:%s", msg.c_str());
}

void clientFileTop(shared_ptr<clientNode> client, string &msg)
{
    nlohmann::json fileJson;
    // 解析json字符串
    try
    {
        fileJson = nlohmann::json::parse(msg);
    }
    catch (const std::exception &e)
    {
        lg.writeLine("解析json字符串失败:%s", e.what());
        return;
    }
    // 获取文件的完整路径
    string fullpath = fileJson["filename"];
    // 获取文件的修改时间
    string modify_time = fileJson["filetime"];
    // 获取文件的大小
    int filesize = fileJson["filesize"];
    mapMtx.lock();
    clientArgsMap[client].fullpath = fullpath;
    clientArgsMap[client].modify_time = modify_time;
    clientArgsMap[client].filesize = filesize;
    mapMtx.unlock();
    lg.writeLine("接收客户端的文件报头:%s", msg);
}

void clearMap()
{
    while (1)
    {
        mapMtx.lock();
        for (auto it = clientArgsMap.begin(); it != clientArgsMap.end();)
        {
            // 若 client 已经断开连接，则释放内存
            if (!it->first->is_active)
            {
                it = clientArgsMap.erase(it); // 使用迭代器版本的 erase，并更新迭代器
            }
            else
            {
                ++it; // 仅在未删除时递增迭代器
            }
        }
        mapMtx.unlock();
        sleep(10);
    }
}

void EXIT(int sig)
{
    exit(0);
}