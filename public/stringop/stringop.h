//
// Created by akijyo on 25-3-1.
// 这是开发框架中的字符串操作的通用函数声明
#pragma once

#include "../cpublic.h" //c/c++常用头文件，如有新增在此文件中添加
#include <string>
#include <vector>

/**
 * 删除字符串从左边开始的指定字符，默认缺省为空格
 * 例如：" h ello"返回"h ello"
 * @param str 要处理的字符串
 * @param ch 要删除的字符
 * @return 处理后的字符串
 */
char *deleteLChar(char *str, const char ch = ' ');
std::string &deleteLChar(std::string &str, const char ch = ' ');

/**
 *删除字符串从尾部开始的指定字符，默认缺省为空格
 *例如："hell o "返回"hell o"
 * @param str 要处理的字符串
 * @param ch 要删除的字符
 * @return 处理后的字符串
 */
char *deleteRChar(char *str, const char ch = ' ');
std::string &deleteRChar(std::string &str, const char ch = ' ');

/**
 * 删除字符串左右两边的指定字符，默认缺省为空格
 * 例如：" h ello "返回"h ello"
 * @param str 要处理的字符串
 * @param ch 要删除的字符
 * @return  处理后的字符串
 */
char *deleteChar(char *str, const char ch = ' ');
std::string &deleteChar(std::string &str, const char ch = ' ');

/**
 * @brief 将字符串中的小写字母转换为大写字母
 *
 * @param str 要处理的字符串
 * @return 处理完后的字符串
 */
char *toUpper(char *str);
std::string &toUpper(std::string &str);

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
char *pickNum(const std::string &src, char *dest, const bool bSigned = false, const bool bFloat = false);
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

///////////////////////////  ////////////////////////////

///////////////////////////  ////////////////////////////

// 字符串分割类ccmdstr
// 用于将字符串按照指定的分隔符（字符串）分割成多个子字符串
// 字符串的格式为：字段内容1+分隔符+字段内容2+分隔符+字段内容3+分隔符+...+字段内容n。
// 例如："messi,10,striker,30,1.72,68.5,Barcelona"，这是足球运动员梅西的资料。
// 包括：姓名、球衣号码、场上位置、年龄、身高、体重和效力的俱乐部，字段之间用半角的逗号分隔。
class ccmdstr
{
  private:
    std::vector<std::string> vstr; // 存放分割后的字符串
    // 检查index是否越界
    bool checkIndex(const int index);

  public:
    ccmdstr() {};
    ccmdstr(const ccmdstr &) = delete;
    ccmdstr &operator=(const ccmdstr &) = delete;

    // 超级构造函数，在构造期间完成字符串的分割
    ccmdstr(const std::string &src, const std::string &sep, bool isDeleteSpace = false);

    // 重载[]运算符，返回分割后的字符串,像数组一样访问
    std::string operator[](const int index);

    /**
     * @brief 核心函数，按照指定的字符串进行分割，然后将结果存放到vstr中
     *
     * @param src 要分割的字符串
     * @param sep 按照什么字符串分割
     * @param isDeleteSpace 是否删除分割后的字符串两边的空格
     * @return true
     * @return false
     */
    bool split(const std::string &src, const std::string &sep, bool isDeleteSpace = false);

    // 返回分割后的字符串个数
    int size();

    /**
     * @brief 获取分割后字符串的对应类型的值，需要指定下表
     *
     * @param index 数组下标
     * @param value 用来获取结果的值
     * @return true
     * @return false
     */
    bool getValue(const int index, std::string &value);
    bool getValue(const int index, char *value, const int len);
    bool getValue(const int index, int &value);
    bool getValue(const int index, unsigned int &value);
    bool getValue(const int index, long &value);
    bool getValue(const int index, unsigned long &value);
    bool getValue(const int index, long long &value);
    bool getValue(const int index, unsigned long long &value);
    bool getValue(const int index, float &value);
    bool getValue(const int index, double &value);
    bool getValue(const int index, long double &value);
    bool getValue(const int index, bool &value);
};