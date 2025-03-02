//
// Created by akijyo on 25-3-1.
//

#include "stringop.h"
using namespace std;

int main()
{
    char str1[100] = " hello";
    string str2 = " hello";
    cout << "deleteLChar(str1):" << deleteLChar(str1) << endl;
    cout << "deleteLChar(str2):" << deleteLChar(str2) << endl;

    strcpy(str1, "hello ");
    str2 = "hello ";
    cout << "deleteRChar(str1):" << deleteRChar(str1) << endl;
    cout << "deleteRChar(str2):" << deleteRChar(str2) << endl;

    strcpy(str1, " hello ");
    str2 = " hello ";
    cout << "deleteChar(str1):" << deleteChar(str1) << endl;
    cout << "deleteChar(str2):" << deleteChar(str2) << endl;

    strcpy(str1, "hello");
    str2 = "hello";
    cout << "toUpper(str1):" << toUpper(str1) << endl;
    cout << "toUpper(str2):" << toUpper(str2) << endl;

    strcpy(str1, "HELLO");
    str2 = "HELLO";
    cout << "toLower(str1):" << toLower(str1) << endl;
    cout << "toLower(str2):" << toLower(str2) << endl;

    strcpy(str1, "hello world");
    str2 = "hello world";
    replaceStr(str1, "world", "you");
    replaceStr(str2, "world", "you");
    cout << "replace(str1):" << str1 << endl;
    cout << "replace(str2):" << str2 << endl;
    
    strcpy(str1, "hello 123.456+789 world");
    str2 = "hello 123.456+789 world";
    char dest1[100];
    string dest2;
    cout<<"pickNum(str1,dest1):"<<pickNum(str1,dest1)<<endl;
    cout<<"pickNum(str2,dest2):"<<pickNum(str2,dest2)<<endl;
    strcpy(str1,"hello 123.456+789 world");
    str2="hello 123.456+789 world";
    memset(dest1,0,sizeof(dest1));
    dest2.clear();
    cout<<"pickNum(str1,dest1,true):"<<pickNum(str1,dest1,true)<<endl;
    cout<<"pickNum(str2,dest2,true):"<<pickNum(str2,dest2,true)<<endl;
    strcpy(str1,"hello 123.456+789 world");
    str2="hello 123.456+789 world";
    memset(dest1,0,sizeof(dest1));
    dest2.clear();
    cout<<"pickNum(str1,dest1,true,true):"<<pickNum(str1,dest1,true,true)<<endl;
    cout<<"pickNum(str2,dest2,true,true):"<<pickNum(str2,dest2,true,true)<<endl;

    if(matchstr("public.cpp","^.*\.c$"))
    {
        cout<<"匹配成功"<<endl;
    }
    else
    {
        cout<<"匹配失败"<<endl;
    }

    return 0;
}