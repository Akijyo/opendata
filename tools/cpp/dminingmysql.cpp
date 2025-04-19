#include "cpublic.h"
#include "fileframe/include/fileio.h"
#include "fileframe/include/logfile.h"
#include "include/Iconnection.h"
#include "include/msql.h"
#include "procheart/include/procheart.h"
#include "stringop/include/jsonns.h"
#include "stringop/include/split.h"
#include "stringop/include/stringop.h"
#include "timeframe/include/timeframe.h"
#include <algorithm>
#include <nlohmann/json.hpp>
#include <string>
#include <unistd.h>

using namespace std;

// 程序参数的结构体
class argss
{
  public:
    string host;    // 数据库地址
    int port;       // 数据库端口号
    string user;    // 数据库用户名
    string passwd;  // 数据库密码
    string db;      // 数据库名
    string charset; // 数据库字符集

    string selectsql; // 抽取数据的sql语句
    string fieldstr;  // 抽取数据的sql语句输出结果集的字段名列表，中间用逗号分割，将作为json文件的字段名
    string fieldtype; // 抽取数据的sql语句输出结果集的字段类型列表，字段类型是c/c++类型，且严格与fieldstr一一对应

    string outpath;   // 输出json文件存放的目录
    string bfilename; // 输出json文件的前缀
    string efilename; // 输出json文件的后缀

    int maxcount;     // 一份输出的json文件最大记录数
    string starttime; // 程序运行的时间区间，列入02，13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行.

    string increfield;    // 递增字段名，它必须是fieldstr中的字段名，并且只能是整形，一般为自增字段
    string increfilename; // 已抽取数据的递增字段最大值存放的文件，如果改文件丢失，将重新抽取全部的数据。

    // 增量抽取相关参数，下面参数和increfilename二选一
    string host1;
    int port1;
    string user1;
    string passwd1;
    string db1;
    // 增量抽取别人数据库的时候，需要自己数据库有一个表，这个表记录了指定抽取进程上次抽取到递增值的最大值
    // 所以这里需要第二组数据库参数，保留的是自己数据库的连接参数
    // 本程序需要额外指定特殊进程名，比如指定抽取公安数据的进程，抽取税务局的进程。。。
};
argss starg;
// 对fieldstr,fieldtype进行分割
ccmdstr sfieldstr;
ccmdstr sfieldtype;

// 创建日志文件对象
logfile lg;
// 创建进程心跳对象
procHeart ph;
// 进程心跳时间，单位秒
int phtimeout;
// 进程名的全局对象
string procname;

// 记录增量抽取的上次最大值
int incremax = 0;
// 增量字段在sql语句查询结果中的位置
int increfieldindex = -1;

// 程序接受信号退出的函数
void EXIT(int sig);
// 程序的帮助文档
void help();
// 解析程序传入json格式的参数
bool parseJson(const string &jsonfile);
// 判断程序是否在可运行的时间区间
bool isRunTime(const string &starttime);
// 数据抽取的主模块
void dmingdata(shared_ptr<IConnection> connection);
// 获取上次增量抽取的最大值
bool getIncreMax();
// 将程序运行完成后，更新增量抽取的最大值到数据库或者文件中
bool updateIncreMax();

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        help();
        return 0;
    }
    // 防止信号影响
    closeiosignal(false);
    // 让2号信号和15号信号都调用EXIT函数
    // 2号信号是Ctrl+C，15号信号是kill命令
    signal(2, EXIT);
    signal(15, EXIT);

    // 打开日志文件
    if (!lg.open(argv[1], ios::out | ios::app))
    {
        cout << "打开日志文件失败！" << endl;
        return 0;
    }
    // 解析json文件
    if (!parseJson(argv[2]))
    {
        lg.writeLine("解析json文件失败");
        return 0;
    }
    // 初始化进程心跳
    ph.addProcInfo(getpid(), procname, phtimeout);

    // 1.判断程序是否在运行区间
    if (!isRunTime(starg.starttime))
    {
        return 0;
    }
    // 2.连接数据库
    shared_ptr<IConnection> connection = make_shared<mysql>();
    if (!connection->connect(starg.host, starg.user, starg.passwd, starg.db, starg.port))
    {
        lg.writeLine("数据库连接失败：%s", connection->last_error_);
        return 0;
    }
    // 3.数据抽取
    if (!getIncreMax())
    {
        return 0;
    }
    dmingdata(connection);
    // 4.更新增量抽取的最大值
    if (!updateIncreMax())
    {
        lg.writeLine("更新增量最大值失败");
        return 0;
    }

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

