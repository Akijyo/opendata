#include "cpublic.h"
#include "fileframe/include/cdir.h"
#include "fileframe/include/fileframe.h"
#include "fileframe/include/fileio.h"
#include "fileframe/include/logfile.h"
#include "include/TcpServer.h"
#include "include/socket.h"
#include "include/threadpool.h"
#include "procheart/include/procheart.h"
#include "stringop/include/stringop.h"
using namespace std;

// 该程序是tcp服务端的文件上传下载程序
// 程序的上传逻辑是：
// 1.客户端发送登录报文，服务端接收后解析json字符串，获取客户端的参数，并发送回一个登录确认报文，客户端接受后才可以继续执行下面的操作
// 2.客户端发送文件报头，服务端接收后解析json字符串，获取文件的完整路径、修改时间和大小，并发送回一个文件报头确认报文，客户端接受后才可以继续执行下面的操作
// 3.客户端发送文件内容，服务端接收后解析json字符串，获取文件的完整路径、修改时间和大小，并发送回一个文件内容确认报文，客户端接受后才可以继续执行下面的操作
// 4.客户端服务器之间没有对完整文件上传结束报文的确认
// 5.上传过程中客户端-服务器采用问-答模式，只有一边确认另一边才可继续下一步
// 6.上传文件全程采用epoll+线程池传输，在服务端方为多线程+多路复用模式

// 程序的下载逻辑是：
// 1.客户端发送登录报文，服务端接收后解析json字符串，获取客户端的参数，并发送回一个登录确认报文，客户端接受后才可以继续执行下面的操作
// 2.客户端发送文件下载请求报文，服务端接收到后立刻开启对该服务端的不间断发送文件，过程中不需要客户端确认，服务器会一直发送
// 3.服务器全部文件发送完成后会发送一个下载完成的信息，客户端以收到该信息为基础停止程序。
// 4.下载过程为完全单线程，依赖封装的TCP类的防粘包，服务器非阻塞，客户端阻塞共同实现
// 5.下载过程IO多路复用基本没有介入

// 创建日志文件对象
logfile lg;
// 创建进程心跳对象
procHeart ph;
// 进程心跳超时时间
int timeout;
// 创建线程池对象
ThreadPool *pool = ThreadPool::getThreadPool(100, 1000);
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
shared_mutex mapMtx;
// 开启一个线程，用于定期检测map中的客户端是否超时，以便及时清理内存
void clearMap();

// 程序的信号退出处理函数
void EXIT(int sig);
// 处理通讯的核心回调函数，IO多路复用的核心回调
void handle(shared_ptr<clientNode> client);
// 处理客户端登录的函数，也就是接收客户端参数
void clientLogin(shared_ptr<clientNode> client, string &msg);
// 接收客户端发送的文件报头的处理逻辑，仅仅用于上传
void recvFileTop(shared_ptr<clientNode> client, string &msg);
// 接收客户端上传的文件
void recvFile(shared_ptr<clientNode> client, char *buffer, int size);
// 先接收客户端发来的File类通知，然后给对方传输文件
void sendFile(shared_ptr<clientNode> client, string &msg);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "Usage: fileserver <logfile>" << endl;
        cout << "example: ./fileserver /temp/log/fileserver.log" << endl;
        cout << "logfile：用于保存文件服务端运行日志的文件" << endl;
        return 0;
    }

    // 防止多余信号干扰
    closeiosignal(false);
    // 接收到2号信号和15号信号时，退出程序
    // 2号信号是Ctrl+C，15号信号是kill命令
    signal(2, EXIT);
    signal(15, EXIT);

    // 打开日志文件
    if (!lg.open(argv[1], ios::out | ios::app))
    {
        cerr << "打开日志文件失败" << endl;
        return 0;
    }

    // 初始化进程心跳
    ph.addProcInfo(getpid(), argv[0], 180);

    // 初始化服务端
    server.createListen(8888);
    // 开启epoll监听线程，handle函数是处理通讯的核心回调函数
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
    while (true)
    {
        // 服务端永远开启
        ph.updateHeart();
        sleep(8);
    }
    return 0;
}

