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
#include <csignal>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <vector>
using namespace std;

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

    string inifilename; // 入库参数设置文件名
    string datapath;    // 待入库数据文件路径
    string datapathbak; // 入库完成后数据文件的备份路径
    string datapatherr; // 入库出错数据文件的存放路径

    int timeval;   // 扫描datapath目录的时间间隔
    int phtimeout; // 进程心跳时间，单位秒，默认30秒
};
// 定义出程序参数的全局变量
agrss starg;

// 数据入库参数的结构体
class datatotable
{
  public:
    string matchfile;    // json数据文件匹配的规则，需要入库哪些指定文件，格式为正则表达式
    string tablename;    // 数据入库的表名
    bool update = false; // 是否更新数据，1表示不更新，2表示更新
    string sql;          // 数据入库前执行的sql语句
};
// 定义数据入库参数的全局变量数组
vector<datatotable> stargtable;

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
// 解析数据入库参数
bool parseDataToTable(const string &jsonfile);
// 数据入库处理的主程序
void dataToTable();

///////////入库程序主函数中的子函数//////////
bool getCurTable(const string &filename, datatotable &curTable);       // 3.2子函数：获取当前文件的入库参数
bool getFieldName(const string &fullpath, vector<string> &fieldNames); // 3.3子函数：获取数据文件中该有的字段名
bool getSQL(string &res, nlohmann::json &subobj, vector<string> &fieldNames,
            datatotable curTable); // 3.4子函数：拼接插入/更新的sql语句
///////////入库程序主函数中的子函数//////////

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
    // 解析程序参数
    if (!parseMainParam(argv[2]))
    {
        lg.writeLine("初始解析程序参数失败");
        return 0;
    }
    // 初始化进程心跳
    ph.addProcInfo(getpid(), argv[0], starg.phtimeout);
    // 执行主函数
    dataToTable();

    return 0;
}

