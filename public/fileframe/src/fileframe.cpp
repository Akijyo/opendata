#include "../include/fileframe.h"

using namespace std;

//根据提供的文件的 绝对路径的目录路径或者文件路径逐级创建目录
bool newdir(const string &dir, bool isFile)
{
    if(dir.empty())
    {
        return false;
    }

    unsigned long pos = 1;//调过根目录的那个/
    while ((pos = dir.find('/', pos)) != string::npos)//循环查找下一个/
    {
        string subDir = dir.substr(0, pos++); // 截取要建立的子目录
        
        if (access(subDir.c_str(), F_OK) == -1)//判断目录是否存在，若不存在则执行里面语句
        {
            if (mkdir(subDir.c_str(), 0755) == -1)//创建目录
            {
                return false;
            }
        }
    }
    if (isFile == false)//如果不是文件路径，最后一个/后面是目录名，需要创建
    {
        if (access(dir.c_str(), F_OK) == -1)
        {
            mkdir(dir.c_str(), 0755);
        }
    }
    return true;
}

// 重命名绝对路径的文件或目录,类似mv命令，用于替代rename函数
bool renamefile(const string &oldname, const string &newname)
{
    if(oldname.empty() || newname.empty())
    {
        return false;
    }
    if(access(oldname.c_str(), F_OK)==-1)//判断原文件是否存在
    {
        return false;
    }
    if (newdir(newname, true) == false)//创建
    {
        return false;
    }
    if (rename(oldname.c_str(), newname.c_str()) == -1)//mv命令
    {
        return false;
    }
    return true;
}

// 复制文件，类似cp命令，用于替代copy函数
// --此文件中唯一使用c++17的文件系统库的函数
bool copyfile(const string &src, const string &dest)
{
    if(src.empty()||dest.empty())
    {
        return false;
    }
    if(access(src.c_str(), F_OK)==-1)//判断原文件是否存在
    {
        return false;
    }
    if (newdir(dest, true) == false)//判断新路径目录是否存在并且创建
    {
        return false;
    }
    try
    {
        filesystem::copy(src, dest, filesystem::copy_options::recursive | filesystem::copy_options::overwrite_existing);
    }
    catch(const filesystem::filesystem_error& e)
    {
        return false;
    }
    return true;
}

// 删除指定绝对路径的文件
bool deletefile(const string &file)
{
    if(file.empty())
    {
        return false;
    }
    if(access(file.c_str(), F_OK)==-1)//判断文件是否存在
    {
        return false;
    }
    if(remove(file.c_str())==-1)//删除文件
    {
        return false;
    }
    return true;
}

//读取文件的大小，单位为字节
int fileSize(const string &file)
{
    if (file.empty())
    {
        return -1;
    }
    struct stat statbuf;//创建文件信息结构体
    if(stat(file.c_str(),&statbuf)==-1)//获取文件信息
    {
        return -1;
    }
    return statbuf.st_size; // 返回文件大小，单位字节 byte
}

// 获取指定绝对路径的文件的最后修改时间
bool fileTime(const string &file, string &time, TimeType type)
{
    if(file.empty())
    {
        return false;
    }
    struct stat statbuf; // 创建文件信息结构体
    if(stat(file.c_str(),&statbuf)==-1)//获取文件信息
    {
        return false;
    }
    timeToStr(statbuf.st_mtime, time, type); // 将时间转换成字符串
    return true;
}
bool fileTime(const string &file, char *time, TimeType type)//C风格字符串重载
{
    if (file.empty())
    {
        return false;
    }
    string timeTemp;
    if (!fileTime(file, timeTemp, type))
    {
        return false;
    }
    strcpy(time, timeTemp.c_str()); // 将string类型转为c风格字符串
    return true;
}

// 设置文件的修改时间
bool setFileTime(const string &file, const string &time,TimeType type)
{
    if(file.empty()||time.empty())
    {
        return false;
    }
    struct utimbuf utimebuf; // 创建文件时间结构体
    utimebuf.actime = utimebuf.modtime = strToTime(time, type); // 将字符串转为时间
    if(utimebuf.actime==-1||utimebuf.modtime==-1)
    {
        return false;
    }
    if(utime(file.c_str(),&utimebuf)==-1)//设置文件时间
    {
        return false;
    }
    return true;
}