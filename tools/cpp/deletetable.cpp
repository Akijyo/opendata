#include "cpublic.h"
#include "fileframe/include/cdir.h"
#include "fileframe/include/fileframe.h"
#include "fileframe/include/fileio.h"
#include "fileframe/include/logfile.h"
#include "include/Iconnection.h"
#include "include/msql.h"
#include "procheart/include/procheart.h"
#include "stringop/include/jsonns.h"
#include "stringop/include/split.h"
#include "stringop/include/stringop.h"
#include "timeframe/include/timeframe.h"
#include <memory>
#include <string>
using namespace std;

///////////////////////////////////////////////////////////////////
//////////本程序是按照条件清理表中数据的通用模块//////////////////////
///////////////////////////////////////////////////////////////////

// 程序参数的结构体
class agrss
{
  public:
    string host;    // 数据库地址
    int port;       // 数据库端口号
    string user;    // 数据库用户名
    string passwd;  // 数据库密码
    string db;      // 数据库名
    string charset; // 数据库字符集

    string tablename; // 待清理的表名
    string keyfield;  // 待清理数据表的唯一字段名，如编号keyid
    string where;     // 待清理数据表的条件，如keyid>1000
    int maxcount;     // 待清理数据表的最大记录数，如1000
    string starttime; // 程序执行踏入的时间点，如02,13,当时到达时程序才运行

    int phtimeout; // 进程心跳时间，单位秒，默认30秒
};
// 定义出程序参数的全局变量
agrss starg;

// 创建日志文件对象
logfile lg;
// 初始化进程心跳
procHeart ph;

// 程序信号中断的处理函数
void EXIT(int sig);
// 程序帮助文档
void help();
// 解析程序参数
bool parseMainParam(const string &jsonfile);
// 判断程序是否在可运行的时间区间
bool isRunTime(const string &starttime);
// 执行主函数
void deleteTable();

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        help();
        return 0;
    }
    // 关闭信号影响
    closeiosignal(false);
    // 对2/15信号处理，遇到这两个信号程序退出
    // 2信号是Ctrl+C，15信号是kill命令
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    // 打开日志文件
    if (!lg.open(argv[1], ios::out | ios::app))
    {
        cout << "打开日志文件失败" << endl;
        return 0;
    }
    // 初始化进程心跳
    ph.addProcInfo(getpid(), argv[0], starg.phtimeout);
    // 解析程序参数
    if (!parseMainParam(argv[2]))
    {
        lg.writeLine("初始解析程序参数失败");
        return 0;
    }
    //判断是否在程序运行时间段
    if (!isRunTime(starg.starttime))
    {
        return 0;
    }
    // 迁移业务的主程序
    deleteTable();
    return 0;
}

// 判断程序是否在可运行的时间区间
bool isRunTime(const string &starttime)
{
    // 如果starttime为空，则表示不限制时间
    if (starttime.empty())
        return true;
    // 获取当前时间的小时
    string timeNow;
    getCurTime(timeNow, TimeType::TIME_TYPE_SIXTEEN);
    timeNow = timeNow.substr(0, 2);
    // 如果当前时间的小时时间段不在在starttime中，则返回true
    if (starttime.find(timeNow) == string::npos)
        return false;
    return true;
}

