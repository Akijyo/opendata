#pragma once
#include "stringop.h"

// json�ļ���ȡ��
// ���ڶ�ȡjson�ļ��е�����
// json�ļ��ĸ�ʽΪ��
// {
//     "key1": "value1",
//     "key2": "value2",
// }
// ���ҽ�json�ļ��е����ݰ���key-value�ķ�ʽ��ȡ

class jsonns
{
  private:
    nlohmann::json j;

  public:
    jsonns() {};
    jsonns(const jsonns &) = delete;
    jsonns &operator=(const jsonns &) = delete;
    // ���캯���������ļ�������ȡjson�ļ�
    jsonns(const std::string &fileName);
    // ����Ƕ��json�Ĺ���洢
    jsonns(const jsonns &j, const std::string &key);

    /**
     * @brief //���ڼ�json�ļ��Ķ�ȡ����key-value��ʽ��ȡ
     * д��ģ�棬���Ǳ���char*�İ汾
     * @param key
     * @param value ��Ŷ�ȡ����ֵ
     * @return true
     * @return false
     */
    bool getchArr(const std::string &key, char *value, const int len);

    /**
     * @brief ����json�ļ��ж�Ӧkey��ֵ��ģ�溯��
     *
     * @tparam T ģ������
     * @param key
     * @param value
     * @return true
     * @return false
     */
    template <class T> bool get(const std::string &key, T &value)
    {
        if (this->j.find(key) == this->j.end()) // û���ҵ�key
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
     * @brief ����json������һ��json����ʱ���������ﰴindexȡֵ
     * ģ�溯��
     * @tparam T
     * @param index ��ֵ
     * @param value ��Ŷ�ȡ����ֵ
     * @return true
     * @return false
     */
    template <class T> bool getForArr(const int index, T &value)
    {
        if (this->j.is_array() == false)
        {
            std::cerr << "��json����������" << std::endl;
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