#include "cpublic.h"
#include "fileframe/include/cdir.h"
#include "fileframe/include/fileframe.h"
#include "fileframe/include/fileio.h"
#include "fileframe/include/logfile.h"
#include "include/curlftp.h"
#include "include/ifilefransfer.h"
#include "include/socket.h"
#include "procheart/include/procheart.h"
#include "stringop/include/jsonns.h"
#include "stringop/include/stringop.h"
#include <unistd.h>
using namespace std;

// 日志文件对象
logfile lg;
// 进程心跳对象
procHeart ph;
// 进程心跳超时时间
int timeout;

// ftp文件信息，保存了文件名和文件时间，用于在增量下载中的判断
class fileInfo
{
  public:
    string name; // 文件名
    string time; // 文件时间
    fileInfo() = default;
    fileInfo(string name, string time) : name(name), time(time)
    {
    }
};
unordered_map<string, string> isUploaded; // 已经上传成功的文件列表
vector<fileInfo> nlistInfo;               // 列出本地目录的文件名，通过cdir解析
vector<fileInfo> uploadList;              // 需要上传的文件列表

// ftp传输信息
class ftpInfo
{
  public:
    string host;          // ftp服务器地址
    string user;          // ftp用户名
    string password;      // ftp密码
    string remote_path;   // ftp服务器上的文件目录(目录路径)
    string local_path;    // 本地保存目录(目录路径)
    string matchfile;     // 匹配的文件，默认匹配全部文件，使用正则表达式
    string ptype;         // 文件下载成功后服务器的文件处理方式：1.什么也不做，2.删除，3.备份（指定目录）
    string backup_path;   // 备份目录，只有ptype=3值才有效，可省略(目录路径)
    string finished_file; // 已经下次成功的文件的列表纪录文件，用于增量下载文件中获取和判断(文件路径)
};
ftpInfo ftpConnect; // 定义全局变量

// 处理2,15信号的退出
void EXIT(int sig);

// 为ftpConnect全局变量赋值
bool getFtpInfo(jsonns &js);

// 打开已经上传的文件列表，载入到isUpload中
void loadIsUploadM();

// 本函数将cdir解析的内容经过正则表达式筛选后的加装进fileInfo类中，多一个时间信息，用于增量下载
void loadNlistInfoV(vector<string> &fileList);

// 将需要上传的文件加入到uploadList中
void loadDontWillV(shared_ptr<iFTP> client);

// 将上传完成的文件名写入到finished文件中
void writeFinishedFile();

// 下载文件的核心函数
void upload(shared_ptr<iFTP> client);

// 程序帮助文档
void help();

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
        cout << "打开日志文件失败" << endl;
        return 0;
    }

    // 解析json文件
    jsonns js(argv[2]);
    getFtpInfo(js);

    // 添加进程心跳信息
    ph.addProcInfo(getpid(), argv[0], timeout);

    // 登录ftp服务器
    shared_ptr<iFTP> client = make_shared<ftpClient>();
    client->initLogin(ftpConnect.host, ftpConnect.user, ftpConnect.password);

    // 调用cdir类解析目录的文件
    vector<string> fileList;
    cdir dir;
    // 打开目录并且筛选文件
    dir.openDir(ftpConnect.local_path, ftpConnect.matchfile);
    while (dir.readFile())
    {
        // 获取文件名
        fileList.push_back(dir.filename);
    }
    if (fileList.size() == 0)
    {
        lg.writeLine("没有找到文件");
    }
    ph.updateHeart(); // 更新心跳信息

    // 如果要求增量下载
    if (ftpConnect.ptype == "1")
    {
        loadIsUploadM();
    }
    loadNlistInfoV(fileList);
    loadDontWillV(client);
    ph.updateHeart(); // 更新心跳信息

    // 遍历容器，下载
    upload(client);
    ph.updateHeart(); // 更新心跳信息

    if (ftpConnect.ptype == "1")
    {
        // 将下载完成的文件名写入到finished文件中
        writeFinishedFile();
        ph.updateHeart(); // 更新心跳信息
    }

    return 0;
}

bool getFtpInfo(jsonns &js)
{
    // 解析host字段
    if (!js.get("host", ftpConnect.host))
    {
        lg.writeLine("没有找到host字段");
        return false;
    }
    // 解析user字段
    if (!js.get("user", ftpConnect.user))
    {
        lg.writeLine("没有找到user字段");
        return false;
    }
    // 解析password字段
    if (!js.get("password", ftpConnect.password))
    {
        lg.writeLine("没有找到password字段");
        return false;
    }
    // 解析remote_path字段
    if (!js.get("remote_path", ftpConnect.remote_path))
    {
        lg.writeLine("没有找到remote_path字段");
        return false;
    }
    if (ftpConnect.remote_path[ftpConnect.remote_path.length() - 1] != '/')
    {
        ftpConnect.remote_path += '/';
    }
    // 解析local_path字段
    if (!js.get("local_path", ftpConnect.local_path))
    {
        lg.writeLine("没有找到local_path字段");
        return false;
    }
    if (ftpConnect.local_path[ftpConnect.local_path.length() - 1] != '/')
    {
        ftpConnect.local_path += '/';
    }
    // 解析matchfile字段
    if (!js.get("matchfile", ftpConnect.matchfile))
    {
        ftpConnect.matchfile = ".*";
    }
    // 解析ptype字段
    if (!js.get("ptype", ftpConnect.ptype))
    {
        ftpConnect.ptype = "1";
    }
    if (ftpConnect.ptype != "1" && ftpConnect.ptype != "2" && ftpConnect.ptype != "3")
    {
        lg.writeLine("ptype字段值不合法");
        return false;
    }
    // 解析backup_path字段
    if (ftpConnect.ptype == "3")
    {
        if (!js.get("backup_path", ftpConnect.backup_path))
        {
            ftpConnect.backup_path = "/temp/backup/download";
        }
        if (ftpConnect.backup_path[ftpConnect.backup_path.length() - 1] != '/')
        {
            ftpConnect.backup_path += '/';
        }
    }
    // 解析finished_file字段
    if (!js.get("finished_file", ftpConnect.finished_file))
    {
        lg.writeLine("没有找到finished_file字段");
    }
    // 解析心跳时间
    if (!js.get("phtimeout", timeout))
    {
        timeout = 30;
    }
    return true;
}