void dataToTable()
{

    // 1.连接数据库
    shared_ptr<IConnection> connection = make_shared<mysql>();
    if (!connection->connect(starg.host, starg.user, starg.passwd, starg.db, starg.port))
    {
        lg.writeLine("连接数据库失败，错误代码：%d", connection->last_errno_);
        return;
    }
    connection->set_charset(starg.charset);

    int parseCount = 50;
    // 解析数据入库参数文件的次数，这个参数定义出来的目的是把parseDataToTable()函数放入循环，意义是如果参数文件被修改，后台程序可以重新加载参数文件

    cdir dir;
    // 3.扫描datapath目录下的文件
    while (true)
    {
        if (parseCount >= 30) // 循环30次重新加载一次入库参数文件
        {
            parseCount = 0;
            // 2.将数据入库参数文件内容加载到数组中
            if (!parseDataToTable(starg.inifilename))
            {
                lg.writeLine("解析数据入库参数文件%s失败", starg.inifilename.c_str());
                return;
            }
        }
        parseCount++;

        if (!dir.openDir(starg.datapath, ".*\\.json\\b", 10000, false, true))
        {
            lg.writeLine("打开数据目录%s失败", starg.datapath.c_str());
            return;
        }

        // 3.1获取指定的数据json格式文件
        while (dir.readFile())
        {
            // 程序日志记录-运行时间
            timeCount tc;
            tc.startCount();
            // 3.2通过指定文件匹配数组中的数据入库参数，找到相关表名。
            datatotable curTable;
            if (!getCurTable(dir.filename, curTable))
            {
                lg.writeLine("没有找到匹配的表名，文件名：%s，匹配规则：%s", dir.fullpath.c_str(),
                             curTable.matchfile.c_str());
                return;
            }

            // 此时已经获取到文件，准备进入json数组解析的循环

            // 3.3 获取数据文件中该有的字段名
            vector<string> fieldNames;
            if (!getFieldName(dir.fullpath, fieldNames))
            {
                lg.writeLine("获取数据文件中该有的字段名失败，文件名：%s", dir.fullpath.c_str());
                return;
            }

            // 3.4 拼接插入/更新的sql语句
            ifstream dataFile(dir.fullpath);
            if (!dataFile.is_open())
            {
                lg.writeLine("打开数据文件失败，文件名：%s", dir.fullpath.c_str());
                return;
            }
            // 获取json数据
            nlohmann::json root = nlohmann::json::parse(dataFile);
            dataFile.close();
            if (!root.is_array())
            {
                lg.writeLine("数据文件格式错误，文件名：%s", dir.fullpath.c_str());
                return;
            }
            // 开启事务
            connection->start_transaction();
            // 3.5.1执行前置sql语句
            if (!curTable.sql.empty())
            {
                if (!connection->update(curTable.sql))
                {
                    lg.writeLine("执行插入/更新的前置sql语句失败，sql语句：%s，报错代码:%d", curTable.sql.c_str(),
                                 connection->last_errno_);
                    connection->rollback();
                    return;
                }
            }
            // 解析每一组数据，并执行插入
            for (auto &it : root)
            {
                nlohmann::json subobj = it;
                string sql;
                if (!getSQL(sql, subobj, fieldNames, curTable))
                {
                    lg.writeLine("拼接sql语句失败，拼接结果：%s", sql.c_str());
                    connection->rollback();
                    return;
                }
                // 3.5 执行sql语句

                if (!connection->update(sql))
                {
                    lg.writeLine("执行插入/更新sql语句失败，sql语句：%s，报错代码:%d", sql.c_str(),
                                 connection->last_errno_);
                    // 4.如果入库失败，则文件放入错误目录
                    connection->rollback();
                    string errFile = starg.datapatherr + dir.filename;
                    if (!renamefile(dir.fullpath.c_str(), errFile.c_str()) != 0)
                    {
                        lg.writeLine("文件备份至错误目录失败，文件名：%s，期望目的地：%s", dir.fullpath.c_str(),
                                     errFile.c_str());
                        return;
                    }

                    return;
                }
            }
            connection->commit();
            double runTime = tc.endCount();
            lg.writeLine("**处理文件%s成功**,本次处理耗时：%.2f", dir.fullpath.c_str(), runTime);
            // 5.提交事务完成后，文件放入备份目录
            string bakFile = starg.datapathbak + dir.filename;
            if (!renamefile(dir.fullpath.c_str(), bakFile.c_str()) != 0)
            {
                lg.writeLine("文件备份至备份目录失败，文件名：%s，期望目的地：%s", dir.fullpath.c_str(),
                             bakFile.c_str());
                return;
            }
        }
        if (dir.size() == 0)
        {
            sleep(starg.timeval);
        }
        ph.updateHeart();
    }
}
// 3.2子函数：获取当前文件的入库参数
// 一个目录中有很多种数据文件，根据文件名匹配表参数给当前循环使用
bool getCurTable(const string &filename, datatotable &curTable)
{
    for (auto &it : stargtable)
    {
        if (matchstr(filename, it.matchfile))
        {
            curTable = it;
            break;
        }
    }
    // 3.2.1 如果没有找到匹配的表名，则将当前文件备份到错误目录，并且程序退出
    if (curTable.tablename.empty())
    {
        return false;
    }
    return true;
}
// 3.3子函数：获取数据文件中该有的字段名
bool getFieldName(const string &fullpath, vector<string> &fieldNames)
{
    ifstream dataFile(fullpath);
    if (!dataFile.is_open())
    {
        return false;
    }
    nlohmann::json root = nlohmann::json::parse(dataFile);
    dataFile.close();
    if (!root.is_array())
    {
        return false;
    }
    // 3.3.1 获取数据文件中该有的字段名
    nlohmann::json firstObj = root[0]; // 只用取第一个对象即可，json数组的字段排序都一致
    for (auto &it : firstObj.items())
    {
        fieldNames.push_back(it.key());
    }
    return true;
}
// 3.4子函数：拼接插入/更新的sql语句
bool getSQL(string &res, nlohmann::json &subobj, vector<string> &fieldNames, datatotable curTable)
{
    try
    {
        // insert into tablename (
        res = "insert into " + curTable.tablename + " (";
        // insert into tablename (field1,field2,field3,
        for (auto &it : fieldNames)
        {
            res += it + ",";
        }
        // 去掉最后一个逗号
        res = res.substr(0, res.length() - 1);
        // insert into tablename (field1,field2, field3) values (
        res += ") values (";

        // insert into tablename (field1,field2, field3) values ('value1','value2',int1,int2)
        for (auto &it : fieldNames)
        {
            auto value = subobj[it];
            if (value.is_string())
            {
                res += "'" + value.get<string>() + "'";
            }
            else if (value.is_null())
            {
                res += "null";
            }
            else if (value.is_boolean())
            {
                res += value.get<bool>() ? "1" : "0"; // 转换为数据库友好的格式
            }
            else
            {
                res += value.dump();
            }
            res += ",";
        }
        res = res.substr(0, res.length() - 1); // 去掉最后一个逗号
        res += ")";

        // 如果不需要更新，此时已经可以返回
        if (!curTable.update)
        {
            return true;
        }

        // insert into tablename (field1,field2,field3) values ('value1','value2',int1,int2) on duplicate key update
        res += " on duplicate key update ";

        // insert into tablename (field1,field2,field3) values ('value1','value2',int1,int2) on duplicate key update
        // field1='value1',field2='value2',field3=int1,field4=int2
        for (auto &it : fieldNames)
        {
            res += it + "=";
            auto value = subobj[it];
            if (value.is_string())
            {
                res += "'" + value.get<string>() + "'";
            }
            else if (value.is_null())
            {
                res += "null";
            }
            else if (value.is_boolean())
            {
                res += value.get<bool>() ? "1" : "0"; // 转换为数据库友好的格式
            }
            else
            {
                res += value.dump();
            }
            res += ",";
        }
        res = res.substr(0, res.length() - 1); // 去掉最后一个逗号
    }
    catch (const std::exception &e)
    {
        return false;
    }
    return true;
}

