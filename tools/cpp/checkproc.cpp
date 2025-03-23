#include "fileframe/include/logfile.h"
#include "procheart/include/procheart.h"
#include <sys/types.h>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << endl;
        cout << "Usage: " << argv[0] << " <logfile>" << endl;
        cout << "Example: " << argv[0] << " /temp/log/checkproc.log" << endl;
        cout << "logfile: 日志文件名" << endl;
        cout << "-----------------------------" << endl;
        cout << "本程序用于检查进程是否存活，如果进程不存在则启动进程。" << endl;
        cout << "--1.本程序由processctrl调用，不要直接调用。" << endl;
        cout << "--2.为了避免普通用户误杀，本程序应由root用户调用。" << endl;
        cout << "--3.本程序不会被kill杀死，但可以用kill -9强行杀死。" << endl;
        cout << "--4.本程序建议每10s内运行一次，以保证进程的及时启动。" << endl;
        return 0;
    }
    for(int i=0;i<64;i++)
    {
        signal(i,SIG_IGN);
    }
    // 1.打开日志文件
    logfile lg;
    if (lg.open(argv[1], ios::out | ios::app) == false)
    {
        cout << "打开日志文件失败" << endl;
        return -1;
    }

    // 2.创建/获取共享内存
    int shid = shmget(KEY_DEFAULT, MAX_PROC * sizeof(proInfo), 0);

    // 3.获取共享内存指针
    proInfo *shmp = (proInfo *)shmat(shid, 0, 0);

    // 4.遍历共享内存，检查进程是否存活
    mysem sem;
    sem.init(KEY_DEFAULT);
    sem.sem_p();
    for (int i = 0; i < MAX_PROC; i++)
    {
        if (shmp[i].pid == 0)
        {
            continue;
        }
        else
        {
            time_t now = time(NULL);
            time_t diff = now - shmp[i].lastHeart;
            if (diff > shmp[i].timeout)
            {
                // 如果进程已经死亡但是信息残留在共享内存（kill-9，段错误），则清除这个信息
                if (kill(shmp[i].pid, 0) == -1)
                {
                    lg.writeLine("进程%s(pid=%d)已经不存在", shmp[i].name, shmp[i].pid);
                    memset(&shmp[i], 0, sizeof(proInfo));
                }
                else // 终止进程
                {
                    // 保存进程号，在procheart类的析构函数中，如果进程正常退出，
                    // 那么会将共享内存中的自己进程信息清除，那么下面循环检测的过程中，
                    // 将会对一个在共享内存中不存在的进程号进行kill，
                    // 由于procheat类的析构函数会把那个地址的进程号恢复成0，会导致向0号进程，
                    // 也就是现在程序的进程发信号并杀死，导致自己杀自己
                    // 为了避免这种情况需要将那个要检测的进程信息备份
                    proInfo dpi = shmp[i];
                    // 尝试正常终止进程（发送15信号终止，但是进程可能因为特殊原因无法因15终止，所以需要kill-9）
                    kill(shmp[i].pid, SIGTERM);

                    // 每隔1秒检查一次进程是否正常终止，总计5秒（进程退出需要时间）
                    // 5秒后如果进程还没有正常终止，则强制终止进程
                    int signret = 0;
                    for (int i = 0; i < 5; i++)
                    {
                        // 用0信号判断进程是否正常终止
                        if ((signret = kill(dpi.pid, 0)) == -1)
                        {
                            break;
                        }
                        this_thread::sleep_for(chrono::seconds(1));
                    }
                    if (signret == -1)
                    {
                        lg.writeLine("进程%s(pid=%d)正常终止", dpi.name, dpi.pid);
                    }
                    else
                    {
                        lg.writeLine("进程%s(pid=%d)无法正常终止，强制终止", dpi.name, dpi.pid);
                        kill(dpi.pid, SIGKILL);
                    }
                    memset(&shmp[i], 0, sizeof(proInfo));
                }
            }
        }
    }
    sem.sem_v();

    // 5. 把共享内存分离
    shmdt(shmp);
    return 0;
}