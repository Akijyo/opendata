#include "cpublic.h"
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
unordered_map<string, string> isDownloaded; // 已经下载成功的文件列表
vector<fileInfo> nlistInfo;                 // 列出服务器文件名，nlist返回的且经过正则表达式筛选后的文件列表
vector<fileInfo> downloadList;              // 需要下载的文件列表

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
    bool ischeck;         // 是否检查文件是否被修改，如果没有修改，且已经下载过（在finished中），则不下载，bool值
};
ftpInfo ftpConnect; // 定义全局变量

// 为ftpConnect全局变量赋值
bool getFtpInfo(jsonns &js);

// 打开已经下载的文件列表，载入到isDownloaded中
void loadIsDownloadedM();

// 本函数将nlist返回内容经过正则表达式筛选后的加装进fileInfo类中，多一个时间信息，用于增量下载
void loadNlistInfoV(vector<string> &fileList, shared_ptr<iFTP> client);

// 将不需要下载（也就是已经下载或者没有修改过的文件）加入到dontDownload中
// 将需要下载的文件加入到downloadList中
void loadDontWillV(shared_ptr<iFTP> client);

// 将下载完成的文件名写入到finished文件中
void writeFinishedFile();

// 下载文件的核心函数
void download(shared_ptr<iFTP> client);

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

    // 调用ftpclient.nlist方法列出服务器目录中的文件名，放入容器中
    vector<string> fileList;
    if (!client->nlist(ftpConnect.remote_path, fileList))
    {
        lg.writeLine("获取远程目录失败:%s\n%s", ftpConnect.remote_path.c_str(), client->errmsg.c_str());
    }
    if (fileList.size() == 0)
    {
        lg.writeLine("没有找到文件");
    }
    ph.updateHeart(); // 更新心跳信息

    // 如果要求增量下载
    if (ftpConnect.ptype == "1")
    {
        loadIsDownloadedM();
    }
    loadNlistInfoV(fileList, client);
    loadDontWillV(client);
    ph.updateHeart(); // 更新心跳信息

    // 遍历容器，下载
    download(client);
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
    // 解析ischeck字段
    if (!js.get("ischeck", ftpConnect.ischeck))
    {
        ftpConnect.ischeck = false;
    }
    // 解析心跳时间
    if (!js.get("phtimeout", timeout))
    {
        timeout = 30;
    }
    return true;
}

void loadIsDownloadedM()
{
    rdfile rf;
    if (!rf.open(ftpConnect.finished_file))
    {
        // 打开已下载完成的文件失败，说明程序第一次启动或者路径改动
        lg.writeLine("打开finished文件失败");
        return;
    }
    string name, time;
    while (rf.readLine(name) && rf.readLine(time))
    {
        // 将文件名加入到isDownloaded中
        isDownloaded[name] = time;
    }
    rf.close();
}

void loadNlistInfoV(vector<string> &fileList, shared_ptr<iFTP> client)
{
    for (auto &it : fileList)
    {
        // 匹配文件名
        if (matchstr(it, ftpConnect.matchfile) == false)
        {
            continue;
        }
        // 如果是增量下载并且要求检查文件是否修改，获取ftp服务器上的这个文件的时间
        string time = "";
        if (ftpConnect.ptype == "1" && ftpConnect.ischeck)
        {
            string remote_file = ftpConnect.remote_path + it;
            if (!client->getModifyTime(remote_file, time))
            {
                lg.writeLine("获取文件%s的时间失败\n%s", remote_file.c_str(), client->errmsg.c_str());
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
    // 不是增量下载，那列表返回的内容就全部加入到下载列表中
    if (ftpConnect.ptype != "1")
    {
        nlistInfo.swap(downloadList);
        return;
    }
    // 如果是增量下载
    for (auto &it : nlistInfo)
    {
        // 检查文件是否下载过，在已下载列表中没找到说明是文件新增，把这个文件加入到下载列表中
        if (isDownloaded.find(it.name) == isDownloaded.end())
        {
            downloadList.push_back(it);
        }
        else // 说明文件下载过
        {
            // 如果需要按时间检查是否修改
            if (ftpConnect.ischeck)
            {
                // 如果文件时间不相同，说明文件被修改过，加入到下载列表中
                string oldTime; // 上次下载的修改时间
                oldTime = isDownloaded[it.name];
                if (oldTime != it.time)
                {
                    downloadList.push_back(it);
                }
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
    for (auto &it : downloadList)
    {
        wf.writeLine("%s", it.name.c_str());
        wf.writeLine("%s", it.time.c_str());
    }
    wf.close();
    lg.writeLine("已下载完成的文件写入到%s", ftpConnect.finished_file.c_str());
}

void download(shared_ptr<iFTP> client)
{
    for (auto &it : downloadList)
    {
        // 下载文件
        string remote_file = ftpConnect.remote_path + it.name;
        string local_file = ftpConnect.local_path + it.name;
        if (client->download(remote_file, local_file) == false)
        {
            lg.writeLine("下载%s失败\n%s", it.name.c_str(), client->errmsg.c_str());
            continue;
        }
        // 下载成功，打印日志
        lg.writeLine("下载%s成功", it.name.c_str());

        // ptype=2，删除文件
        if (ftpConnect.ptype == "2")
        {
            if (client->rmfile(remote_file) == false)
            {
                lg.writeLine("删除%s失败\n%s", remote_file.c_str(), client->errmsg.c_str());
            }
        }

        // ptype=3，备份文件
        if (ftpConnect.ptype == "3")
        {
            if (client->rename(remote_file, ftpConnect.backup_path + it.name) == false)
            {
                lg.writeLine("备份%s失败\n%s", remote_file.c_str(), client->errmsg.c_str());
            }
        }
    }
}

void help()
{
    cout << "Usage: ftpgetfiles <logfile> <jsonfile>" << endl;
    cout << "logfile：用于保存下载日志的文件" << endl;
    cout << "jsonfile：用于保存程序传入参数的json文件" << endl;
    cout << "Example: ./ftpgetfiles /temp/log/download.log "
            "/home/akijyo/桌面/code/c++/opendata/tools/others/ftpgetfiles.json"
         << endl
         << endl;
    cout << "json文件应有的字段：" << endl;
    cout << "host: ftp服务器地址" << endl;
    cout << "user: ftp用户名" << endl;
    cout << "password: ftp密码" << endl;
    cout << "remote_path: ftp服务器上的文件目录" << endl;
    cout << "local_path: 本地保存目录" << endl;
    cout << "matchfile: 匹配的文件，使用正则表达式" << endl;
    cout << "ptype: 文件下载成功后服务器的文件处理方式：1.增量下载，2.删除，3.备份（指定目录）" << endl;
    cout << "backup_path: 备份目录，只有ptype=3值才有效" << endl;
    cout << "finished_file: 已经下次成功的文件的列表纪录文件绝对路径，用于增量下载文件中获取和判断" << endl;
    cout << "ischeck: 是否检查文件是否被修改，如果没有修改，且已经下载过（在finished中），则不下载" << endl;
    cout << "phtimeout: 进程心跳时间，单位秒，默认30秒" << endl;
    cout << "注意：json文件中所有路径都必须是绝对路径" << endl;
}