// 获取上次增量抽取的最大值
bool getIncreMax()
{
    //如果不是增量抽取，则返回
    if (starg.increfield.empty())
    {
        return true;
    }
    // 获取增量字段在sql语句查询结果中的位置
    for (int i = 0; i < sfieldstr.size(); i++)
    {
        if (sfieldstr[i] == starg.increfield)
        {
            increfieldindex = i;
            break;
        }
    }
    if (increfieldindex == -1)
    {
        lg.writeLine("增量字段在sql语句查询结果中不存在");
        return false;
    }
    if (!starg.host1.empty())//如果host1不为空，则去查找数据库
    {
        // 连接本地数据库
        shared_ptr<IConnection> connection = make_shared<mysql>();
        if (!connection->connect(starg.host1, starg.user1, starg.passwd1, starg.db1, starg.port1))
        {
            lg.writeLine("增量抽取中连接数据库失败：%s", connection->last_error_);
            return false;
        }
        // 准备sql
        string sql = "select incremax from T_INCREMAX where procname='" + procname + "'";
        // 执行sql
        if (!connection->query(sql))
        {
            lg.writeLine("查询增量最大值语句执行失败：%s", sql.c_str());
            return false;
        }
        // 获取增量最大值
        if (connection->next())
        {
            incremax = connection->get_int(0);
        }
    }
    else//去查找文件
    {
        // 打开记录本进程的增量最大值的文件
        rdfile rfincre;
        if (!rfincre.open(starg.increfilename))
        {
            return true;
        }
        // 读取文件，该文件中只有一行记录
        string line;
        rfincre.readLine(line);
        // 解析文件中的增量最大值
        incremax = stoi(line);
    }
    lg.writeLine("获取增量最大值：%d", incremax);

    return true;
}

// 数据抽取的主模块
void dmingdata(shared_ptr<IConnection> connection)
{
    // 3.1准备sql
    string sql = starg.selectsql;
    // 3.1.1如果是增量抽取，则需要在sql语句中添加递增字段的条件
    if (!starg.increfield.empty())
    {
        replaceStr(sql,"?",to_string(incremax));
    }
    // 3.2执行sql语句
    if (!connection->query(sql))
    {
        lg.writeLine("查询sql语句执行失败：%s", connection->last_error_);
        return;
    }

    // 获取文件时间戳
    string filetime = getCurTime(TimeType::TIME_TYPE_TWO);//获取当前时间
    filetime += ":00";//秒位置0
    filetime = pickNum(filetime);//去掉符号
    // 定义文件序号
    int fileindex = 1;

    wtfile wf;
    // 定义跟json对象
    nlohmann::json root;
    string fullFilePath;
    while (connection->next())
    {
        // 3.3获取sql执行结果

        // 打开文件
        if (!wf.isOpen())
        {
            fullFilePath = starg.outpath + "/" + starg.bfilename + "_" + filetime + "_" + starg.efilename + "_" +
                           to_string(fileindex) + ".json";
            // 打开文件
            if (!wf.open(fullFilePath.c_str()))
            {
                lg.writeLine("打开文件失败：%s", fullFilePath.c_str());
                return;
            }
        }
        // 获取每个字段的值
        nlohmann::json subarr;
        for (int i = 0; i < sfieldstr.size(); i++)
        {
            // 如果是增量字段，保存最大值
            if (i == increfieldindex)
            {
                int value = connection->get_int(i);
                incremax = max(incremax, value);
                continue;
            }
            if (sfieldtype[i] == "int")
            {
                int value = connection->get_int(i);
                subarr[sfieldstr[i]] = value;
            }
            else if (sfieldtype[i] == "double" || sfieldtype[i] == "float")
            {
                double value = connection->get_double(i);
                subarr[sfieldstr[i]] = value;
            }
            else if (sfieldtype[i] == "string")
            {
                string value = connection->value(i).value();
                subarr[sfieldstr[i]] = value;
            }
            else
            {
                lg.writeLine("不支持的字段类型：%s", sfieldtype[i].c_str());
                return;
            }
        }
        root.push_back(subarr);
        if (starg.maxcount != 0 && root.size() >= starg.maxcount)
        {
            // 文件序号+1，为创建新文件做准备
            fileindex++;
            // 写入文件
            wf << root.dump(4) << "\n";
            // 记录文件生成日志
            lg.writeLine("数据抽取文件生成：%s，记录数%d", fullFilePath.c_str(), root.size());
            // 清空json对象
            root.clear();
            // 关闭原有文件
            wf.close();
            // 更新进程心跳
            ph.updateHeart();
        }
    }
    // 3.4将最后结果写入文件
    if (wf.isOpen() && !root.empty())
    {
        wf << root.dump(4) << "\n";
        lg.writeLine("数据抽取文件生成：%s，记录数%d", fullFilePath.c_str(), root.size());
        wf.close();
    }
    else if (wf.isOpen() && root.empty())
    {
        // 没有数据，关闭文件并记录
        wf.close();
    }
    // 更新进程心跳
    ph.updateHeart();
}

