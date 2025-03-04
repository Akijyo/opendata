#include "jsonns.h"
using namespace std;
using namespace nlohmann;

jsonns::jsonns(const string &fileName)
{
    if(matchstr(fileName,"^.*\\.json$")==false)
    {
        cout<<"�ļ�������json�ļ�"<<endl;
        return;
    }
    ifstream ifs(fileName, ios::in);
    if (ifs.is_open() == false)
    {
        cout << "���ļ�ʧ��" << endl;
        return;
    }
    try
    {
        ifs >> this->j;
    }
    catch (const exception &e)
    {
        cout << "����json�ļ�ʧ��" << endl;
        return;
    }
    ifs.close();
}
jsonns::jsonns(const jsonns &j, const string &key)
{
    if(j.j.find(key)==j.j.end())
    {
        cout<<"û���ҵ���Ӧ��key"<<endl;
        return;
    }
    this->j=j.j[key];
}

//������c��������ȡֵ����
bool jsonns::getchArr(const string &key, char *value, const int len)
{
    if(this->j.find(key)==this->j.end())//û���ҵ�key
    {
        return false;
    }
    if(len>=this->j[key].size())//����ⲿ������len���ڵ��������ַ�������,ֻ���������ַ���
    {
        strcpy(value,this->j[key].get<string>().c_str());
        value[this->j[key].size()]='\0';
    }
    else//����ⲿ������lenС�������ַ�������,ֻ����len���ȵ��ַ���
    {
        strncpy(value,this->j[key].get<string>().c_str(),len);
        value[len]='\0';
    }
    return true;
}