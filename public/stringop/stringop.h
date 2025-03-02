//
// Created by akijyo on 25-3-1.
// 这是开发框架中的字符串操作的通用函数声明
#pragma once

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

/**
 * 删除字符串左右两边的指定字符，默认缺省为空格
 * @param str 要处理的字符串
 * @param ch 要删除的字符
 * @return  处理后的字符串
 */
char* deleteChar(char* str,const char ch=' ');
std::string& deleteChar(std::string& str,const char ch=' ');

/**
 * @brief 将字符串中的小写字母转换为大写字母
 * 
 * @param str 要处理的字符串
 * @return 处理完后的字符串
 */
char* toUpper(char* str);
std::string& toUpper(std::string& str);

/**
 * @brief 将字符串的大写字母全部转换为小写字母
 * 
 * @param str 要处理的字符串
 * @return 处理完后的字符串
 */
char *toLower(char *str);
std::string &toLower(std::string &str);

/**
 * @brief 将目标字符串str中的src字符串替换为dest字符串
 * 
 * @param str 目标字符串
 * @param src 被替换的字符串
 * @param dest 需要替换成的字符串
 * @param isLoop 是否循环替换，默认为false
 * @return true 
 * @return false 
 */
bool replace(char *str, const char *src, const char *dest, const bool isLoop = false);
bool replace(std::string &str, const std::string &src, const std::string &dest, const bool isLoop = false);
