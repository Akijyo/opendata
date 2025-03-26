#include "include/curlftp.h"
#include "stringop/include/stringop.h"
#include "timeframe/include/timeframe.h"
#include <cmath>
#include <ctime>
#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <sstream>
#include <string>
using namespace std;

string ftpClient::rtnUrl(const string &remoteFD)
{
    // 拼接ftp url
    string url = "ftp://" + this->host + "/" + remoteFD;
    return url;
}
// 没用的玩意
time_t ftpClient::parseMDTMresponse(const string &response)
{
    if (response.size() == 0)
    {
        return -1;
    }
    string temp = pickNum(response);
    // 初始化ts结构体
    tm ts = {};
    // 提取出时间字符串
    try // stoi函数要用try处理防止异常导致程序退出
    {
        ts.tm_year = stoi(temp.substr(0, 4)) - 1900; // tm结构体中的年份是从1900年开始计数的
        ts.tm_mon = stoi(temp.substr(4, 2)) - 1;     // 月份是从0开始计数
        ts.tm_mday = stoi(temp.substr(6, 2));
        ts.tm_hour = stoi(temp.substr(8, 2));
        ts.tm_min = stoi(temp.substr(10, 2));
        ts.tm_sec = stoi(temp.substr(12, 2));
    }
    catch (const std::exception &e)
    {
        return -1;
    }
    return mktime(&ts);
}

bool ftpClient::getModifyTime(const string &remoteFilename, TimeType type)
{
    curlpp::Easy request;
    // 设置请求的url
    request.setOpt(new curlpp::options::Url(rtnUrl(remoteFilename)));
    // 设置ftp服务器的用户名和密码
    request.setOpt(new curlpp::options::UserPwd(this->u_p));
    // 只获取文件的最后修改时间
    request.setOpt(new curlpp::options::NoBody(true));
    // 请求文件的时间信息
    request.setOpt(new curlpp::options::FileTime(true));
    // 吸收多余的标准输出，在全部内容完成后可以注释这段内容
    std::ostringstream dummyStream;
    request.setOpt(new curlpp::options::WriteStream(&dummyStream));

    // 执行请求
    try
    {
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
    // 设置请求的url
    request.setOpt(new curlpp::options::Url(rtnUrl(remoteFilename)));
    // 设置ftp服务器的用户名和密码
    request.setOpt(new curlpp::options::UserPwd(this->u_p));
    // 设置请求的命令
    // request.setOpt(curlpp::options::CustomRequest("SIZE"));

    // 捕获服务器响应
    ostringstream response;
    request.setOpt(new curlpp::options::WriteStream(&response));
    // 执行请求
    try
    {
        request.perform();
    }
    catch (const curlpp::RuntimeError &e)
    {
        cerr << "请求失败: " << e.what() << endl;
        return false;
    }

    // 解析响应得来的文件大小
    string size = response.str();
    if (size.size() == 0)
    {
        return false;
    }
    size = pickNum(size);
    this->filesize = stoul(size);
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
    return true;
}

bool ftpClient::rmdir(const string &remoteDirname)
{
    return true;
}

bool ftpClient::rmfile(const string &remoteFilename)
{
    return true;
}

bool ftpClient::rename(const string &oldname, const string &newname)
{
    return true;
}

bool ftpClient::nlist(const string &remoteDirname, vector<string> &list)
{
    return true;
}

bool ftpClient::site(const string &cmd)
{
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
