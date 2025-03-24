#include "fileframe/include/cdir.h"
#include "fileframe/include/fileframe.h"
#include "procheart/include/procheart.h"
#include "stringop/include/stringop.h"
#include "timeframe/include/timeframe.h"

procHeart ph;

using namespace std;
void EXIT(int sig)
{
    cout << "程序退出，sig=" << sig << endl;
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cout << "Usage: " << argv[0] << " <filepath> <match> <timeval>" << endl;
        cout << "Example: " << argv[0] << " /temp/output \".*\\.json\\b\" 1" << endl;
        cout << "filepath: 文件绝对路径" << endl;
        cout << "match: 匹配文件名的正则表达式，shell会对反斜杠解释，与c++冲突，需要加上双引号" << endl;
        cout << "timeval: 备份文件的时间间隔单位天，可以是小数" << endl;
        cout << "--------------------------------------------" << endl;
        cout << "这是一个工具程序，用于压缩指定文件" << endl;
        cout << "本程序将调用/usr/bin/gzip命令进行压缩，所以请确保系统中有gzip命令" << endl;
        cout << "本程序将filepath目录以及子目录中timeval天前且正则匹配的文件全部压缩，timeval可以是"
                "小数"
             << endl;

        return 0;
    }

    for (int i = 0; i < 64; i++)
    {
        // close(i);
        signal(i, SIG_IGN);
    }
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    // 获取指定偏移的时间
    string dtime = getCurTime(TimeType::TIME_TYPE_ONE, -1 * atof(argv[3]) * 24 * 3600);

    // 添加进程心跳信息
    ph.addProcInfo(getpid(), argv[0], 120);

    // 1.打开目录
    cdir dir;
    dir.openDir(argv[1], argv[2], 10000, true, false);

    // 2.遍历历史文件，如果文件的修改时间早于dtime，则删除文件
    while (dir.readFile())
    {
        //如果文件的修改时间早于dtime，且文件不是已经压缩的文件，则压缩文件
        if (dir.modifytime < dtime && matchstr(dir.filename, ".*\\.gz\\b") == false)
        {
            //拼接linux命令
            string cmd = "gzip -f " + dir.fullpath + " 1>/dev/null 2>/dev/null";
            system(cmd.c_str());
        }
        ph.updateHeart();
    }
    return 0;
}
