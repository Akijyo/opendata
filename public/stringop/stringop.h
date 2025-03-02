//
// Created by akijyo on 25-3-1.
// 这是开发框架中的字符串操作的通用函数声明
#pragma once

#include "../cpublic.h" //c/c++常用头文件，如有新增在此文件中添加


/**
 * 删除字符串从左边开始的指定字符，默认缺省为空格
 * 例如：" h ello"返回"h ello"
 * @param str 要处理的字符串
 * @param ch 要删除的字符
 * @return 处理后的字符串
 */
char* deleteLChar(char* str,const char ch=' ');
std::string& deleteLChar(std::string& str,const char ch=' ');

/**
 *删除字符串从尾部开始的指定字符，默认缺省为空格
 *例如："hell o "返回"hell o"
 * @param str 要处理的字符串
 * @param ch 要删除的字符
 * @return 处理后的字符串
 */
char* deleteRChar(char* str,const char ch=' ');
std::string& deleteRChar(std::string& str,const char ch=' ');

/**
 * 删除字符串左右两边的指定字符，默认缺省为空格
 * 例如：" h ello "返回"h ello"
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
 * 例如："hello world"中的"world"替换为"you"，则返回"hello you"
 * @param str 目标字符串
 * @param src 被替换的字符串
 * @param dest 需要替换成的字符串
 * @param isLoop 是否循环替换，默认为false
 * @return true 
 * @return false 
 */
bool replaceStr(char *str, const char *src, const char *dest, const bool isLoop = false);
bool replaceStr(std::string &str, const std::string &src, const std::string &dest, const bool isLoop = false);

/**
 * @brief 从一个字符中提取数字，并放到另一个字符串中，可以选择提取符号和小数点
 * 例如："hello 123.456 world"提取出"123.456"
 * @param src 需要提取的字符串
 * @param dest 数字放置的字符串
 * @param bSigned 是否提取+-符号，默认为false
 * @param bFloat 是否提取小数点，默认为false
 * @return 存放数字等的字符串
 */
char* pickNum(const std::string& src,char* dest,const bool bSigned=false,const bool bFloat=false);
std::string &pickNum(const std::string &src, std::string &dest, const bool bSigned = false, const bool bFloat = false);
std::string pickNum(const std::string &src, const bool bSigned = false, const bool bFloat = false);

/**
 * @brief 正则表达式匹配处理函数
 * 
 * @param str 需要去匹配的字符串
 * @param pattern 正则表达式匹配规则
 * @return true 
 * @return false 
 */
bool matchstr(const std::string &str, const std::string &pattern);