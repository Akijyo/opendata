#pragma once
#include "../cpublic.h" //c/c++常用头文件，如有新增在此文件中添加
#include "../stringop/stringop.h"

//定义好需要获取的时间字符串格式
enum class TimeType
{
    TIME_TYPE_ONE, // 2021-01-01 12:00:00
    TIME_TYPE_TWO, // 2021-01-01 12:00
    TIME_TYPE_THREE, // 2021-01-01
    TIME_TYPE_FOUR,  // 2021-01
    TIME_TYPE_FIVE,  // 2021/01/01 12:00:00
    TIME_TYPE_SIX,   // 2021/01/01 12:00
    TIME_TYPE_SEVEN, // 2021/01/01
    TIME_TYPE_EIGHT, // 2021/01
    TIME_TYPE_NINE,  // 2021年01月01日 12:00:00
    TIME_TYPE_TEN,    // 2021年01月01日 12:00
    TIME_TYPE_ELEVEN, // 2021年01月01日
    TIME_TYPE_TWELVE, // 2021年01月
    TIME_TYPE_THIRTEEN, // 2021年01月01日 12时00分00秒
    TIME_TYPE_FOURTEEN, // 2021年01月01日 12时00分
    TIME_TYPE_FIFTEEN,  // 12:00:00
    TIME_TYPE_SIXTEEN,  // 12:00
};

/**
 * @brief 将time_t类型的时间转换为字符串，有c/c++两种风格
 *          具体可转换格式见TimeType枚举
 * @param time 传入的time_t时间
 * @param dest 存放转换后的字符串
 * @param type 外部指定的类型，默认为TimeType::TIME_TYPE_ONE
 * @param timeval 时间偏移的秒数，默认为0
 * @return std::string& 
 */
std::string& timeToStr(const time_t& time,std::string& dest,TimeType type = TimeType::TIME_TYPE_ONE);
std::string timeToStr(const time_t &time, TimeType type = TimeType::TIME_TYPE_ONE);
char *timeToStr(const time_t &time, char *dest, TimeType type = TimeType::TIME_TYPE_ONE);

//将字符串转为time_t类型的时间，需要传入的时间格式为标准格式，需要年月日时分秒全齐，否则函数不工作，返回负一
time_t strToTime(const std::string &str, TimeType type = TimeType::TIME_TYPE_ONE);

/**
 * @brief 获取当前系统时间，有c/c++两种风格
 *
 * @param dest 存放获取到的时间字符串
 * @param type 外部指定的类型，默认为TimeType::TIME_TYPE_ONE
 * @param timeval 时间偏移的秒数，默认为0
 * @return std::string& 
 */
std::string &getCurTime(std::string &dest, TimeType type = TimeType::TIME_TYPE_ONE,const int timeval = 0);
std::string getCurTime(TimeType type = TimeType::TIME_TYPE_ONE,const int timeval = 0);
char *getCurTime(char *dest, TimeType type = TimeType::TIME_TYPE_ONE, const int timeval = 0);


/**
 * @brief 为传入的字符串添加以秒为级别的时间偏移量
 *      只允许时间格式完全的字符串进行操作，否则返回false，或者返回传入字符串
        重载四个版本，两个直接在传入的字符串上改动，两个需要传入存放结果的字符串
 * @param src 传入的字符串
 * @param dest 存放获取到的时间字符串
 * @param type 时间格式
 * @param timeval 时间偏移量
 * @return std::string& 
 */
std::string &addTime(std::string &src, TimeType type = TimeType::TIME_TYPE_ONE, const int timeval = 0);
char *addTime(char *src, TimeType type = TimeType::TIME_TYPE_ONE, const int timeval = 0);
bool addTime(const std::string &src, std::string &dest, TimeType type = TimeType::TIME_TYPE_ONE, const int timeval = 0);
bool addTime(const std::string &src, char *dest, TimeType type = TimeType::TIME_TYPE_ONE, const int timeval = 0);


////////////////////////////   /////////////////////////////
////////////////////////////   /////////////////////////////

//计时器类，实现一个微秒级的计时器
class timeCount
{
  private:
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point end;
  public:
    timeCount(){};
    timeCount(const timeCount &tc) = delete;
    timeCount &operator=(const timeCount &tc) = delete;
    void startCount();//开始计时
    double endCount();//结束计时并返回结果
};