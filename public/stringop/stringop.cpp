//
// Created by akijyo on 25-3-1.
//字符串操作函数的实现

#include "stringop.h"
using namespace std;

//删除字符串左侧的指定字符,C语言版本
char* deleteLChar(char* str, const char ch)
{
    if (str==nullptr)//传入的字符串为空
    {
        return nullptr;
    }
    char* readPtr=str;
    char* writePtr=str;
    while (*readPtr==ch)//跳过左侧的指定字符
    {
        readPtr++;
    }
    while (*readPtr!='\0')//复制剩余的字符串
    {
        *writePtr=*readPtr;
        writePtr++;
        readPtr++;
    }
    *writePtr='\0';
    return str;
}
//删除字符串左侧的指定字符,C++版本
string& deleteLChar(string& str, const char ch)
{
    if (str.empty())
    {
        return str;
    }
    int start = 0;
    while(start<str.length()&&str[start]==ch)//跳过左侧的指定字符
    {
        start++;
    }
    if(start>0)
    {
        str.erase(0, start);
    }
    return str;
}
//删除字符串右侧的指定字符,C语言版本
char* deleteRChar(char* str, const char ch)
{
    if (str==nullptr)//传入的字符串为空
    {
        return nullptr;
    }
    int len=strlen(str);
    while (len>0&&str[len-1]==ch)//跳过右侧的指定字符
    {
        str[len-1]='\0';
        len--;
    }
    return str;
}
//删除字符串右侧的指定字符,C++版本
string& deleteRChar(string& str, const char ch)
{
    if (str.empty())
    {
        return str;
    }
    int end = str.length() - 1;
    while(end>=0&&str[end]==ch)//跳过右侧的指定字符
    {
        end--;
    }
    if(end<str.length()-1)
    {
        str.resize(end+1);
    }
    return str;
}
//删除字符串两侧的指定字符,C语言版本
char* deleteChar(char* str,const char ch)
{
    if(str==nullptr)//传入的字符串为空
    {
        return nullptr;
    }
    str=deleteLChar(str,ch);//删除左侧的指定字符
    str=deleteRChar(str,ch);//删除右侧的指定字符
    return str;
}
//删除字符串两侧的指定字符,C++版本
string& deleteChar(string& str,const char ch)
{
    if(str.empty())
    {
        return str;
    }
    str=deleteLChar(str,ch);//删除左侧的指定字符
    str=deleteRChar(str,ch);//删除右侧的指定字符
    return str;
}
//将字符串中的小写字母转换为大写字母,C语言版本
char* toUpper(char* str)
{
    if(str==nullptr)//传入的字符串为空
    {
        return nullptr;
    }
    char *ptr = str;
    while(*ptr!='\0')
    {
        if(*ptr>='a'&&*ptr<='z')//小写字母转换为大写字母
        {
            *ptr=*ptr-32;
        }
        ptr++;
    }
    return str;
}
//将字符串中的小写字母转换为大写字母,C++版本
string& toUpper(string& str)
{
    if(str.empty())//传入的字符串为空
    {
        return str;
    }
    for(auto& ch:str)
    {
        if(ch>='a'&&ch<='z')//小写字母转换为大写字母
        {
            ch=ch-32;
        }
    }
    return str;
}
//将字符串中的大写字母转换为小写字母,C语言版本
char* toLower(char* str)
{
    if(str==nullptr)//传入的字符串为空
    {
        return nullptr;
    }
    char *ptr = str;
    while(*ptr!='\0')
    {
        if(*ptr>='A'&&*ptr<='Z')//大写字母转换为小写字母
        {
            *ptr=*ptr+32;
        }
        ptr++;
    }
    return str;
}
//将字符串中的大写字母转换为小写字母,C++版本
string& toLower(string& str)
{
    if(str.empty())//传入的字符串为空
    {
        return str;
    }
    for(auto& ch:str)
    {
        if(ch>='A'&&ch<='Z')//大写字母转换为小写字母
        {
            ch=ch+32;
        }
    }
    return str;
}
//将目标字符串str中的src字符串替换为dest字符串,C语言版本
bool replace(char *str, const char *src, const char *dest, const bool isLoop)
{
    if(isLoop==true&&strstr(dest,src)!=nullptr)//替换后的字符串中包含被替换的字符串
    {
        return false;//循环替换会导致死循环
    }
    char *pos = strstr(str, src);
    if(pos==nullptr)//目标字符串中不包含被替换的字符串
    {
        return false;
    }
    int srcLen = strlen(src);
    int destLen = strlen(dest);
    int strLen = strlen(str);
    
}