void loadIsUploadM()
{
    rdfile rf;
    if (!rf.open(ftpConnect.finished_file))
    {
        // 打开已上传完成的文件失败，说明程序第一次启动或者路径改动
        lg.writeLine("打开finished文件失败");
        return;
    }
    string name, time;
    while (rf.readLine(name) && rf.readLine(time))
    {
        // 将文件名加入到isUploaded中
        isUploaded[name] = time;
    }
    rf.close();
}

void loadNlistInfoV(vector<string> &fileList)
{
    for (auto &it : fileList)
    {
        // 如果是增量上传，获取本地目标文件的时间
        // 上传模块的增量上传必须核查时间
        string time = "";
        if (ftpConnect.ptype == "1")
        {
            string local_file = ftpConnect.local_path + it;
            if (!fileTime(local_file, time))
            {
                lg.writeLine("获取文件%s的时间失败", local_file.c_str());
                continue;
            }
        }
        string name = it;
        // 将类内容压入nlistInfo中
        nlistInfo.push_back(fileInfo(name, time));
    }
}

void loadDontWillV(shared_ptr<iFTP> client)
{
    // 不是增量上传，那列表返回的内容就全部加入到上传列表中
    if (ftpConnect.ptype != "1")
    {
        nlistInfo.swap(uploadList);
        return;
    }
    // 如果是增量上传
    for (auto &it : nlistInfo)
    {
        // 检查文件是否上传过，在已上传列表中没找到说明是文件新增，把这个文件加入到上传列表中
        if (isUploaded.find(it.name) == isUploaded.end())
        {
            uploadList.push_back(it);
        }
        else // 说明文件上传过
        {
            // 如果本地文件时间不相同，说明文件被修改过，加入到上传列表中
            string oldTime; // 上次上传的修改时间
            oldTime = isUploaded[it.name];
            if (oldTime != it.time)
            {
                uploadList.push_back(it);
            }
        }
    }
}

void writeFinishedFile()
{
    // 追加方式打开已下载完成的文件
    wtfile wf;
    if (!wf.open(ftpConnect.finished_file, false, ios::out | ios::app))
    {
        lg.writeLine("写回完成文件时打开finished文件失败");
        return;
    }
    // 写入已下载完成的文件
    for (auto &it : uploadList)
    {
        wf.writeLine("%s", it.name.c_str());
        wf.writeLine("%s", it.time.c_str());
    }
    wf.close();
    lg.writeLine("已上传完成的文件写入到%s", ftpConnect.finished_file.c_str());
}

void upload(shared_ptr<iFTP> client)
{
    for (auto &it : uploadList)
    {
        // 上传文件
        string remote_file = ftpConnect.remote_path + it.name;
        string local_file = ftpConnect.local_path + it.name;
        if (client->upload(local_file, remote_file, true) == false)
        {
            lg.writeLine("上传%s失败\n%s", it.name.c_str(), client->errmsg.c_str());
            continue;
        }
        // 上传成功，打印日志
        lg.writeLine("上传%s成功", it.name.c_str());

        // ptype=2，删除文件
        if (ftpConnect.ptype == "2")
        {
            if (deletefile(local_file) == false)
            {
                lg.writeLine("上传完成后删除文件失败:%s", local_file.c_str());
            }
        }

        // ptype=3，备份文件
        if (ftpConnect.ptype == "3")
        {
            if (renamefile(local_file, ftpConnect.backup_path + it.name) == false)
            {
                lg.writeLine("上传完成后备份失败:%s", remote_file.c_str());
            }
        }
    }
}

void help()
{
    cout << "Usage: ftpputfiles <logfile> <jsonfile>" << endl;
    cout << "logfile：用于保存上传日志的文件" << endl;
    cout << "jsonfile：用于保存程序传入参数的json文件" << endl;
    cout << "Example: ./ftpputfiles /temp/log/upload.log "
            "/home/akijyo/桌面/code/c++/opendata/tools/others/ftpputfiles.json"
         << endl
         << endl;
    cout << "json文件应有的字段：" << endl;
    cout << "host: ftp服务器地址" << endl;
    cout << "user: ftp用户名" << endl;
    cout << "password: ftp密码" << endl;
    cout << "remote_path: ftp服务器上的文件目录" << endl;
    cout << "local_path: 本地保存目录" << endl;
    cout << "matchfile: 匹配的文件，使用正则表达式" << endl;
    cout << "ptype: 文件上传成功后服务器的文件处理方式：1.增量上传，2.删除，3.备份（指定目录）" << endl;
    cout << "backup_path: 备份目录，只有ptype=3值才有效" << endl;
    cout << "finished_file: 已经上传成功的文件的列表纪录文件绝对路径，用于增量上传文件中获取和判断" << endl;
    cout << "phtimeout: 进程心跳时间，单位秒，默认30秒" << endl;
    cout << "注意：json文件中所有路径都必须是绝对路径" << endl;
}

void EXIT(int sig)
{
    exit(0);
}