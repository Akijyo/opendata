#include "logfile.h"
using namespace std;

// 判断文件是否打开
bool logfile::isOpen()
{
    return this->lfout.is_open();
}

// 打开文件
bool logfile::open(const std::string &filename, ios::openmode mode, bool isBackup, int maxsize, bool isBuffer)
{
    if(this->lfout.is_open())
    {
        this->lfout.close();
    }
    this->filename = filename;
    this->isBackup = isBackup;
    this->maxsize = maxsize;
    this->mode = mode;
    this->isBuffer = isBuffer;
    this->lfout.open(filename, mode);
    if(!this->lfout.is_open())
    {
        return false;
    }
    if(isBuffer==false)
    {
        this->lfout.rdbuf()->pubsetbuf(nullptr, 0);
    }
    return true;
}