#pragma once
#include "stringop.h"

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
    bool checkIndex(const unsigned long index);

  public:
    ccmdstr() {};
    ccmdstr(const ccmdstr &) = delete;
    ccmdstr &operator=(const ccmdstr &) = delete;

    // 超级构造函数，在构造期间完成字符串的分割
    ccmdstr(const std::string &src, const std::string &sep, bool isDeleteSpace = false);

    // 重载[]运算符，返回分割后的字符串,像数组一样访问
    const std::string operator[](const unsigned long index)const;

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
    int size()const;

    /**
     * @brief 获取分割后字符串的对应类型的值，需要指定下表
     *
     * @param index 数组下标
     * @param value 用来获取结果的值
     * @return true
     * @return false
     */
    bool getValue(const int index, std::string &value);
    bool getValue(const int index, char *value, const unsigned long len);
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
//重载<<运算符让打印函数可以直接打印ccmdstr对象
std::ostream &operator<<(std::ostream &os, const ccmdstr &cmdstr);