// 将程序运行完成后，更新增量抽取的最大值到数据库或者文件中
bool updateIncreMax()
{
    if (starg.increfield.empty())
    {
        return true;
    }
    // 1.更新到数据库
    if (!starg.host1.empty())
    {
        // 连接本地数据库
        shared_ptr<IConnection> connection = make_shared<mysql>();
        if (!connection->connect(starg.host1, starg.user1, starg.passwd1, starg.db1, starg.port1))
        {
            lg.writeLine("增量抽取中连接数据库失败：%s", connection->last_error_);
            return false;
        }
        // 准备sql
        string sql = "update T_INCREMAX set incremax=" + to_string(incremax) + " where procname='" + procname + "'";
        // 执行sql
        if (!connection->update(sql))
        {
            lg.writeLine("更新增量最大值语句执行失败：%s", sql.c_str());
            return false;
        }
    }
    else
    {
        // 2.更新到文件
        wtfile wf;
        if (!wf.open(starg.increfilename.c_str()))
        {
            lg.writeLine("打开增量最大值文件失败：%s", starg.increfilename.c_str());
            return false;
        }
        wf << to_string(incremax) << "\n";
    }
    return true;
}

bool parseJson(const string &jsonfile)
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
    if (!js.get("selectsql", starg.selectsql))
    {
        lg.writeLine("json文件中没有selectsql字段");
        return false;
    }
    if (!js.get("fieldstr", starg.fieldstr))
    {
        lg.writeLine("json文件中没有fieldstr字段");
        return false;
    }
    if (!js.get("fieldtype", starg.fieldtype))
    {
        lg.writeLine("json文件中没有fieldtype字段");
        return false;
    }
    sfieldstr.split(starg.fieldstr, ",");
    sfieldtype.split(starg.fieldtype, ",");
    if (sfieldstr.size() != sfieldtype.size())
    {
        lg.writeLine("json文件中fieldstr,fieldtype字段的个数不一致");
        return false;
    }

    // 输出文件相关参数
    if (!js.get("outpath", starg.outpath))
    {
        lg.writeLine("json文件中没有outpath字段");
        return false;
    }
    if (!js.get("bfilename", starg.bfilename))
    {
        lg.writeLine("json文件中没有bfilename字段");
        return false;
    }
    if (!js.get("efilename", starg.efilename))
    {
        lg.writeLine("json文件中没有efilename字段");
        return false;
    }

    // 其他可选参数
    if (!js.get("maxcount", starg.maxcount))
        starg.maxcount = 0;
    if (!js.get("starttime", starg.starttime))
        starg.starttime = "";

    // 增量抽取相关参数
    if (!js.get("increfield", starg.increfield))
        starg.increfield = "";
    if (starg.increfield.empty() == false)
    {
        // 首先尝试获取 host1，因为它优先
        bool hasHost1 = js.get("host1", starg.host1);

        if (hasHost1)
        {
            // 如果有host1，则需要其他数据库连接参数
            if (!js.get("user1", starg.user1))
            {
                lg.writeLine("json文件中没有user1字段");
                return false;
            }
            if (!js.get("passwd1", starg.passwd1))
            {
                lg.writeLine("json文件中没有passwd1字段");
                return false;
            }
            if (!js.get("port1", starg.port1))
                starg.port1 = 3306;
            if (!js.get("db1", starg.db1))
            {
                lg.writeLine("json文件中没有db1字段");
                return false;
            }
        }
        else
        {
            // 如果没有host1，则必须有increfilename
            if (!js.get("increfilename", starg.increfilename))
            {
                lg.writeLine("增量抽取模式下，increfilename和host1参数必须二选一");
                return false;
            }
        }
    }

    // 获取进程名
    if (!js.get("procname", procname))
    {
        lg.writeLine("json文件中没有procname字段");
        return false;
    }

    // 进程心跳超时参数
    int timeout = 0;
    if (!js.get("phtimeout", timeout))
        timeout = 30; // 默认30秒

    // 日志记录参数信息
    // lg.writeLine("host=%s", starg.host.c_str());
    // lg.writeLine("port=%d", starg.port);
    // lg.writeLine("user=%s", starg.user.c_str());
    // lg.writeLine("passwd=******");
    // lg.writeLine("charset=%s", starg.charset.c_str());
    // lg.writeLine("selectsql=%s", starg.selectsql.c_str());
    // lg.writeLine("fieldstr=%s", starg.fieldstr.c_str());
    // lg.writeLine("fieldlen=%s", starg.fieldlen.c_str());
    // lg.writeLine("outpath=%s", starg.outpath.c_str());
    // lg.writeLine("bfilename=%s", starg.bfilename.c_str());
    // lg.writeLine("efilename=%s", starg.efilename.c_str());
    // lg.writeLine("maxcount=%d", starg.maxcount);
    // lg.writeLine("starttime=%s", starg.starttime.c_str());

    // if (!starg.increfield.empty())
    // {
    //     lg.writeLine("increfield=%s", starg.increfield.c_str());
    //     lg.writeLine("increfilename=%s", starg.increfilename.c_str());

    //     if (!starg.host1.empty())
    //     {
    //         lg.writeLine("host1=%s", starg.host1.c_str());
    //         lg.writeLine("port1=%d", starg.port1);
    //         lg.writeLine("user1=%s", starg.user1.c_str());
    //         lg.writeLine("passwd1=******");
    //     }
    // }

    // lg.writeLine("phtimeout=%d", timeout);

    return true;
}

