#pragma once
#include "stringop.h"
#include <cstddef>

// json文件读取类
// 用于读取json文件中的数据
// json文件的格式为：
// {
//     "key1": "value1",
//     "key2": "value2",
// }
// 并且将json文件中的数据按照key-value的方式读取

class jsonns
{
  private:
    nlohmann::json j;

  public:
    jsonns() {};
    jsonns(const jsonns &) = delete;
    jsonns &operator=(const jsonns &) = delete;
    // 构造函数，给定文件名，读取json文件
    jsonns(const std::string &fileName);
    // 对于嵌套json的构造存储
    jsonns(const jsonns &j, const std::string &key);

    /**
     * @brief //对于简单json文件的读取，按key-value方式读取
     * 写成模版，但是保留char*的版本
     * @param key
     * @param value 存放读取到的值
     * @return true
     * @return false
     */
    bool getchArr(const std::string &key, char *value, const unsigned long len);

    /**
     * @brief 返回json文件中对应key的值的模版函数
     *
     * @tparam T 模版类型
     * @param key
     * @param value
     * @return true
     * @return false
     */
    template <class T> bool get(const std::string &key, T &value)
    {
        if (this->j.find(key) == this->j.end()) // 没有找到key
        {
            return false;
        }
        try
        {
            value = this->j[key].get<T>();
        }
        catch (const std::exception &e)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief 当次json对象是一个json数组时，从数组里按index取值
     * 模版函数
     * @tparam T
     * @param index 键值
     * @param value 存放读取到的值
     * @return true
     * @return false
     */
    template <class T> bool getForArr(const int index, T &value)
    {
        if (this->j.is_array() == false)
        {
            std::cerr << "此json对象不是数组" << std::endl;
            return false;
        }
        if (this->j.size() <= index || index < 0)
        {
            return false;
        }
        try
        {
            value = this->j[index].get<T>();
        }
        catch (const std::exception &e)
        {
            return false;
        }
        return true;
    }
};