void deleteTable()
{
    // 1.链接数据库
    shared_ptr<IConnection> connection = make_shared<mysql>();
    if (!connection->connect(starg.host, starg.user, starg.passwd, starg.db, starg.port))
    {
        lg.writeLine("连接数据库失败，host=%s,user=%s,passwd=%s,db=%s,port=%d", starg.host.c_str(), starg.user.c_str(),
                     starg.passwd.c_str(), starg.db.c_str(), starg.port);
        return;
    }
    connection->set_charset(starg.charset);

    // 创建时间计时器
    timeCount tc;
    tc.startCount();

    // 2.获取带删除数据字段值的集合
    string selectSql = "select " + starg.keyfield + " from " + starg.tablename + " where " + starg.where;
    // 执行select
    if (!connection->query(selectSql))
    {
        lg.writeLine("执行查询sql语句失败，sql语句：%s，报错代码:%d", selectSql.c_str(), connection->last_errno_);
        return;
    }

    // 3.进行删除操作
    // 初始化原始sql
    string deleteSqlbase = "delete from " + starg.tablename + " where " + starg.keyfield + " in (";
    string deleteSql = deleteSqlbase;
    // 读取数据
    int loopCount = 0;
    int totolCount = 0;
    while (connection->next())
    {
        loopCount++;
        totolCount++;
        // 获取keyfield字段的值
        optional<string> keyfieldValue = connection->value(starg.keyfield);
        // 拼接值
        deleteSql += "'" + keyfieldValue.value() + "',";
        // 当循环次数到maxcount时，执行删除操作
        if (loopCount >= starg.maxcount)
        {
            loopCount = 0;
            // 拼接完整的删除sql
            deleteSql.back() = ')'; // 去掉最后一个逗号
                                    // 执行删除操作
            if (!connection->update(deleteSql))
            {
                lg.writeLine("执行删除sql语句失败，sql语句：%s，报错代码:%d", deleteSql.c_str(),
                             connection->last_errno_);
                return;
            }
            deleteSql = deleteSqlbase; // 重置sql
        }
        ph.updateHeart();
    }
    // 删除剩下的数据
    if (loopCount > 0)
    {
        deleteSql.back() = ')'; // 去掉最后一个逗号
        if (!connection->update(deleteSql))
        {
            lg.writeLine("执行删除sql语句失败，sql语句：%s，报错代码:%d", deleteSql.c_str(), connection->last_errno_);
            return;
        }
        ph.updateHeart();
    }
    double runTime = tc.endCount();
    lg.writeLine("本次程序运行耗时：%.2f，共计删除记录%d条", runTime, totolCount);
}

// 解析程序参数
bool parseMainParam(const string &jsonfile)
{
    // 读取json文件
    jsonns js(jsonfile);
    if (!js.get("host", starg.host))
    {
        lg.writeLine("缺少host参数");
        return false;
    }
    if (!js.get("port", starg.port))
    {
        starg.port = 3306; // 默认端口号
    }
    if (!js.get("user", starg.user))
    {
        lg.writeLine("缺少user参数");
        return false;
    }
    if (!js.get("passwd", starg.passwd))
    {
        lg.writeLine("缺少passwd参数");
        return false;
    }
    if (!js.get("db", starg.db))
    {
        lg.writeLine("缺少db参数");
        return false;
    }
    if (!js.get("charset", starg.charset))
    {
        lg.writeLine("缺少charset参数");
        return false;
    }
    if (!js.get("tablename", starg.tablename))
    {
        lg.writeLine("缺少tablename参数");
        return false;
    }
    if (!js.get("keyfield", starg.keyfield))
    {
        lg.writeLine("缺少keyfield参数");
        return false;
    }
    if (!js.get("where", starg.where))
    {
        starg.where = "1=1"; // 默认条件
    }
    if (!js.get("maxcount", starg.maxcount))
    {
        lg.writeLine("缺少maxcount参数");
        return false;
    }
    if (!js.get("starttime", starg.starttime))
    {
        starg.starttime = ""; // 默认时间
    }
    if (!js.get("phtimeout", starg.phtimeout))
    {
        starg.phtimeout = 30; // 默认心跳时间
    }
    return true;
}

void help()
{
    cout << "本程序是数据共享平台的通用模块，用于数据管理，把数据库表中没有意义的数据删除" << endl;
    cout << "Usage: deletetable <logfile> <jsonfile>" << endl;
    cout << "logfile：用于保存数据入库日志的文件" << endl;
    cout << "jsonfile：用于保存程序传入参数的json文件" << endl;
    cout << "Example: ./deletetable /temp/log/deletetable.log "
            "/home/akijyo/桌面/code/c++/opendata/tools/others/deletetable.json"
         << endl
         << endl;
    cout << "json文件应有的字段：" << endl;
    cout << "host: 数据库地址。" << endl;
    cout << "user: 数据库用户。" << endl;
    cout << "passwd: 数据库密码。" << endl;
    cout << "db: 数据库名。" << endl;
    cout << "port: 数据库端口号，缺省3306。" << endl;
    cout << "charset: 数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现乱码。" << endl;
    cout << "tablename: 待清理数据库表名，不得为空。" << endl;
    cout << "keyfield: 待清理数据库的唯一字段名，数据库的删除以此字段参考条件" << endl;
    cout << "where: 需要清理的数据被覆盖的条件" << endl;
    cout << "maxcount: 最大删除数，如1000就每次执行一个sql就删除1000条数据" << endl;
    cout << "starttime: 程序执行的时间段，如02,13就表明程序在02时13时才可以运行" << endl;
    cout << "phtimeout: 进程心跳时间，单位秒，默认30秒" << endl;
}

void EXIT(int sig)
{
    lg.writeLine("程序被信号中断，sig=%d", sig);
    exit(0);
}