void help()
{
    cout << "Usage: dminingmysql <logfile> <jsonfile>" << endl;
    cout << "logfile：用于保存抽取数据日志的文件" << endl;
    cout << "jsonfile：用于保存程序传入参数的json文件" << endl;
    cout << "Example: ./dminingmysql /temp/log/dminingCode.log "
            "/home/akijyo/桌面/code/c++/opendata/tools/others/dminingCode.json"
         << endl
         << endl;
    cout << "json文件应有的字段：" << endl;
    cout << "host: 数据库地址。" << endl;
    cout << "user: 数据库用户。" << endl;
    cout << "passwd: 数据库密码。" << endl;
    cout << "db: 数据库名。" << endl;
    cout << "port: 数据库端口号，缺省3306。" << endl;
    cout << "charset: 数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现乱码。" << endl;
    cout << "selectsql: 从数据库源数据库抽取数据的sql语句，如果是增量抽取，一定要用递增字段作为查询条件，且增量值在sql中用?指出。" << endl;
    cout << "fieldstr: 抽取数据的sql语句输出结果集的字段名列表，中间用逗号分割，将作为json文件的字段名" << endl;
    cout << "fieldtype: 抽数据的sql语句输出结果集的字段类型列表，字段类型是c/c++类型，且严格与fieldstr一一对应" << endl;
    cout << "outpath: 输出json文件存放的目录" << endl;
    cout << "bfilename: 输出json文件的前缀" << endl;
    cout << "efilename: 输出json文件的后缀" << endl;
    cout << "maxcount: "
            "输出json文件的最大记录数，缺省是0，表示不限制，如果本参数取值为0，注意适当加大timeout的取值，防止程序超时"
         << endl;
    cout << "starttime: 程序运行的时间区间，列入02，13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行."
         << endl
         << "           如果starttime为空，那么starttime参数将失效，只要倴城启动就会执行数据抽取，为了减少" << endl
         << "           数据源的压力，从数据库抽取数据的时候，一般在对方数据库最闲的时候进行" << endl;
    cout << "increfield: "
            "递增字段名，它必须是fieldstr中的字段名，并且只能是整形，一般为自增字段。如果increfield为空，将不增量抽取"
         << endl;
    cout << "increfilename: 已抽取数据的递增字段最大值存放的文件，如果改文件丢失，将重新抽取全部的数据。" << endl;
    cout << "host1: 已经抽取数据的递增字段最大值存放的数据库连接参数，该内容和increfilename二选一，该内容优先" << endl;
    cout << "user1: 同上，用户名" << endl;
    cout << "passwd1: 同上，密码" << endl;
    cout << "db: 同上，数据库名。" << endl;
    cout << "port1: 同上，端口，缺省3306。" << endl;
    cout << "phtimeout: 进程心跳时间，单位秒，默认30秒" << endl;
    cout << "注意：json文件中所有路径都必须是绝对路径" << endl;
}

void EXIT(int sig)
{
    lg.writeLine("程序被信号中断，sig=%d", sig);
    exit(0);
}