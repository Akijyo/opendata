#include "jsonns.h"
using namespace std;
using namespace nlohmann;

jsonns::jsonns(const string &fileName)
{
    if(matchstr(fileName,"^.*\\.json$")==false)
    {
        cout<<"文件名不是json文件"<<endl;
        return;
    }
    ifstream ifs(fileName, ios::in);
    if (ifs.is_open() == false)
    {
        cout << "打开文件失败" << endl;
        return;
    }
    try
    {
        ifs >> this->j;
    }
    catch (const exception &e)
    {
        cout << "解析json文件失败" << endl;
        return;
    }
    ifs.close();
}
jsonns::jsonns(const jsonns &j, const string &key)
{
    if(j.j.find(key)==j.j.end())
    {
        cout<<"没有找到对应的key"<<endl;
        return;
    }
    this->j=j.j[key];
}

//独立的c语言数组取值函数
bool jsonns::getchArr(const string &key, char *value, const int len)
{
    if(this->j.find(key)==this->j.end())//没有找到key
    {
        return false;
    }
    if(len>=this->j[key].size())//如果外部给定的len大于等于类中字符串长度,只复制类中字符串
    {
        strcpy(value,this->j[key].get<string>().c_str());
        value[this->j[key].size()]='\0';
    }
    else//如果外部给定的len小于类中字符串长度,只复制len长度的字符串
    {
        strncpy(value,this->j[key].get<string>().c_str(),len);
        value[len]='\0';
    }
    return true;
}