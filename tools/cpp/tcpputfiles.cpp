#include "cpublic.h"
#include "fileframe/include/cdir.h"
#include "fileframe/include/logfile.h"
#include "include/TcpClient.h"
#include "include/socket.h"
#include "procheart/include/procheart.h"
#include "stringop/include/jsonns.h"
#include <nlohmann/json.hpp>
using namespace std;
// 创建日志文件对象
logfile lg;
// 创建进程心跳对象
procHeart ph;
// 进程心跳超时时间
int timeout;
// 创建tcp客户端对象
TcpClient client;

// 程序运行的参数的集合类
class argss
{
  public:
    int clientType;     // 客户端类型 1.上传 2.下载，默认为1
    string ip;          // 服务器IP
    int port;           // 服务器端口
    string client_path; // 本地文件的目录 /data /data/aaa
    string server_path; // 服务器文件的目录 /data /data/aaa
    bool recursive;     // 是否递归上传下载，开启后将会遍历目录下的所有文件，且服务端目录将保持一致
    string matchfile;   // 匹配文件名，默认空字符串，表示匹配所有文件，使用正则表达式匹配
    int ptype;          // 传输完成后本地处理情况，1.删除。2.备份到特定目录
    string backup_path; // 备份目录，只有的当ptype=2时才会使用
    int timeval;        // 执行上传文件任务的时间间隔，单位秒
};
argss fileConnect;

// 处理2,15信号的退出
void EXIT(int sig);
// 帮助文档
void help();
// 解析json文件
bool getArgs(string jsonfile);
// 定期发送心跳包
void sendHeartbeat();
// 登录tcp服务器的函数，负责的是把程序的参数传给服务端，发送的是json格式的字符串
bool login();
// 上传文件的函数
bool uploadFile();

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        help();
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
    // 解析命令行参数
    if (!getArgs(argv[2]))
    {
        lg.writeLine("解析命令行参数失败");
        return 0;
    }

    // 初始化客户端
    client.connectServer(fileConnect.ip, fileConnect.port);
    // 创建定期发送心跳包的线程
    thread heartbeatThread(sendHeartbeat);
    heartbeatThread.detach();
    sleep(1);
    // 登录tcp服务器
    if (!login())
    {
        lg.writeLine("登录失败:ip:%s,port:%d", fileConnect.ip.c_str(), fileConnect.port);
        return 0;
    }

    // 开启上传
    uploadFile();

    return 0;
}

void sendHeartbeat()
{
    while (1)
    {
        // 检测网络通信心跳是否超时
        if (client.getCount() > 3)
        {
            lg.writeLine("心跳超时，断开连接");
            exit(0);
        }
        client.addCount();
        // 发送心跳包
        if (!client.sendMsgWithType("heartbeat", MessageType::Heart))
        {
            lg.writeLine("发送心跳包失败");
        }
        // 等待一段时间
        sleep(fileConnect.timeval);
    }
}

bool login()
{
    nlohmann::json loginJson;
    loginJson["clientType"] = fileConnect.clientType;
    loginJson["ip"] = fileConnect.ip;
    loginJson["port"] = fileConnect.port;
    loginJson["client_path"] = fileConnect.client_path;
    loginJson["server_path"] = fileConnect.server_path;
    loginJson["recursive"] = fileConnect.recursive;
    loginJson["matchfile"] = fileConnect.matchfile;
    loginJson["ptype"] = fileConnect.ptype;
    loginJson["backup_path"] = fileConnect.backup_path;
    loginJson["timeval"] = fileConnect.timeval;
    string loginStr = loginJson.dump();
    lg.writeLine("loginstr:%s", loginStr.c_str());
    if (!client.sendMsgWithType(loginStr, MessageType::Login))
    {
        lg.writeLine("发送程序参数失败");
        return false;
    }
    // 接收服务端的登录成功的消息
    string msg;
    MessageType type;
    if (!client.recvMsgWithType(msg, type))
    {
        lg.writeLine("接收登录成功的消息失败");
        return false;
    }
    lg.writeLine("登录成功，服务端返回的消息:%s", msg.c_str());
    lg.writeLine("登录成功,ip:%s,port:%d", fileConnect.ip.c_str(), fileConnect.port);
    client.resetCount();
    return true;
}

bool uploadFile()
{
    cdir dir;
    // 打开目录
    dir.openDir(fileConnect.client_path, fileConnect.matchfile, 10000, fileConnect.recursive);
    // 遍历目录中的每个文件
    while (dir.readFile())
    {
        // 拼接传输的文件报头
        nlohmann::json fileJson;
        fileJson["filename"] = dir.fullpath;
        fileJson["filetime"] = dir.modifytime;
        fileJson["filesize"] = dir.filesize;
        string fileStr = fileJson.dump();
        lg.writeLine("filestr:%s", fileStr.c_str());
        // 发送文件报头
        if (!client.sendMsgWithType(fileStr, MessageType::Top))
        {
            lg.writeLine("发送文件报头失败");
            return false;
        }
        // 发送文件

        // 接收服务端的确认报文
    }
    return true;
}

