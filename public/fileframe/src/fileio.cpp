#include "../include/fileio.h"

using namespace std;

// 判断文件是否已经打开
bool wtfile::isOpen() const
{
    return this->fout.is_open();
}

// 打开文件
bool wtfile::open(const std::string &filename, bool temp, std::ios::openmode mode, bool buffer)
{
    if(fout.is_open())//如果文件已经打开，则关闭文件
    {
        fout.close();
    }
    this->filename = filename;
    this->tempfilename.clear();

    if (temp)//如果启用临时文件
    {
        this->tempfilename = this->filename + ".temp";//拼接临时文件名
        this->fout.open(this->tempfilename,mode);
    }
    else
    {
        this->fout.open(this->filename,mode);
    }

    if (buffer==false)//如果不适用文件缓冲区
    {
        this->fout.rdbuf()->pubsetbuf(nullptr, 0);//关闭文件缓冲区
    }
    return this->fout.is_open();
}

// 写入二进制文件
bool wtfile::writeBin(void *data, size_t size)
{
    if (this->fout.is_open() == false)
    {
        return false;
    }
    //写入二进制数据,ofstream的write函数是用于写入二进制数据的，static_cast是强制类型转换(c++11)
    this->fout.write((static_cast<char *>(data)), size);
    return this->fout.good();
}

// 关闭文件
void wtfile::close()
{
    if (this->fout.is_open()==false)
    {
        return;
    }
    if (this->tempfilename.empty() == false) // 如果临时文件名不为空，说明启用了临时文件
    {
        renamefile(this->tempfilename, this->filename); // 将临时文件改为正式文件
    }
    this->tempfilename.clear();
    this->filename.clear();
    this->fout.close();
}

wtfile::~wtfile()
{
    this->close();
}

// wtfile end
/////////////////////////////////////////////////////////////////////////////
// rdfile start

// 判断文件是否已经打开
bool rdfile::isOpen() const
{
    return this->fin.is_open();
}

// 打开文件
bool rdfile::open(const string &filename, ios::openmode mode)
{
    if(fin.is_open())//如果文件已经打开，则关闭文件
    {
        fin.close();
    }
    this->filename = filename;
    this->fin.open(this->filename, mode);
    return this->fin.is_open();
}

// 读取一行
bool rdfile::readLine(string &str, const string &endSign)
{
    if(this->fin.is_open()==false)
    {
        return false;
    }
    str.clear(); // 清空传入的字符串
    string line; // 存放读取到的一行，这个函数需要判断结束符所以需要定义这个string去拼接
    while (true)
    {
        getline(this->fin, line); // std::string的getline函数，读取一行

        //对于eof标志，当程序调用读取函数（如getline）时，若发现这次调用已经没有数据可读了，就会设置eof标志
        if(this->fin.eof())
        {
            break;
        }
        str += line; // 拼接读取到的一行

        if(endSign.empty()==true)//如果没有结束符
        {
            return true;
        }

        // 行结束符，而不是结束符，一定是一行的末尾结束位置有结束符才中断
        // 对于find中的第二个参数，是直接从末尾查找，能提高查找效率
        if(endSign.empty()==false && line.find(endSign,str.size()-endSign.size())!=string::npos)//如果找到结束符
        {
            return true;
        }

        str+="\n";//拼接换行符,因为getline函数会去掉换行符
    }
    return false;
}

// 读取二进制文件
bool rdfile::readBin(void *data, size_t size)
{
    if (this->fin.is_open() == false)
    {
        return false;
    }
    //读取二进制数据,ifstream的read函数是用于读取二进制数据的，static_cast是强制类型转换(c++11)
    this->fin.read((static_cast<char *>(data)), size);
    return this->fin.good();
}

// 关闭文件
void rdfile::close()
{
    if (this->fin.is_open()==false)
    {
        return;
    }
    this->filename.clear();
    this->fin.close();
}

rdfile::~rdfile()
{
    this->close();
}