void handle(shared_ptr<clientNode> client)
{
    // 循环处理是为了短时间内客户端迅速发送多条信息，但是epoll只反应过来一次，所以循环recvMsgBin防止因为ET模式粘包，让这一个线程处理这一次的epoll事件
    while (client->is_active)
    {
        // 接收消息
        MessageType type;
        // 接收内容大小
        size_t size = 0;
        // 开辟一段内存10240字节
        char buffer[10240] = {0};
        memset(buffer, 0, sizeof(buffer));

        // 接收客户端消息
        if (!server.recvMsgBin(buffer, client, type, size))
        {
            return;
        }

        // 心跳包的处理逻辑
        if (type == MessageType::Heart)
        {
            // 重置心跳包计数
            client->resetCount();
            // 发送心跳包，不往回发，对于上传程序很难写。
            lg.writeLine("(Heart)客户端%d的心跳包", client->cfd);
        }
        // 客户端传入参数的解析
        else if (type == MessageType::Login)
        {
            string msg(buffer);
            msg.shrink_to_fit();
            lg.writeLine("(Login)客户端%d登录报文:%s", client->cfd, msg.c_str());
            clientLogin(client, msg);
        }
        // 解析文件报头
        else if (type == MessageType::Top)
        {
            string msg(buffer);
            msg.shrink_to_fit();
            lg.writeLine("(Top)客户端%d文件报头:%s", client->cfd, msg.c_str());
            recvFileTop(client, msg);
        }
        // 上传/下载文件的逻辑
        else if (type == MessageType::File)
        {
            int clientType;
            {
                shared_lock<shared_mutex> lock(mapMtx);
                clientType = clientArgsMap[client].clientType;
            }

            if (clientType == 1)
            {
                recvFile(client, buffer, size);
            }
            else if (clientType == 2)
            {
                // 先将信息转为string类型
                string msg(buffer);
                msg.shrink_to_fit();
                lg.writeLine("(File)客户端%d的下载请求%s:", client->cfd, msg.c_str());
                // 这里的信息是文件下载请求
                sendFile(client, msg);
            }
        }
    }
}

void clientLogin(shared_ptr<clientNode> client, string &msg)
{
    nlohmann::json loginJson;
    argss fileConnect;
    // 解析json字符串
    try
    {
        loginJson = nlohmann::json::parse(msg);
    }
    catch (const std::exception &e)
    {
        lg.writeLine("(clientLogin)解析json字符串失败:%s", e.what());
        return;
    }
    fileConnect.clientType = loginJson["clientType"];
    fileConnect.client_path = loginJson["client_path"];
    fileConnect.server_path = loginJson["server_path"];
    {
        unique_lock<shared_mutex> lock(mapMtx);
        clientArgsMap[client] = fileConnect;
    }

    // 发送登录成功的消息
    server.sendMsgWithType(client, "login success", MessageType::Login);
}

void recvFileTop(shared_ptr<clientNode> client, string &msg)
{
    nlohmann::json fileJson;
    // 解析json字符串
    try
    {
        fileJson = nlohmann::json::parse(msg);
    }
    catch (const std::exception &e)
    {
        lg.writeLine("(recvFileTop)解析json字符串失败:%s", e.what());
        return;
    }
    // 获取文件的完整路径
    string fullpath = fileJson["filename"];
    // 获取文件的修改时间
    string modify_time = fileJson["filetime"];
    // 获取文件的大小
    int filesize = fileJson["filesize"];

    // 将文件名等信息存入map中，用于该客户端后续的文件接收，客户端是顺序执行的，一定会先发送文件报头再发送文件，所以采用map去记忆
    {
        unique_lock<shared_mutex> lock(mapMtx);
        clientArgsMap[client].fullpath = fullpath;
        clientArgsMap[client].modify_time = modify_time;
        clientArgsMap[client].filesize = filesize;
    }

    // 解析文件路径，将客户端传入的文件路径替换为服务端传入的文件路径
    string fullfilepath;
    {
        shared_lock<shared_mutex> lock(mapMtx);
        fullfilepath = clientArgsMap[client].fullpath;
    }
    replaceStr(fullfilepath, clientArgsMap[client].client_path, clientArgsMap[client].server_path, false);
    // 如果文件已经存在就删掉它
    deletefile(fullfilepath);

    // 发送接受文件头确认的消息
    if (!server.sendMsgWithType(client, "top ok", MessageType::Top))
    {
        lg.writeLine("(recvFileTop)发送接受文件头确认的消息失败");
    }
}

