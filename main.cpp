#include "fileframe/include/fileio.h"
#include "modhead.h"
#include "stringop/include/split.h"
#include <cstring>
#include <list>
#include <type_traits>
using namespace std;

//定义日志文件类的全局变量
logfile lg;

// 定义站点参数的结构体
struct st_stcode
{
    string provname; // 省份名称
    string obtid;    // 站点编号
    string obtname;  // 站点名称
    double lat;      // 纬度：单位度
    double lon;      // 经度：单位度
    double alt;      // 海拔：单位米
};
list<st_stcode> stcodelist; // 存放站点参数的容器
//把站点文件参数加载到容器中
bool loadstcode(const string &inifile)
{
    //创建文件读取对象
    rdfile rf;
    if(rf.open(inifile) == false)
    {
        lg.writeLine("打开站点参数文件失败：%s", inifile.c_str());
        return false;
    }
    //读掉标题类
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

    for(auto &it : stcodelist)
    {
        lg.writeLine("provname:%s, obtid:%s, obtname:%s, lat:%f, lon:%f, alt:%f\n", it.provname.c_str(), it.obtid.c_str(), it.obtname.c_str(), it.lat, it.lon, it.alt);
    }
    return true;
}

void closeIOSignal(bool io)
{
    for (int i = 0; i < 64; i++)
    {
        if(io)
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


int main(int argc,char* argv[])
{
    if (argc != 4)
    {
        cout << "usage: " << argv[0] << " <inifile> <outpath> <logfile>" << endl;
        cout << "example: " << argv[0] << " /data/stcode.ini /output/surdata /log/logfile.log" << endl;

        cout << "inifile: 气象站点参数文件名" << endl;
        cout << "outpath:气象站点数据文件存放的目录" << endl;
        cout << "logfile:日志文件名" << endl;
    }

    if (lg.open(argv[3]) == false)
    {
        cout << "打开日志文件失败" << endl;
        return -1;
    }

    closeIOSignal(false);
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    lg.writeLine("程序开始运行");

    // 1.从站点参数文件中加载站点参数，存放在容器中
    if(loadstcode(argv[1]) == false)
    {
        lg.writeLine("加载站点参数失败");
        EXIT(-1);
    }

    // 2.根据站点参数，生成站点观测数据（随机数）

    // 3.将生成的站点观测数据写入到文件中

    lg.writeLine("程序运行结束");
}