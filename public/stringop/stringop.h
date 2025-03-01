//
// Created by akijyo on 25-3-1.
// 这是开发框架中的字符串操作的通用函数声明

#ifndef STRINGOP_H
#define STRINGOP_H

#include "../cpublic.h" //c/c++常用头文件，如有新增在此文件中添加


/**
 * 删除字符串从左边开始的指定字符，默认缺省为空格
 * @param str 要处理的字符串
 * @param ch 要删除的字符
 * @return 处理后的字符串
 */
char* deleteLChar(char* str,const char ch=' ');
std::string& deleteLChar(std::string& str,const char ch=' ');

/**
 *删除字符串从尾部开始的指定字符，默认缺省为空格
 * @param str 要处理的字符串
 * @param ch 要删除的字符
 * @return 处理后的字符串
 */
char* deleteRChar(char* str,const char ch=' ');
std::string& deleteRChar(std::string& str,const char ch=' ');


#endif //STRINGOP_H
