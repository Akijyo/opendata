#include "split.h"
using namespace std;
// �ָ��ַ����ķ���
bool ccmdstr::split(const string &str, const string &sep, bool isDeleteSpace)
{
    if (str.empty() || sep.empty()) // ������ַ���Ϊ��
    {
        return false;
    }
    int index = 0; // ���ڱ�����ָ��
    int start = 0; // ���ڴ�����ַ�������ʼλ��
    string temp;   // ��Žػ���������ʱ�ַ���
    while ((index = str.find(sep, index)) != string::npos)
    {
        temp = str.substr(start, index - start);
        if (isDeleteSpace == true)
        {
            temp = deleteChar(temp); // ɾ���ո�
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

// �������캯�����ڹ����ڼ�����ַ����ķָ�
ccmdstr::ccmdstr(const string &str, const string &sep, bool isDeleteSpace)
{
    this->split(str, sep, isDeleteSpace);
}

bool ccmdstr::checkIndex(const int index)
{
    if (index < 0 || index >= this->vstr.size()) // �±�Խ��
    {
        return false;
    }
    return true;
}

// ����[]����������طָ����ַ���,������һ������
const string ccmdstr::operator[](const int index)const
{
    if (index < 0 || index >= this->vstr.size()) // �±�Խ��
    {
        return string();
    }
    return this->vstr[index];
}

// �����ַ�������Ĵ�С
int ccmdstr::size()const
{
    return this->vstr.size();
}

// ��ȡ�ָ���ַ����Ķ�Ӧ���͵�ֵ����Ҫָ���±�
bool ccmdstr::getValue(const int index, string &value)
{
    this->checkIndex(index);
    value = this->vstr[index];
    return true;
}
//��ȡc����ַ�����Ҫ����Ա�ⲿ����ʱָ������
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
    try//��try��ֹstoi������������ʱ�׳��쳣�˳�����
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

// ����<<������ô�ӡ��������ֱ�Ӵ�ӡccmdstr����
ostream &operator<<(ostream &os, const ccmdstr &cmdstr)
{
    for (int i = 0; i < cmdstr.size(); i++)
    {
        os << "[" << i << "]:" << cmdstr[i] << endl;
    }
    return os;
}