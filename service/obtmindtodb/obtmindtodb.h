#pragma once
#include "../../modhead.h"

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

void EXIT(int sig);

// 连接数据库
bool connectdb(const std::string &jsonfile, std::shared_ptr<IConnection> connection);

void func(std::shared_ptr<IConnection> connection,const std::string &filedir);