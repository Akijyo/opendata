#include "split.h"
using namespace std;
// 分割字符串的方法
bool ccmdstr::split(const string &str, const string &sep, bool isDeleteSpace)
{
    if (str.empty() || sep.empty()) // 传入的字符串为空
    {
        return false;
    }
    int index = 0; // 用于遍历的指针
    int start = 0; // 用于存放子字符串的起始位置
    string temp;   // 存放截获下来的临时字符串
    while ((index = str.find(sep, index)) != string::npos)
    {
        temp = str.substr(start, index - start);
        if (isDeleteSpace == true)
        {
            temp = deleteChar(temp); // 删除空格
        }
        if (temp.empty() == false)
        {
            this->vstr.push_back(temp);
        }
        index += sep.length();
        start = index;
    }
    temp = str.substr(start);
    if (isDeleteSpace == true)
    {
        deleteChar(temp);
    }
    if (temp.empty() == false)
    {
        this->vstr.push_back(temp);
    }
    return true;
}

// 超级构造函数，在构造期间完成字符串的分割
ccmdstr::ccmdstr(const string &str, const string &sep, bool isDeleteSpace)
{
    this->split(str, sep, isDeleteSpace);
}

bool ccmdstr::checkIndex(const int index)
{
    if (index < 0 || index >= this->vstr.size()) // 下标越界
    {
        return false;
    }
    return true;
}

// 重载[]运算符，返回分割后的字符串,像数组一样访问
const string ccmdstr::operator[](const int index)const
{
    if (index < 0 || index >= this->vstr.size()) // 下标越界
    {
        return string();
    }
    return this->vstr[index];
}

// 返回字符串数组的大小
int ccmdstr::size()const
{
    return this->vstr.size();
}

// 获取分割后字符串的对应类型的值，需要指定下标
bool ccmdstr::getValue(const int index, string &value)
{
    this->checkIndex(index);
    value = this->vstr[index];
    return true;
}
//提取c风格字符串需要程序员外部调用时指定长度
bool ccmdstr::getValue(const int index, char *value, const int len)
{
    if (len < 0)
    {
        return false;
    }
    this->checkIndex(index);
    if(len>=this->vstr[index].size())
    {
        strcpy(value, this->vstr[index].c_str());
        value[this->vstr[index].size()] = '\0';
    }
    else
    {
        strncpy(value,this->vstr[index].c_str(),len);
        value[len]='\0';
    }
    return true;
}
bool ccmdstr::getValue(const int index, int &value)
{
    this->checkIndex(index);
    try//用try防止stoi函数遇到错误时抛出异常退出程序
    {
        value = stoi(pickNum(this->vstr[index], true));
    }
    catch (const exception &e)
    {
        return false;
    }
    return true;
}
bool ccmdstr::getValue(const int index, unsigned int &value)
{
    this->checkIndex(index);
    try
    {
        value = stoi(pickNum(this->vstr[index]));
    }
    catch (const exception &e)
    {
        return false;
    }
    return true;
}
bool ccmdstr::getValue(const int index, long &value)
{
    this->checkIndex(index);
    try
    {
        value = stol(pickNum(this->vstr[index], true));
    }
    catch (const exception &e)
    {
        return false;
    }
    return true;
}
bool ccmdstr::getValue(const int index, unsigned long &value)
{
    this->checkIndex(index);
    try
    {
        value = stoul(pickNum(this->vstr[index]));
    }
    catch (const exception &e)
    {
        return false;
    }
    return true;
}
bool ccmdstr::getValue(const int index, long long &value)
{
    this->checkIndex(index);
    try
    {
        value = stoll(pickNum(this->vstr[index], true));
    }
    catch (const exception &e)
    {
        return false;
    }
    return true;
}
bool ccmdstr::getValue(const int index, unsigned long long &value)
{
    this->checkIndex(index);
    try
    {
        value = stoull(pickNum(this->vstr[index]));
    }
    catch (const exception &e)
    {
        return false;
    }
    return true;
}
bool ccmdstr::getValue(const int index, float &value)
{
    this->checkIndex(index);
    try
    {
        value = stof(pickNum(this->vstr[index], true, true));
    }
    catch (const exception &e)
    {
        return false;
    }
    return true;
}
bool ccmdstr::getValue(const int index, double &value)
{
    this->checkIndex(index);
    try
    {
        value = stod(pickNum(this->vstr[index], true, true));
    }
    catch (const exception &e)
    {
        return false;
    }
    return true;
}
bool ccmdstr::getValue(const int index, long double &value)
{
    this->checkIndex(index);
    try
    {
        value = stold(pickNum(this->vstr[index], true, true));
    }
    catch (const exception &e)
    {
        return false;
    }
    return true;
}
bool ccmdstr::getValue(const int index, bool &value)
{
    this->checkIndex(index);
    string temp = this->vstr[index];
    toUpper(temp);
    if (temp == "TRUE" || temp == "YES" || temp == "1")
    {
        value = true;
    }
    else if (temp == "FALSE" || temp == "NO" || temp == "0")
    {
        value = false;
    }
    else
    {
        return false;
    }
    return value;
}

// 重载<<运算符让打印函数可以直接打印ccmdstr对象
ostream &operator<<(ostream &os, const ccmdstr &cmdstr)
{
    for (int i = 0; i < cmdstr.size(); i++)
    {
        os << "[" << i << "]:" << cmdstr[i] << endl;
    }
    return os;
}