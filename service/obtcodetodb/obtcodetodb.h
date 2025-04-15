#pragma once
#include "../../modhead.h"
#include "include/Iconnection.h"
#include <memory>


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

void EXIT(int sig);

// 加载站点参数
bool loadstcode(const std::string &inifile);

// 连接数据库
bool connectdb(const std::string &jsonfile, std::shared_ptr<IConnection> connection);

// 更新数据库
bool updatedb(std::shared_ptr<IConnection> connection);