// 解析程序参数
bool parseMainParam(const string &jsonfile)
{
    // 读取json文件
    jsonns js(jsonfile);

    // 必需参数
    if (!js.get("host", starg.host))
    {
        lg.writeLine("json文件中没有host字段");
        return false;
    }
    if (!js.get("user", starg.user))
    {
        lg.writeLine("json文件中没有user字段");
        return false;
    }
    if (!js.get("passwd", starg.passwd))
    {
        lg.writeLine("json文件中没有passwd字段");
        return false;
    }
    if (!js.get("db", starg.db))
    {
        lg.writeLine("json文件中没有db字段");
        return false;
    }

    // 可选参数，设置默认值
    if (!js.get("port", starg.port))
        starg.port = 3306;
    if (!js.get("charset", starg.charset))
        starg.charset = "utf8mb4";

    // 必需参数
    if (!js.get("inifilename", starg.inifilename))
    {
        lg.writeLine("json文件中没有inifilename字段");
        return false;
    }
    if (!js.get("datapath", starg.datapath))
    {
        lg.writeLine("json文件中没有datapath字段");
        return false;
    }
    if (starg.datapath.back() != '/')
        starg.datapath += '/';
    if (!js.get("datapathbak", starg.datapathbak))
    {
        lg.writeLine("json文件中没有datapathbak字段");
        return false;
    }
    if (starg.datapathbak.back() != '/')
        starg.datapathbak += '/';
    if (!js.get("datapatherr", starg.datapatherr))
    {
        lg.writeLine("json文件中没有datapatherr字段");
        return false;
    }
    if (starg.datapatherr.back() != '/')
        starg.datapatherr += '/';
    if (!js.get("timeval", starg.timeval))
        starg.timeval = 10;

    // 可选参数，设置默认值
    if (!js.get("phtimeout", starg.phtimeout))
        starg.phtimeout = 30;

    return true;
}

// 解析数据入库参数
bool parseDataToTable(const string &jsonfile)
{
    stargtable.clear();
    // 读取json文件
    ifstream inifile(jsonfile);
    if (!inifile.is_open())
    {
        lg.writeLine("打开数据入库参数inifile文件失败");
        return false;
    }
    nlohmann::json root = nlohmann::json::parse(inifile);
    inifile.close();
    if (!root.is_array())
    {
        lg.writeLine("数据入库参数inifile文件格式错误");
        return false;
    }
    for (auto &subobj : root)
    {
        datatotable subparam;
        subparam.matchfile = subobj.value("matchfile", "");
        if (subparam.matchfile.empty())
        {
            lg.writeLine("数据入库参数inifile文件中没有matchfile字段");
            return false;
        }
        subparam.tablename = subobj.value("tablename", "");
        if (subparam.tablename.empty())
        {
            lg.writeLine("数据入库参数inifile文件中没有tablename字段");
            return false;
        }

        // 可选字段使用安全访问方式
        subparam.update = subobj.value("update", false);
        subparam.sql = subobj.value("sql", "");
        stargtable.push_back(subparam);
    }
    return true;
}

void help()
{
    cout << "本程序是数据共享平台的通用模块，用于把数据入库" << endl;
    cout << "Usage: jsontodb <logfile> <jsonfile>" << endl;
    cout << "logfile：用于保存数据入库日志的文件" << endl;
    cout << "jsonfile：用于保存程序传入参数的json文件" << endl;
    cout << "Example: ./jsontodb /temp/log/jsontodb.log "
            "/home/akijyo/桌面/code/c++/opendata/tools/others/jsontodb.json"
         << endl
         << endl;
    cout << "json文件应有的字段：" << endl;
    cout << "host: 数据库地址。" << endl;
    cout << "user: 数据库用户。" << endl;
    cout << "passwd: 数据库密码。" << endl;
    cout << "db: 数据库名。" << endl;
    cout << "port: 数据库端口号，缺省3306。" << endl;
    cout << "charset: 数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现乱码。" << endl;
    cout << "inifilename: 数据入库的参数设置，如表名，入库前操作等。json数组格式，保存条操作内容" << endl;
    cout << "datapath: 待入库的数据json文件保存地址" << endl;
    cout << "datapathbak: 入库完成后数据文件的备份地址" << endl;
    cout << "datapatherr: 入库出错数据文件的存入地址" << endl;
    cout << "timeval: 扫描datapath目录的时间间隔" << endl;
    cout << "phtimeout: 进程心跳时间，单位秒，默认30秒" << endl;
    cout << "注意：json文件中所有路径都必须是绝对路径" << endl;
    cout << "inifilename文件的json应该有的字段如下：" << endl;
    cout << "matchfile: json数据文件匹配的规则，需要入库哪些指定文件，格式为正则表达式" << endl;
    cout << "tablename: 数据入库的表名，必须有且正确" << endl;
    cout << "update: 是否更新数据，false表示不更新，true表示更新" << endl;
    cout << "sql: 数据入库前执行的sql语句，默认空" << endl;
}

void EXIT(int sig)
{
    lg.writeLine("程序被信号中断，sig=%d", sig);
    exit(0);
}