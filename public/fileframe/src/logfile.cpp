#include "../include/logfile.h"

using namespace std;

// 判断文件是否打开
bool logfile::isOpen()
{
    return this->lfout.is_open();
}

// 打开文件
bool logfile::open(const std::string &filename, ios::openmode mode, bool isBackup, int maxsize, bool isBuffer)
{
    // 若文件已经打开，则关闭文件
    if (this->lfout.is_open())
    {
        this->lfout.close();
    }

    this->filename = filename;
    this->isBackup = isBackup;
    this->maxsize = maxsize;
    this->mode = mode;
    this->isBuffer = isBuffer;
    this->lfout.open(filename, mode);
    if (!this->lfout.is_open())
    {
        return false;
    }
    if (isBuffer == false)
    {
        // 关闭文件缓冲区
        this->lfout.rdbuf()->pubsetbuf(nullptr, 0);
    }
    return true;
}

// 备份文件
bool logfile::backupFile()
{
    // 判断是否需要备份文件
    if (this->isBackup == true)
    {
        // 获取文件大小
        int size = fileSize(this->filename);
        // 若文件大小超过最大值，且备份标志位为true，则备份文件
        if (size >= this->maxsize * 1024 * 1024)
        {
            // 获取当前时间
            std::string date = getCurTime(TimeType::TIME_TYPE_ONE);
            // 去掉日期中的非数字字符
            date = pickNum(date);
            // 备份文件名为：原文件名+.+日期
            std::string newname = this->filename + "." + date;
            unique_lock<mutex> lck(this->mtx);
            {
                if (renamefile(this->filename, newname) == false)
                {
                    return false;
                }
                // 关闭文件并重新打开
                this->lfout.close();
                if (this->open(this->filename, this->mode, this->isBackup, this->maxsize, this->isBuffer) == false)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

logfile::~logfile()
{
    if (this->lfout.is_open())
    {
        this->lfout.close();
    }
}