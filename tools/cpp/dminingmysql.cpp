#include "cpublic.h"
#include "fileframe/include/logfile.h"
#include "procheart/include/procheart.h"

using namespace std;

// 创建日志文件对象
logfile lg;
// 创建进程心跳对象
procHeart ph;
// 程序接受信号退出的函数
void EXIT(int sig);
// 程序的帮助文档
void help();

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
    if (!lg.open(argv[1]))
    {
        cout << "打开日志文件失败！" << endl;
        return 0;
    }
    return 0;
}

void help()
{
    cout << "Usage: dminingmysql <logfile> <jsonfile>" << endl;
    cout << "logfile：用于保存抽取数据的文件" << endl;
    cout << "jsonfile：用于保存程序传入参数的json文件" << endl;
    cout << "Example: ./dminingmysql /temp/log/dminingmysql.log "
            "/home/akijyo/桌面/code/c++/opendata/tools/others/dminingmysql.json"
         << endl
         << endl;
    cout << "json文件应有的字段：" << endl;
    cout << "host: 数据库地址。" << endl;
    cout << "user: 数据库用户。" << endl;
    cout << "passwd: 数据库密码。" << endl;
    cout << "charset: 数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现乱码。" << endl;
    cout << "selectsql: 从数据库源数据库抽取数据的sql语句，如果是增量抽取，一定要用递增字段作为查询条件。" << endl;
    cout << "fieldstr: 抽取数据的sql语句输出结果集的字段名列表，中间用逗号分割，将作为json文件的字段名" << endl;
    cout << "fieldlen: 抽取数据的sql语句输出结果集字段的长度列表，中间用逗号分割。fieldstr与fieldlen的字段必须一一对应"
         << endl;
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
    cout << "phtimeout: 进程心跳时间，单位秒，默认30秒" << endl;
    cout << "注意：json文件中所有路径都必须是绝对路径" << endl;
}

void EXIT(int sig)
{
    lg.writeLine("程序被信号中断，sig=%d", sig);
    exit(0);
}