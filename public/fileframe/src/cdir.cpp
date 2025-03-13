#include "../include/cdir.h"
using namespace std;
using namespace filesystem;
//返回文件列表的大小
unsigned int cdir::size()
{
    return filelist.size();
}

//打开目录
bool cdir::openDir(const string &dir, const string &match, const int maxsize, bool recursive, bool isSort)
{
    this->pos = 0;
    if(this->filelist.empty() == false)//如果文件列表不为空，清空文件列表
    {
        this->filelist.clear();
    }
    if(dir.empty())//如果目录为空，返回false
    {
        return false;
    }

    string file;
    if (recursive == false)
    {
        for (auto &entry : directory_iterator(dir))//不递归遍历
        {
            if((unsigned long)maxsize<=this->filelist.size())//如果文件列表大小超过最大值，停止遍历
            {
                break;
            }
            if (entry.is_regular_file() == true)//只要普通的文件
            {
                file = entry.path().string();//从directory_entry中获取path类再获取string
                if (matchstr(file, match) == true)//匹配文件名，调用stringop.h中的函数
                {
                    this->filelist.push_back(file);
                }
            }
        }
    }
    else
    {
        for (auto &entry : recursive_directory_iterator(dir))//递归遍历
        {
            //函数体与上半部一致
            if((unsigned long)maxsize<=this->filelist.size())
            {
                break;
            }
            if (entry.is_regular_file() == true)
            {
                file = entry.path().string();
                if (matchstr(file, match) == true)
                {
                    this->filelist.push_back(file);
                }
            }
        }
    }
    //如果需要排序则调用sort函数
    if (isSort == true)
    {
        sort(this->filelist.begin(), this->filelist.end());
    }
    return true;
}

//根据pos位置读取文件信息，调用一次就读一个文件
bool cdir::readFile()
{
    if (this->pos>=this->filelist.size())
    {
        this->pos = 0;
        this->filelist.clear();
        return false;
    }
    this->fullpath = this->filelist[this->pos];
    this->filename = this->fullpath.substr(this->fullpath.find_last_of('/') + 1);
    this->dirname = this->fullpath.substr(0, this->fullpath.find_last_of('/'));
    this->filesize = file_size(this->fullpath);

    //////////////c++17只提供了获取和设置最后修改时间的函数last_write_time/////////
    //////////////没有提供获取和设置创建时间和最后访问时间的函数///////////////////
    //////////////所以下面是linux c下的语法///////////////////////////////////////
    struct stat statbuf;//创建文件信息结构体stat
    if(stat(this->fullpath.c_str(),&statbuf)==-1)//调用stat函数将文件信息存入statbuf
    {
        return false;
    }
    this->modifytime=timeToStr(statbuf.st_mtime, this->type);//调用timeframe.h中的函数将时间转换成字符串
    this->createtime=timeToStr(statbuf.st_ctime, this->type);
    this->accesstime = timeToStr(statbuf.st_atime, this->type);
    //////////////LinuxC的语法结束///////////////////////////////////////////////

    this->pos++;
    return true;
}