#include "../include/logfile.h"

using namespace std;

// �ж��ļ��Ƿ��
bool logfile::isOpen()
{
    return this->lfout.is_open();
}

// ���ļ�
bool logfile::open(const std::string &filename, ios::openmode mode, bool isBackup, int maxsize, bool isBuffer)
{
    // ���ļ��Ѿ��򿪣���ر��ļ�
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
        // �ر��ļ�������
        this->lfout.rdbuf()->pubsetbuf(nullptr, 0);
    }
    return true;
}

// �����ļ�
bool logfile::backupFile()
{
    // �ж��Ƿ���Ҫ�����ļ�
    if (this->isBackup == true)
    {
        // ��ȡ�ļ���С
        int size = fileSize(this->filename);
        // ���ļ���С�������ֵ���ұ��ݱ�־λΪtrue���򱸷��ļ�
        if (size >= this->maxsize * 1024 * 1024)
        {
            // ��ȡ��ǰʱ��
            std::string date = getCurTime(TimeType::TIME_TYPE_ONE);
            // ȥ�������еķ������ַ�
            date = pickNum(date);
            // �����ļ���Ϊ��ԭ�ļ���+.+����
            std::string newname = this->filename + "." + date;
            unique_lock<mutex> lck(this->mtx);
            {
                if (renamefile(this->filename, newname) == false)
                {
                    return false;
                }
                // �ر��ļ������´�
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