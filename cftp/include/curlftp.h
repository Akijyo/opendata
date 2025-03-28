#include "iftp.h"
#include <ctime>
#include <curlpp/cURLpp.hpp>

// 封装curlpp库实现ftp客户端
class ftpClient : public iFTP
{
  public:
    ftpClient()
    {
        curlpp::initialize();
    }
    ~ftpClient()
    {
        curlpp::terminate();
    }
    ftpClient(const std::string &host, const std::string &user, const std::string &passwd) : iFTP(host, user, passwd)
    {
        curlpp::initialize();
    }
    ftpClient(const ftpClient &) = delete;
    ftpClient &operator=(const ftpClient &) = delete;

    // 获取服务器文件的最后修改时间
    // filename为服务器上的文件名，返回值为文件的最后修改时间（字符串）
    // 获取的时间保存在类公开成员modifyTime中，提供直接获取的重载版本
    bool getModifyTime(const std::string &remoteFilename, TimeType type = TimeType::TIME_TYPE_ONE) override;
    bool getModifyTime(const std::string &remoteFilename, std::string &time,
                       TimeType type = TimeType::TIME_TYPE_ONE) override;

    // 获取服务器文件的大小
    // filename为服务器上的文件名，返回值为文件的大小，返回-1表示获取失败
    // 获取的文件大小保存在类公开成员filesize中，提供直接获取的重载版本
    bool getFileSize(const std::string &remoteFilename) override;
    bool getFileSize(const std::string &remoteFilename, unsigned long &size) override;

    // 在服务器上创建目录，不能递归创建
    // dirname为要创建的目录名，返回值为创建成功与否
    bool mkdir(const std::string &remoteDirname) override;

    // 在服务器上递归创建目录，但是不会考虑是否成功
    // dirname为要创建的目录名
    void mkdirrecus(const std::string &remoteDirname) override;

    // 在服务器上删除目录，不提供递归删除，而且只删除空目录
    // dirname为要删除的目录名，返回值为删除成功与否
    bool rmdir(const std::string &remoteDirname) override;

    // 在服务器上删除文件
    // filename为要删除的文件名，返回值为删除成功与否
    bool rmfile(const std::string &remoteFilename) override;

    // 在服务器上重命名文件
    // oldname为原文件名，newname为新文件名，返回值为重命名成功与否
    bool rename(const std::string &oldname, const std::string &newname) override;

    // 执行nlist命令，获取目录下的文件列表
    // remoteDirname为服务器上的目录名，list为返回的文件列表
    bool nlist(const std::string &remoteDirname, std::vector<std::string> &list) override;

    // 执行site命令，执行ftp服务器的原始命令
    // cmd为要执行的命令，返回值为执行结果
    bool site(const std::string &cmd) override;

    // 在服务器上下载文件
    // remoteFilename为服务器上的文件名，localFilename为本地文件名，返回值为下载成功与否
    // check为检查文件在下载过程中是否被修改，如果为true则下载后会检查文件的最后修改时间和大小
    bool download(const std::string &remoteFilename, const std::string &localFilename, bool check = true) override;

    // 在服务器上上传文件
    // localFilename为本地文件名，remoteFilename为服务器上的文件名，返回值为上传成功与否
    // check为检查文件在上传过程中是否被修改，如果为true则上传后会检查文件的最后修改时间和大小
    bool upload(const std::string &localFilename, const std::string &remoteFilename, bool check = true) override;

  private:
    //拼装ftpURL
    std::string rtnUrl(const std::string &remoteFD);
};