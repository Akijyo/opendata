#include "include/curlftp.h"
#include "stringop/include/split.h"
#include "stringop/include/stringop.h"
#include "timeframe/include/timeframe.h"
#include <cstdio>
#include <curlpp/Options.hpp>
#include <sstream>
using namespace std;

string ftpClient::rtnUrl(const string &remoteFD)
{
    // 拼接ftp文件/目录 url
    string url = "ftp://" + this->host + "/" + remoteFD;
    return url;
}

bool ftpClient::getModifyTime(const string &remoteFilename, TimeType type)
{
    curlpp::Easy request;
    try
    {
        // 设置请求的url
        request.setOpt(new curlpp::options::Url(this->rtnUrl(remoteFilename)));
        // 设置ftp服务器的用户名和密码
        request.setOpt(new curlpp::options::UserPwd(this->u_p));
        // 只获取文件的最后修改时间
        request.setOpt(new curlpp::options::NoBody(true));
        // 请求文件的时间信息
        request.setOpt(new curlpp::options::FileTime(true));
        // 吸收多余的标准输出
        std::ostringstream dummyStream;
        request.setOpt(new curlpp::options::WriteStream(&dummyStream));
        // 执行请求
        request.perform();
    }
    catch (const curlpp::RuntimeError &e)
    {
        cerr << "请求失败: " << e.what() << endl;
        return false;
    }

    // 解析响应得来的时间
    long filetime = 0;
    curlpp::infos::FileTime::get(request, filetime);
    if (filetime == 0)
    {
        return false;
    }
    // 东八区修正
    filetime -= 8 * 3600;

    this->modifyTime = timeToStr(filetime, type);
    return true;
}

bool ftpClient::getModifyTime(const string &remoteFilename, string &time, TimeType type)
{
    if (!getModifyTime(remoteFilename, type))
    {
        return false;
    }
    time = this->modifyTime;
    return true;
}

bool ftpClient::getFileSize(const string &remoteFilename)
{
    curlpp::Easy request;
    try
    {
        // 设置请求的url
        request.setOpt(new curlpp::options::Url(this->rtnUrl(remoteFilename)));
        // 设置ftp服务器的用户名和密码
        request.setOpt(new curlpp::options::UserPwd(this->u_p));
        // 只获取文件的大小
        request.setOpt(new curlpp::options::NoBody(true));
        // 吸收多余输出内容
        ostringstream response;
        request.setOpt(new curlpp::options::WriteStream(&response));
        // 执行请求
        request.perform();
    }
    catch (const curlpp::RuntimeError &e)
    {
        cerr << "请求失败: " << e.what() << endl;
        return false;
    }

    // 解析响应得来的文件大小
    long filesize = 0;
    curlpp::infos::ContentLengthDownloadT::get(request, filesize);
    if (filesize == 0)
    {
        return false;
    }
    this->filesize = filesize;
    return true;
}

bool ftpClient::getFileSize(const string &remoteFilename, unsigned long &size)
{
    if (!getFileSize(remoteFilename))
    {
        return false;
    }
    size = this->filesize;
    return true;
}

bool ftpClient::mkdir(const string &remoteDirname)
{
    curlpp::Easy request;
    try
    {
        // 设置请求的url
        request.setOpt(curlpp::options::Url("ftp://" + this->host));
        // 设置ftp服务器的用户名和密码
        request.setOpt(curlpp::options::UserPwd(this->u_p));
        // 不关注文件
        request.setOpt(curlpp::options::NoBody(true));
        // 发送创建目录的命令
        list<string> command;
        command.push_back("MKD " + remoteDirname);
        request.setOpt(curlpp::options::Quote(command));

        // 启用调试信息，查看 FTP 服务器返回的消息
        request.setOpt(new curlpp::options::Verbose(true));

        // 执行请求
        request.perform();
    }
    catch (const curlpp::RuntimeError &e)
    {
        cerr << "请求失败: " << e.what() << endl;
        return false;
    }
    return true;
}

void ftpClient::mkdirrecus(const string &remoteDirname)
{
    // 分割目录
    ccmdstr dirsplit(remoteDirname, "/");
    string dir;

    curlpp::Easy request;
    // 设置请求的url
    request.setOpt(curlpp::options::Url("ftp://" + this->host));
    // 设置ftp服务器的用户名和密码
    request.setOpt((curlpp::options::UserPwd(this->u_p)));
    // 不关注文件
    request.setOpt(curlpp::options::NoBody(true));

    // 启用调试信息，查看 FTP 服务器返回的消息
    // request.setOpt(new curlpp::options::Verbose(true));

    // 递归创建目录
    for (int i = 0; i < dirsplit.size(); i++)
    {
        // 构建要创建的目录
        dir += "/" + dirsplit[i];
        // 发送创建目录的命令
        list<string> command;
        command.push_back("MKD " + dir);
        request.setOpt(curlpp::options::Quote(command));
        // 执行请求
        try
        {
            request.perform();
        }
        catch (const curlpp::RuntimeError &e)
        {
            // perform失败会直接抛出异常并且终止程序，所以这里要用try-catch捕获防止程序退出
            continue;
        }
    }
}

