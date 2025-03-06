#include "timeop.h"
#include <cstring>
#include <ctime>
using namespace std;
using namespace chrono;

string &timeToStr(const time_t &time, string &dest, TimeType type)
{
    tm ts;
    localtime_r(&time, &ts);
    ts.tm_year += 1900;
    ts.tm_mon += 1;
    switch (type)
    {
    case TimeType::TIME_TYPE_ONE:
        sFomat(dest, "%04d-%02d-%02d %02d:%02d:%02d", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
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
        sFomat(dest, "%04d/%02d/%02d %02d:%02d:%02d", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
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
        sFomat(dest, "%04d年%02d月%02d日 %02d:%02d:%02d", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
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
        sFomat(dest, "%04d年%02d月%02d日 %02d时%02d分%02d秒", ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
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

string timeToStr(const time_t &time, TimeType type)
{
    string dest;
    timeToStr(time, dest, type);
    return dest;
}

char *timeToStr(const time_t &time, char *dest, TimeType type)
{
    if(dest==nullptr)
    {
        return dest;
    }
    string destTemp;
    timeToStr(time, destTemp, type);
    strcpy(dest, destTemp.c_str());
    return dest;
}

time_t strToTime(string &str, TimeType type)
{
    if(str.empty())
    {
        return 0;
    }
    string temp;
    tm ts;
    switch (type)
    {
    case TimeType::TIME_TYPE_ONE:
    case TimeType::TIME_TYPE_FIVE:
    case TimeType::TIME_TYPE_NINE:
    case TimeType::TIME_TYPE_THIRTEEN: {
        temp = pickNum(str);
        if(temp.size()!=14)
        {
            return 0;
        }
        try
        {
            ts.tm_year = stoi(temp.substr(0, 4)) - 1900;
            ts.tm_mon = stoi(temp.substr(4, 2)) - 1;
            ts.tm_mday = stoi(temp.substr(6, 2));
            ts.tm_hour = stoi(temp.substr(8, 2));
            ts.tm_min = stoi(temp.substr(10, 2));
            ts.tm_sec = stoi(temp.substr(12, 2));
        }
        catch (const std::exception &e)
        {
            break;
        }
    }
    default:
        break;
    }
    return mktime(&ts);
}