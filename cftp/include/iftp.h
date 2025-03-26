// 这个文件定义了文件传输协议的接口
#pragma once
#include "cpublic.h"
#include "fileframe/include/fileframe.h"
#include "timeframe/include/timeframe.h"

class iFileTransfer
{
  protected:
    std::string host;   // 服务器的地址
    std::string user;   // 服务器登录用户
    std::string passwd; // 服务器登录密码
    std::string u_p;    // 用户:密码的组合结构，用于登录
  public:
    std::string modifyTime; // 文件的最后修改时间
    unsigned long filesize;//文件大小

    iFileTransfer()=default;
    // 构造函数初始化
    iFileTransfer(const std::string &host, const std::string &user, const std::string &passwd)
        : host(host), user(user), passwd(passwd)
    {
        this->u_p = user + ":" + passwd;
    }

    iFileTransfer(const iFileTransfer &) = delete;
    iFileTransfer &operator=(const iFileTransfer &) = delete;
    virtual ~iFileTransfer()=default;

    // 初始化登录信息 host:服务器地址 user:登录用户 passwd:登录密码
    void initLogin(const std::string &host, const std::string &user, const std::string &passwd)
    {
        this->host = host;
        this->user = user;
        this->passwd = passwd;
        this->u_p = user + ":" + passwd;
    }

    // 获取服务器文件的最后修改时间
    // filename为服务器上的文件名，返回值为文件的最后修改时间（字符串）
    //获取的时间保存在类公开成员modifyTime中，提供直接获取的重载版本
    virtual bool getModifyTime(const std::string &remoteFilename, TimeType type = TimeType::TIME_TYPE_ONE) = 0;
    virtual bool getModifyTime(const std::string &remoteFilename, std::string &time,
                               TimeType type = TimeType::TIME_TYPE_ONE) = 0;

    // 获取服务器文件的大小
    // filename为服务器上的文件名，返回值为文件的大小，返回-1表示获取失败
    // 获取的文件大小保存在类公开成员filesize中，提供直接获取的重载版本
    virtual bool getFileSize(const std::string &remoteFilename) = 0;
    virtual bool getFileSize(const std::string &remoteFilename, unsigned long &size) = 0;

    // 在服务器上创建目录
    // dirname为要创建的目录名，返回值为创建成功与否
    virtual bool mkdir(const std::string &remoteDirname) = 0;

    // 在服务器上删除目录
    // dirname为要删除的目录名，返回值为删除成功与否
    virtual bool rmdir(const std::string &remoteDirname) = 0;

    // 在服务器上删除文件
    // filename为要删除的文件名，返回值为删除成功与否
    virtual bool rmfile(const std::string &remoteFilename) = 0;

    // 在服务器上重命名文件
    // oldname为原文件名，newname为新文件名，返回值为重命名成功与否
    virtual bool rename(const std::string &oldname, const std::string &newname) = 0;

    // 在服务器上下载文件
    // remoteFilename为服务器上的文件名，localFilename为本地文件名，返回值为下载成功与否
    // check为检查文件在下载过程中是否被修改，如果为true则下载后会检查文件的最后修改时间和大小
    virtual bool download(const std::string &remoteFilename, const std::string &localFilename, bool check = true) = 0;

    // 在服务器上上传文件
    // localFilename为本地文件名，remoteFilename为服务器上的文件名，返回值为上传成功与否
    // check为检查文件在上传过程中是否被修改，如果为true则上传后会检查文件的最后修改时间和大小
    virtual bool upload(const std::string &localFilename, const std::string &remoteFilename, bool check = true) = 0;
};

// FTP协议的接口
class iFTP : public iFileTransfer
{
  public:
    iFTP() = default;
    iFTP(const std::string &host, const std::string &user, const std::string &passwd)
        : iFileTransfer(host, user, passwd)
    {
    }

    iFTP(const iFTP &) = delete;
    iFTP &operator=(const iFTP &) = delete;

    virtual ~iFTP() = default;

    // 新增接口：执行nlist命令，获取目录下的文件列表
    // remoteDirname为服务器上的目录名，list为返回的文件列表
    virtual bool nlist(const std::string &remoteDirname, std::vector<std::string> &list) = 0;

    // 新增接口：执行site命令，执行ftp服务器的原始命令
    // cmd为要执行的命令，返回值为执行结果
    virtual bool site(const std::string &cmd) = 0;
};