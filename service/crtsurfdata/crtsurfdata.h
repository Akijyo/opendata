#pragma once
#include "../../modhead.h"


// 定义站点参数的结构体
struct st_stcode
{
    std::string provname; // 省份名称
    std::string obtid;    // 站点编号
    std::string obtname;  // 站点名称
    double lat;           // 纬度：单位度
    double lon;           // 经度：单位度
    double alt;           // 海拔：单位米
};
// 定义站点观测数据的结构体
struct st_surfdata
{
    std::string obtid;    // 站点编号
    std::string datetime; // 日期时间，格式为yyyy-mm-dd hh:mi:ss，秒位为0
    int temperature;      // 温度，单位0.1摄氏度
    int pressure;         // 气压，单位0.1百帕
    int humidity;         // 湿度，单位百分比
    int winddir;          // 风向，单位度0-360
    int windspeed;        // 风速，单位0.1米/秒
    int rain;             // 降雨量，单位0.1毫米
    int vis;              // 能见度，单位0.1米
};

// 加载站点参数
bool loadstcode(const std::string &inifile);

// 生成站点观测数据
void crtsurfdata();

// 将生成的站点观测数据写入文件
void crtsurffile(const std::string &outpath);
