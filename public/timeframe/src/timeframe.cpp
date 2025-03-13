#include "../include/timeframe.h"
using namespace std;
using namespace chrono;

// time_t转字符串
string &timeToStr(const time_t &time, string &dest, TimeType type)
{
    if (time <= 0)//传入了一个错误的时间
    {
        return dest;
    }

    tm ts; // 声明出一个tm结构体

    // 将time_t类型的时间转换为tm结构体
    // localtime_r是线程安全函数，如果用struct tm *ts=localtime(&time);会有线程安全问题
    localtime_r(&time, &ts);

    // tm结构体中的年份是从1900年开始计数的，月份是从0开始计数
    ts.tm_year += 1900;
    ts.tm_mon += 1;

    // 根据不同的类型格式化时间字符串，调用共用字符串操作中的sFomat函数
    switch (type)
    {
    case TimeType::TIME_TYPE_ONE:
        sFomat(dest, "%04d-%02d-%02d %02d:%02d:%02d", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min,
               ts.tm_sec);
        break;
    case TimeType::TIME_TYPE_TWO:
        sFomat(dest, "%04d-%02d-%02d %02d:%02d", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min);
        break;
    case TimeType::TIME_TYPE_THREE:
        sFomat(dest, "%04d-%02d-%02d", ts.tm_year, ts.tm_mon, ts.tm_mday);
        break;
    case TimeType::TIME_TYPE_FOUR:
        sFomat(dest, "%04d-%02d", ts.tm_year, ts.tm_mon);
        break;
    case TimeType::TIME_TYPE_FIVE:
        sFomat(dest, "%04d/%02d/%02d %02d:%02d:%02d", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min,
               ts.tm_sec);
        break;
    case TimeType::TIME_TYPE_SIX:
        sFomat(dest, "%04d/%02d/%02d %02d:%02d", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min);
        break;
    case TimeType::TIME_TYPE_SEVEN:
        sFomat(dest, "%04d/%02d/%02d", ts.tm_year, ts.tm_mon, ts.tm_mday);
        break;
    case TimeType::TIME_TYPE_EIGHT:
        sFomat(dest, "%04d/%02d", ts.tm_year, ts.tm_mon);
        break;
    case TimeType::TIME_TYPE_NINE:
        sFomat(dest, "%04d年%02d月%02d日 %02d:%02d:%02d", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min,
               ts.tm_sec);
        break;
    case TimeType::TIME_TYPE_TEN:
        sFomat(dest, "%04d年%02d月%02d日 %02d:%02d", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min);
        break;
    case TimeType::TIME_TYPE_ELEVEN:
        sFomat(dest, "%04d年%02d月%02d日", ts.tm_year, ts.tm_mon, ts.tm_mday);
        break;
    case TimeType::TIME_TYPE_TWELVE:
        sFomat(dest, "%04d年%02d月", ts.tm_year, ts.tm_mon);
        break;
    case TimeType::TIME_TYPE_THIRTEEN:
        sFomat(dest, "%04d年%02d月%02d日 %02d时%02d分%02d秒", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min,
               ts.tm_sec);
        break;
    case TimeType::TIME_TYPE_FOURTEEN:
        sFomat(dest, "%04d年%02d月%02d日 %02d时%02d分", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min);
        break;
    case TimeType::TIME_TYPE_FIFTEEN:
        sFomat(dest, "%02d:%02d:%02d", ts.tm_hour, ts.tm_min, ts.tm_sec);
        break;
    case TimeType::TIME_TYPE_SIXTEEN:
        sFomat(dest, "%02d:%02d", ts.tm_hour, ts.tm_min);
        break;
    default:
        break;
    }
    return dest;
}
// 重载timeToStr函数的无引用版本
string timeToStr(const time_t &time, TimeType type)
{
    string dest;
    timeToStr(time, dest, type);
    return dest;
}
// 重载timeToStr函数的c风格字符串版本
char *timeToStr(const time_t &time, char *dest, TimeType type)
{
    if (dest == nullptr)
    {
        return dest;
    }
    string destTemp;
    timeToStr(time, destTemp, type);
    strcpy(dest, destTemp.c_str());//将string类型转为c风格字符串
    return dest;
}

