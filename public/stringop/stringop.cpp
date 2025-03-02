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
    while (*str==ch)//跳过左侧的指定字符
    {
        str++;
    }
    return str;
}
//删除字符串左侧的指定字符,C++版本
string& deleteLChar(string& str, const char ch)
{
    if (str.empty())
    {
        return str;
    }
    while (str[0]==ch)//跳过左侧的指定字符
    {
        str.erase(0,1);
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
    int len=str.length();
    while (str[len-1]==ch)//跳过右侧的指定字符
    {
        str.erase(len-1,1);
    }
    return str;
}
//删除字符串两侧的指定字符,C语言版本
