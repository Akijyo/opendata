/*********************************************************************************
本程序载入气象站点观测数据文件，并且将文件内容存入到数据库中
由于数据量太大，数据只插入一次，有重复则不插入
外置脚本会删除数据库中数据生成日期在系统时间2小时之前的数据
*********************************************************************************/
#include "obtmindtodb.h"
#include "fileframe/include/cdir.h"
#include "fileframe/include/fileframe.h"
#include "timeframe/include/timeframe.h"
#include <fstream>
#include <nlohmann/json.hpp>
using namespace std;

// 创建日志对象的全局变量
logfile lg;
// 进程心跳
procHeart ph;

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cout << "usage: " << argv[0] << " <surfdatadir> <dbsetting> <logfile>" << endl;
        cout << "example:/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 60 " << argv[0]
             << " /temp/output/surfdata "
                "/home/akijyo/桌面/code/c++/opendata/database/dbsettings.json /temp/log/obtmindtodb.log"
             << endl;

        cout << "surfdatadir:观测数据生成所在的目录" << endl;
        cout << "dbsetting:数据库连接设置的json文件，要求有host,user,passwd,port,db" << endl;
        cout << "logfile:日志文件名" << endl;
        cout << "---------------------------------------------------" << endl;
        cout << "本程序用于将生成的气象站点观测数据载入数据库" << endl;
        cout << "由于数据量太大，数据只插入一次，有重复则不插入" << endl;
        return 0;
    }
    closeiosignal(false);

    if (lg.open(argv[3]) == false)
    {
        cout << "打开日志文件失败" << endl;
        return -1;
    }

    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    ph.addProcInfo(getpid(), "agrv[0]", 20);

    // 1.连接数据库
    shared_ptr<IConnection> connection = make_shared<mysql>();
    if (!connectdb(argv[2], connection))
    {
        return 0;
    }
    ph.updateHeart();
    // 2.更新数据库
    func(connection, argv[1]);
    ph.updateHeart();
    
    return 0;
}

bool connectdb(const std::string &jsonfile, std::shared_ptr<IConnection> connection)
{
    jsonns js(jsonfile);
    string host, user, passwd, db;
    int port;
    js.get("host", host);
    js.get("user", user);
    js.get("passwd", passwd);
    js.get("port", port);
    js.get("db", db);
    if (!connection->connect(host, user, passwd, db, port))
    {
        lg.writeLine("连接数据库失败");
        return false;
    }
    ph.updateHeart();
    return true;
}

void func(std::shared_ptr<IConnection> connection, const std::string &filedir)
{
    cdir dir;
    dir.openDir(filedir, ".*\\.json\\b");
    while (dir.readFile())
    {
        timeCount tc;
        tc.startCount();
        string filename = dir.fullpath;
        struct st_surfdata surfdata; // 创建站点观测数据结构体
        ifstream ifs(filename, ios::in);
        nlohmann::json js = nlohmann::json::parse(ifs);
        ifs.close();
        connection->start_transaction();
        for (auto &subarr : js)
        {
            surfdata.obtid = subarr["obtid"];
            surfdata.datetime = subarr["datetime"];
            auto temperature = subarr["temperature"];
            surfdata.temperature = (int)(temperature.get<double>() * 10);
            auto pressure = subarr["pressure"];
            surfdata.pressure = (int)(pressure.get<double>() * 10);
            surfdata.humidity = subarr["humidity"];
            surfdata.winddir = subarr["winddir"];
            auto windspeed = subarr["windspeed"];
            surfdata.windspeed = (int)(windspeed.get<double>() * 10);
            auto rain = subarr["rain"];
            surfdata.rain = (int)(rain.get<double>() * 10);
            auto vis = subarr["vis"];
            surfdata.vis = (int)(vis.get<double>() * 10);
            string sql =
                "insert into T_ZHOBTMIND(obtid,ddatetime,temperature,pressure,humidity,winddir,windspeed,rain,vis) "
                "values('" +
                surfdata.obtid + "','" + surfdata.datetime + "'," + to_string(surfdata.temperature) + "," +
                to_string(surfdata.pressure) + "," + to_string(surfdata.humidity) + "," + to_string(surfdata.winddir) +
                "," + to_string(surfdata.windspeed) + "," + to_string(surfdata.rain) + "," + to_string(surfdata.vis) +
                ")";
            connection->update(sql);
        }
        connection->commit();
        ph.updateHeart();
        double time = tc.endCount();
        lg.writeLine("处理文件：%s，记录数：%d，耗时：%.2f", filename.c_str(), js.size(), time);
        // 处理完则删除文件
        deletefile(filename);
    }
}

void EXIT(int sig)
{
    lg.writeLine("程序被信号中断，sig=%d", sig);
    exit(0);
}