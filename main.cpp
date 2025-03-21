#include "modhead.h"
#include "service/crtsurfdata/crtsurfdata.h"
using namespace std;

//创建日志对象的全局变量
logfile lg;

void closeIOSignal(bool io)
{
    for (int i = 0; i < 64; i++)
    {
        if (io)
        {
            close(i);
        }
        signal(i, SIG_IGN);
    }
}
void EXIT(int sig)
{
    lg.writeLine("程序被信号中断，sig=%d", sig);
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cout << "usage: " << argv[0] << " <inifile> <outpath> <logfile>" << endl;
        cout << "example: " << argv[0] << " /data/stcode.ini /output/surdata /log/logfile.log" << endl;

        cout << "inifile: 气象站点参数文件名" << endl;
        cout << "outpath:气象站点数据文件存放的目录" << endl;
        cout << "logfile:日志文件名" << endl;
    }
    closeIOSignal(true);

    if (lg.open(argv[3]) == false)
    {
        cout << "打开日志文件失败" << endl;
        return -1;
    }

    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    lg.writeLine("程序开始运行");

    // 1.从站点参数文件中加载站点参数，存放在容器中
    if (loadstcode(argv[1]) == false)
    {
        lg.writeLine("加载站点参数失败");
        EXIT(-1);
    }

    // 2.根据站点参数，生成站点观测数据（随机数）
    crtsurfdata();

    // 3.将生成的站点观测数据写入到文件中
    crtsurffile(argv[2]);

    lg.writeLine("程序运行结束");
}