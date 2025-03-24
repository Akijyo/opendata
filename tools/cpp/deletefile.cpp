#include "fileframe/include/cdir.h"
#include "fileframe/include/fileframe.h"
#include "timeframe/include/timeframe.h" 
#include "procheart/include/procheart.h"

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
        cout << "match: 匹配文件名的正则表达式，shell会对反斜杠解释，与c++冲突，需要加伤双引号" << endl;
        cout << "timeval: 删除文件的时间间隔单位天，可以是小数" << endl;
        cout << "--------------------------------------------" << endl;
        cout << "这是一个工具程序，用于删除历史的数据文件或者日志" << endl;
        cout << "本程序将filepath目录以及子目录中timeval天前且正则匹配的文件全部删除，timeval可以是小数" << endl;
        return 0;
    }

    for (int i = 0; i < 64; i++)
    {
        close(i);
        signal(i, SIG_IGN);
    }
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);
    string dtime = getCurTime(TimeType::TIME_TYPE_ONE, -1 * atof(argv[3]) * 24 * 3600);

    // 添加进程心跳信息
    ph.addProcInfo(getpid(), argv[0], 30);

    // 1.打开目录
    cdir dir;
    dir.openDir(argv[1], argv[2], 10000, true, false);

    // 2.遍历历史文件，如果文件的修改时间早于dtime，则删除文件
    while (dir.readFile())
    {
        if (dir.modifytime < dtime)
        {
            deletefile(dir.fullpath);
        }
        ph.updateHeart();
    }
    return 0;
}