void recvFile(shared_ptr<clientNode> client, char *buffer, int size)
{
    // 解析文件路径，将客户端传入的文件路径替换为服务端传入的文件路径
    string fullpath;
    {
        shared_lock<shared_mutex> lock(mapMtx);
        fullpath = clientArgsMap[client].fullpath;
    }
    replaceStr(fullpath, clientArgsMap[client].client_path, clientArgsMap[client].server_path, false);
    lg.writeLine("(upload)接收文件：%s,大小：%d...", fullpath.c_str(), clientArgsMap[client].filesize);

    // 创建目录
    newdir(fullpath, true);

    // 写入文件
    wtfile file;
    file.open(fullpath, false, ios::out | ios::app | ios::binary);
    file.writeBin(buffer, size);
    file.close();
    // 回应文件接收成功的消息
    if (!server.sendMsgWithType(client, "file ok", MessageType::Data))
    {
        lg.writeLine("(upload)发送文件接收成功的消息失败");
    }
}

// 给客户端发送和文件的下载模块核心函数
void sendFile(shared_ptr<clientNode> client, string &msg)
{
    // 解析文件下载请求
    nlohmann::json fileJson;
    // 解析json字符串
    try
    {
        fileJson = nlohmann::json::parse(msg);
    }
    catch (const std::exception &e)
    {
        lg.writeLine("(download)解析json字符串失败:%s", e.what());
        return;
    }
    int ptype = fileJson["ptype"];
    string backup_path;
    if (ptype == 2)
    {
        backup_path = fileJson["backup_path"];
    }
    bool recursive = fileJson["recursive"];
    string matchfile = fileJson["matchfile"];
    // 开始发送文件------------------------------------
    // 打开目录
    cdir dir;
    {
        shared_lock<shared_mutex> lock(mapMtx);
        dir.openDir(clientArgsMap[client].server_path, matchfile, 10000, recursive);
    }
    // 遍历目录中的每个文件
    while (dir.readFile())
    {
        lg.writeLine("(download)发送文件%s...", dir.fullpath.c_str());
        // 发送文件报头
        nlohmann::json fileJsonTop;
        {
            shared_lock<shared_mutex> lock(mapMtx);
            fileJsonTop["filename"] = dir.fullpath;
            fileJsonTop["filetime"] = dir.modifytime;
            fileJsonTop["filesize"] = dir.filesize;
        }
        string fileStr = fileJsonTop.dump();
        // 发送文件报头
        if (!server.sendMsgWithType(client, fileStr, MessageType::Top))
        {
            lg.writeLine("(download)发送文件报头失败");
            return;
        }

        int onread = 0;     // 每次读取的字节数
        char buffer[10000]; // 数据缓冲区
        int total = 0;      // 已经读取的字节数

        // 打开文件
        rdfile file;
        if (!file.open(dir.fullpath, ios::in | ios::binary))
        {
            lg.writeLine("(download)打开文件失败");
            return;
        }

        // 传输文件
        while (true)
        {
            memset(buffer, 0, sizeof(buffer));
            // 将每次连接传输的大小控制在10000字节以下
            if (dir.filesize - total >= 10000)
            {
                onread = 10000;
            }
            else
            {
                onread = dir.filesize - total;
            }

            // 读取文件内容
            file.readBin(buffer, onread);
            // 发送读取到的内容
            if (!server.sendMsgBin(client, buffer, onread, MessageType::File))
            {
                lg.writeLine("(download)发送文件失败");
                return;
            }
            // 记录已经读取的字节数
            total += onread;
            // 读取完毕，跳出循环下一个文件
            if (total == dir.filesize)
            {
                break;
            }
        }
        // 传输完成关闭文件
        file.close();
        // 根据ptype字段的值来决定传输完成后的处理方式
        if (ptype == 1)
        {
            // 删除文件
            if (!deletefile(dir.fullpath))
            {
                lg.writeLine("(download)删除文件失败");
                return;
            }
        }
        else if (ptype == 2)
        {
            // 备份文件
            string backupFile = backup_path + dir.filename;
            if (!renamefile(dir.fullpath, backupFile))
            {
                lg.writeLine("(download)备份文件失败");
                return;
            }
        }
    }
    // 发送结束标志
    if (!server.sendMsgWithType(client, "file ok", MessageType::Data))
    {
        lg.writeLine("(download)发送文件结束标志失败");
        return;
    }
}

void clearMap()
{
    while (1)
    {
        {
            unique_lock<shared_mutex> lock(mapMtx);
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
        }
        sleep(10);
    }
}

void EXIT(int sig)
{
    exit(0);
}