// 将字符串转为time_t类型的时间
time_t strToTime(const string &str, TimeType type)
{
    if (str.empty())
    {
        return -1;
    }

    string temp; // 临时存放字符串，存放在str中提取出的数字
    temp = pickNum(str);//调用字符串操作中的pickNum函数提取字符串中的数字
    if (temp.size() != 14)//提取出不是14位的数字，初步判断说明时间格式不对
    {
        return -1;
    }

    tm ts = {}; // tm结构体，用于保存时间数值
    // 这里一定要初始化，就是tm ts = {};因为tm结构体成员不止年月日时分秒，
    //但是下面赋值只赋值了这六个字段，就会导致最后的mktime函数异常，导致运行结果呈现叠加态
    
    switch (type)
    {
        // 只有这四种格式运行转换,年月日时分秒全齐
    case TimeType::TIME_TYPE_ONE:
    case TimeType::TIME_TYPE_FIVE:
    case TimeType::TIME_TYPE_NINE:
    case TimeType::TIME_TYPE_THIRTEEN: {
        try//stoi函数要用try处理防止异常导致程序退出
        {
            ts.tm_year = stoi(temp.substr(0, 4)) - 1900;// tm结构体中的年份是从1900年开始计数的
            ts.tm_mon = stoi(temp.substr(4, 2)) - 1;// 月份是从0开始计数
            ts.tm_mday = stoi(temp.substr(6, 2));
            ts.tm_hour = stoi(temp.substr(8, 2));
            ts.tm_min = stoi(temp.substr(10, 2));
            ts.tm_sec = stoi(temp.substr(12, 2));
        }
        catch (const std::exception &e)
        {
            return -1;
        }
        break;
    }
    default:
        return -1;
    }
    return mktime(&ts); // 将tm结构体转为time_t类型的时间
}

// 获取当前系统时间
string &getCurTime(string &dest, TimeType type, const int timeval)
{
    if (abs(timeval) > 1000000000)//时间偏移量过大，直接返回
    {
        return dest;
    }
    system_clock::time_point now = system_clock::now();//获取当前系统时间
    time_t time = system_clock::to_time_t(now);//将系统时间转为time_t类型
    time += timeval;//加上时间偏移量
    dest = timeToStr(time, type);//调用上面的time_t类型转字符串函数
    return dest;
}
string getCurTime(TimeType type, const int timeval)
{
    string dest;
    getCurTime(dest, type, timeval);
    return dest;
}
char *getCurTime(char *dest, TimeType type, const int timeval) // 获取当前系统时间的c风格字符串版本
{
    if (dest == nullptr)
    {
        return dest;
    }
    string destTemp;
    getCurTime(destTemp, type, timeval);
    strcpy(dest, destTemp.c_str());//将临时的string类型转为c风格字符串
    return dest;
}

// 为传入的字符串添加以秒为级别的时间偏移量
string &addTime(string &src, TimeType type, const int timeval)
{
    if(src.empty())
    {
        return src;
    }
    time_t time = strToTime(src, type);//将字符串转为time_t类型
    if(time==-1)
    {
        return src;
    }
    time += timeval;//加上时间偏移量
    src = timeToStr(time, type);//将time_t类型转为字符串
    return src;
}
char *addTime(char *src, TimeType type, const int timeval)
{
    if (src == nullptr)
    {
        return src;
    }
    string srcTemp(src);
    addTime(srcTemp, type, timeval);
    strcpy(src, srcTemp.c_str());//将临时的string类型转为c风格字符串
    return src;
}
//重载addTime函数的字符串版本，返回bool值，需要传入存放结果的字符串
bool addTime(const string &src, string &dest, TimeType type, const int timeval)
{
    if (src.empty())
    {
        return false;
    }
    time_t time=strToTime(src,type);
    if(time==-1)
    {
        return false;
    }
    time += timeval;
    //这里不需要清空dest，因为timeToStr函数中的sFomat函数会自动清空dest
    dest = timeToStr(time, type);
    return true;
}
bool addTime(const string &src, char *dest, TimeType type, const int timeval)
{
    if (dest == nullptr)
    {
        return false;
    }
    string destTemp;
    if (!addTime(src, destTemp, type, timeval))
    {
        return false;
    }
    strcpy(dest, destTemp.c_str());
    return true;
}

////////////////////////////   /////////////////////////////
////////////////////////////   /////////////////////////////

// 计时器类，实现一个微秒级的计时器
void timeCount::startCount()
{
    start = steady_clock::now(); // 获取当前时间
}
double timeCount::endCount()
{
    end = steady_clock::now();//获取当前时间
    auto duration = duration_cast<microseconds>(end - start);//计算时间差
    return double(duration.count()) * microseconds::period::num / microseconds::period::den;
    //相当于time* 1/1000000
}