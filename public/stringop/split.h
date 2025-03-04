#pragma once
#include "stringop.h"

// �ַ����ָ���ccmdstr
// ���ڽ��ַ�������ָ���ķָ������ַ������ָ�ɶ�����ַ���
// �ַ����ĸ�ʽΪ���ֶ�����1+�ָ���+�ֶ�����2+�ָ���+�ֶ�����3+�ָ���+...+�ֶ�����n��
// ���磺"messi,10,striker,30,1.72,68.5,Barcelona"�����������˶�Ա÷�������ϡ�
// ���������������º��롢����λ�á����䡢��ߡ����غ�Ч���ľ��ֲ����ֶ�֮���ð�ǵĶ��ŷָ���
class ccmdstr
{
  private:
    std::vector<std::string> vstr; // ��ŷָ����ַ���
    // ���index�Ƿ�Խ��
    bool checkIndex(const int index);

  public:
    ccmdstr() {};
    ccmdstr(const ccmdstr &) = delete;
    ccmdstr &operator=(const ccmdstr &) = delete;

    // �������캯�����ڹ����ڼ�����ַ����ķָ�
    ccmdstr(const std::string &src, const std::string &sep, bool isDeleteSpace = false);

    // ����[]����������طָ����ַ���,������һ������
    const std::string operator[](const int index)const;

    /**
     * @brief ���ĺ���������ָ�����ַ������зָȻ�󽫽����ŵ�vstr��
     *
     * @param src Ҫ�ָ���ַ���
     * @param sep ����ʲô�ַ����ָ�
     * @param isDeleteSpace �Ƿ�ɾ���ָ����ַ������ߵĿո�
     * @return true
     * @return false
     */
    bool split(const std::string &src, const std::string &sep, bool isDeleteSpace = false);

    // ���طָ����ַ�������
    int size()const;

    /**
     * @brief ��ȡ�ָ���ַ����Ķ�Ӧ���͵�ֵ����Ҫָ���±�
     *
     * @param index �����±�
     * @param value ������ȡ�����ֵ
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
//����<<������ô�ӡ��������ֱ�Ӵ�ӡccmdstr����
std::ostream &operator<<(std::ostream &os, const ccmdstr &cmdstr);