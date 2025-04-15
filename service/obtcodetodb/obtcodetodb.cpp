#include "obtcodetodb.h"
#include "include/Iconnection.h"
#include "include/msql.h"
#include "stringop/include/jsonns.h"
#include "timeframe/include/timeframe.h"
#include <memory>
using namespace std;

vector<st_stcode> stcodelist; // 存放站点参数的容器

// 创建日志对象的全局变量
logfile lg;
// 进程心跳
procHeart ph;

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cout << "usage: " << argv[0] << " <inifile> <dbsetting> <logfile>" << endl;
        cout << "example:/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 60 " << argv[0]
             << " /temp/data/数据集合/01中国气象站点数据/中国气象站点参数.ini "
                "/home/akijyo/桌面/code/c++/opendata/database/dbsettings.json /temp/log/obtcodetodb.log"
             << endl;

        cout << "inifile: 气象站点参数文件名" << endl;
        cout << "dbsetting:数据库连接设置的json文件，要求有host,user,passwd,port,db" << endl;
        cout << "logfile:日志文件名" << endl;
        cout << "---------------------------------------------------" << endl;
        cout << "本程序用于解析气象站点参数文件，将站点参数写入数据库中" << endl;
        cout << "若没有改站参数信息，则数据库新增内容，若改站参数信息被修改，则数据库更新内容" << endl;
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

    // 1.从站点参数文件中加载站点参数，存放在容器中
    if (!loadstcode(argv[1]))
    {
        return 0;
    }
    // 2.连接数据库
    shared_ptr<IConnection> connection = make_shared<mysql>();
    if (!connectdb(argv[2], connection))
    {
        return 0;
    }
    // 3.更新数据库
    updatedb(connection);
    return 0;
}

// 把站点文件参数加载到容器中
bool loadstcode(const string &inifile)
{
    // 创建文件读取对象
    rdfile rf;
    if (rf.open(inifile) == false)
    {
        lg.writeLine("打开站点参数文件失败：%s", inifile.c_str());
        return false;
    }
    // 读掉标题类
    string temp;
    // rf.readLine(temp);

    ccmdstr spl;
    struct st_stcode stcode;

    while (rf.readLine(temp))
    {
        // lg.writeLine("stcode:%s\n", temp.c_str());
        // cout<<temp<<endl;
        spl.split(temp, ",", true);

        memset(&stcode, 0, sizeof(stcode));

        spl.getValue(0, stcode.provname);
        spl.getValue(1, stcode.obtid);
        spl.getValue(2, stcode.obtname);
        spl.getValue(3, stcode.lat);
        spl.getValue(4, stcode.lon);
        spl.getValue(5, stcode.alt);
        stcodelist.push_back(stcode);
    }
    ph.updateHeart();
    return true;
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

bool updatedb(std::shared_ptr<IConnection> connection)
{
    int success = 0;
    timeCount tc;
    tc.startCount();
    connection->start_transaction();
    for (auto &it : stcodelist)
    {
        string sql = "INSERT INTO T_ZHOBTCODE (obtid,cityname,provname,lat,lon,height,upttime) VALUES ('" + it.obtid +
                     "','" + it.obtname + "','" + it.provname + "'," + to_string((int)(it.lat * 100)) + "," +
                     to_string((int)(it.lon * 100)) + "," + to_string((int)(it.alt * 10)) + ", NOW()) " +
                     "ON DUPLICATE KEY UPDATE " + "provname='" + it.provname + "'," + "cityname='" + it.obtname + "'," +
                     "lat=" + to_string((int)(it.lat * 100)) + "," + "lon=" + to_string((int)(it.lon * 100)) + "," +
                     "height=" + to_string((int)(it.alt * 10)) + "," + "upttime=NOW()";
        if (!connection->update(sql))
        {
            lg.writeLine("更新数据库失败，sql=%s，err：%s", sql.c_str(), connection->last_error_);
            continue;
        }
        success++;
    }
    connection->commit();
    double time = tc.endCount();
    lg.writeLine("总记录数：%d，成功：%d，耗时：%.2f", stcodelist.size(), success, time);
    ph.updateHeart();
    return true;
}

void EXIT(int sig)
{
    lg.writeLine("程序被信号中断，sig=%d", sig);
    exit(0);
}