bool ftpClient::rmdir(const string &remoteDirname)
{
    curlpp::Easy request;
    try
    {
        // 设置请求url
        request.setOpt(new curlpp::options::Url("ftp://" + this->host));
        // 设置ftp服务器的用户名和密码
        request.setOpt(new curlpp::options::UserPwd(this->u_p));
        // 设置删除目录的命令
        list<string> command;
        command.push_back("RMD " + remoteDirname);
        request.setOpt(new curlpp::options::Quote(command));

        // 启用调试信息，查看 FTP 服务器返回的消息
        request.setOpt(new curlpp::options::Verbose(true));

        // 执行请求
        request.perform();
    }
    catch (const curlpp::RuntimeError &e)
    {
        cerr << "删除目录失败: " << e.what() << endl;
        return false;
    }
    return true;
}

bool ftpClient::rmfile(const string &remoteFilename)
{
    curlpp::Easy request;
    try
    {
        // 设置请求的url，这里不能给文件全路径，只能ip
        request.setOpt(new curlpp::options::Url("ftp://" + this->host));
        // 设置ftp服务器的用户名和密码
        request.setOpt(new curlpp::options::UserPwd(this->u_p));
        // 设置删除文件的命令
        list<string> command;
        command.push_back("DELE " + remoteFilename);
        request.setOpt(new curlpp::options::Quote(command));

        // 启用调试信息，查看 FTP 服务器返回的消息
        // request.setOpt(new curlpp::options::Verbose(true));

        // 执行请求
        request.perform();
    }
    catch (const curlpp::RuntimeError &e)
    {
        cerr << "删除文件失败: " << e.what() << endl;
        return false;
    }
    return true;
}

bool ftpClient::rename(const string &oldname, const string &newname)
{
    curlpp::Easy request;
    try
    {
        // 设置请求的url
        request.setOpt(new curlpp::options::Url("ftp://" + this->host));
        // 设置ftp服务器的用户名和密码
        request.setOpt(new curlpp::options::UserPwd(this->u_p));
        // 设置重命名文件的命令
        list<string> command;
        command.push_back("RNFR " + oldname);
        command.push_back("RNTO " + newname);
        request.setOpt(new curlpp::options::Quote(command));

        // 启用调试信息，查看 FTP 服务器返回的消息
        // request.setOpt(new curlpp::options::Verbose(true));

        // 执行请求
        request.perform();
    }
    catch (const curlpp::RuntimeError &e)
    {
        cerr << "重命名文件失败: " << e.what() << endl;
        return false;
    }
    return true;
}

bool ftpClient::nlist(const string &remoteDirname, vector<string> &list)
{
    curlpp::Easy request;
    ostringstream response;
    try
    {
        // 设置请求的url
        request.setOpt(new curlpp::options::Url(this->rtnUrl(remoteDirname)));
        // 设置ftp服务器的用户名和密码
        request.setOpt(new curlpp::options::UserPwd(this->u_p));
        // 发送获取文件列表的命令
        request.setOpt(new curlpp::options::CustomRequest("NLST"));

        // 启用调试信息，查看 FTP 服务器返回的消息
        // request.setOpt(new curlpp::options::Verbose(true));

        // 捕获服务器返回的文件列表
        request.setOpt(new curlpp::options::WriteStream(&response));

        // 执行请求
        request.perform();
    }
    catch (const curlpp::RuntimeError &e)
    {
        cerr << "获取文件列表失败: " << e.what() << endl;
        return false;
    }
    // 将ostring流中的文件列表转换为istringstream，不然不可读
    istringstream is(response.str());
    // 读取文件列表
    while (is)
    {
        string temp;
        getline(is, temp);
        if (temp.empty() == false)
        {
            list.push_back(temp);
        }
    }
    return true;
}

bool ftpClient::site(const string &cmd)
{
    curlpp::Easy request;
    try
    {
        // 设置请求的url
        request.setOpt(new curlpp::options::Url("ftp://" + this->host));
        // 设置ftp服务器的用户名和密码
        request.setOpt(new curlpp::options::UserPwd(this->u_p));
        // 发送获取文件列表的命令
        request.setOpt(new curlpp::options::CustomRequest("SITE " + cmd));

        // 启用调试信息，查看 FTP 服务器返回的消息
        request.setOpt(new curlpp::options::Verbose(true));

        // 执行请求
        request.perform();
    }
    catch (const curlpp::RuntimeError &e)
    {
        // SITE执行成功会返回200代码，但是libcurl会将其视为错误
        // 这里捕获异常并判断错误信息，如果包含 "RETR response: 200"，则认为是成功
        std::string errMsg = e.what();
        // 如果错误信息包含 "RETR response: 200"，则认为是 libcurl 的误判，视为成功
        if (errMsg.find("RETR response: 200") != std::string::npos)
        {
            return true;
        }
        std::cerr << "SITE执行失败: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool ftpClient::download(const string &remoteFilename, const string &localFilename, bool check)
{
    return true;
}

bool ftpClient::upload(const string &localFilename, const string &remoteFilename, bool check)
{
    return true;
}
