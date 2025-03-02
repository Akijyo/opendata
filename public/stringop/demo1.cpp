//
// Created by akijyo on 25-3-1.
//

#include "stringop.h"
using namespace std;

int main()
{
    char str1[] = " hello";
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
    replace(str1, "world", "you");
    replace(str2, "world", "you");
    cout << "replace(str1):" << str1 << endl;
    cout << "replace(str2):" << str2 << endl;
    return 0;
}