void EXIT(int sig)
{
    exit(0);
}

bool getArgs(string jsonfile)
{
    jsonns js(jsonfile);
    // 解析clientType字段
    if (!js.get("clientType", fileConnect.clientType))
    {
        fileConnect.clientType = 1;
    }
    if (fileConnect.clientType != 1 && fileConnect.clientType != 2)
    {
        lg.writeLine("clientType字段值不合法");
        return false;
    }
    // 解析ip字段
    if (!js.get("ip", fileConnect.ip))
    {
        lg.writeLine("获取ip字段失败");
        return false;
    }
    // 解析port字段
    if (!js.get("port", fileConnect.port))
    {
        lg.writeLine("获取port字段失败");
        return false;
    }
    // 解析client_path字段
    if (!js.get("client_path", fileConnect.client_path))
    {
        lg.writeLine("获取client_path字段失败");
        return false;
    }
    // 解析server_path字段
    if (!js.get("server_path", fileConnect.server_path))
    {
        lg.writeLine("获取server_path字段失败");
        return false;
    }
    // 解析recursive字段
    if (!js.get("recursive", fileConnect.recursive))
    {
        fileConnect.recursive = false;
    }
    // 解析matchfile字段
    if (!js.get("matchfile", fileConnect.matchfile))
    {
        fileConnect.matchfile = ".*";
    }
    // 解析ptype字段
    if (!js.get("ptype", fileConnect.ptype))
    {
        fileConnect.ptype = 1;
    }
    if (fileConnect.ptype != 1 && fileConnect.ptype != 2)
    {
        lg.writeLine("ptype字段值不合法");
        return false;
    }
    // 解析backup_path字段
    if (fileConnect.ptype == 2)
    {
        if (!js.get("backup_path", fileConnect.backup_path))
        {
            lg.writeLine("获取backup_path字段失败");
            return false;
        }
    }
    // 解析timeval字段
    if (!js.get("timeval", fileConnect.timeval))
    {
        fileConnect.timeval = 20;
    }
    if (fileConnect.timeval > 20 || fileConnect.timeval < 1)
    {
        fileConnect.timeval = 20;
    }
    // 解析phtimeout字段
    if (!js.get("phtimeout", timeout))
    {
        timeout = 30;
    }
    if (timeout < 30)
    {
        timeout = 30; // 心跳时间不能小于30秒
    }
    if (fileConnect.client_path[fileConnect.client_path.length() - 1] != '/')
    {
        fileConnect.client_path += '/';
    }
    if (fileConnect.server_path[fileConnect.server_path.length() - 1] != '/')
    {
        fileConnect.server_path += '/';
    }
    if (fileConnect.ptype == 2 && fileConnect.backup_path[fileConnect.backup_path.length() - 1] != '/')
    {
        fileConnect.backup_path += '/';
    }
    return true;
}

void help()
{
    cout << "Usage: tcpputfiles <logfile> <jsonfile>" << endl;
    cout << "logfile：用于保存上传日志的文件" << endl;
    cout << "jsonfile：用于保存程序传入参数的json文件" << endl;
    cout << "Example: ./tcpputfiles /temp/log/tcpupload.log "
            "/home/akijyo/桌面/code/c++/opendata/tools/others/tcpputfiles.json"
         << endl
         << endl;
    cout << "json文件应有的字段：" << endl;
    cout << "clientType: 客户端类型，1.上传。2.下载。本程序默认1" << endl;
    cout << "ip: 服务端ip地址" << endl;
    cout << "port: 服务端端口" << endl;
    cout << "client_path: 客户端文件传输的目录" << endl;
    cout << "server_path: 服务端文件传输的目录" << endl;
    cout << "recursive: 是否需要递归传输子目录文件" << endl;
    cout << "matchfile: 匹配的文件格式，使用正则表达式进行匹配" << endl;
    cout << "ptype: 文件传输完成后的行为，1.删除。2.备份" << endl;
    cout << "backup_path: 文件传输完成后的备份的目录，只有当ptype=2才有效" << endl;
    cout << "timeval: 执行上传文件任务的时间间隔，单位秒" << endl;
    cout << "phtimeout: 进程心跳时间，单位秒，默认30秒" << endl;
    cout << "注意：json文件中所有路径都必须是绝对路径" << endl;
}