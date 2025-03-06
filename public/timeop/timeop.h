#include "../cpublic.h" //c/c++����ͷ�ļ������������ڴ��ļ������
#include "../stringop/stringop.h"

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
    TIME_TYPE_NINE,  // 2021��01��01�� 12:00:00
    TIME_TYPE_TEN,    // 2021��01��01�� 12:00
    TIME_TYPE_ELEVEN, // 2021��01��01��
    TIME_TYPE_TWELVE, // 2021��01��
    TIME_TYPE_THIRTEEN, // 2021��01��01�� 12ʱ00��00��
    TIME_TYPE_FOURTEEN, // 2021��01��01�� 12ʱ00��
    TIME_TYPE_FIFTEEN,  // 12:00:00
    TIME_TYPE_SIXTEEN,  // 12:00
};

std::string& timeToStr(const time_t& time,std::string& dest,TimeType type = TimeType::TIME_TYPE_ONE);
std::string timeToStr(const time_t &time, TimeType type = TimeType::TIME_TYPE_ONE);
char *timeToStr(const time_t &time, char *dest, TimeType type = TimeType::TIME_TYPE_ONE);
time_t strToTime(const std::string &str, TimeType type = TimeType::TIME_TYPE_ONE);

std::string &getCurTime(std::string &dest, TimeType type = TimeType::TIME_TYPE_ONE);
std::string getCurTime(TimeType type = TimeType::TIME_TYPE_ONE);
char *getCurTime(char *dest, TimeType type = TimeType::TIME_TYPE_ONE);