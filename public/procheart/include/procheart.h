#include "../../cpublic.h"
#include "../../semaphore/include/semaphore.h"
#include <ctime>
#include <sys/types.h>
// 这个文件相关类通过使用共享内存将进程的信息保存在系统中
// 记录进程的存活情况，保存心跳信息

// 进程数据类，保存了进程的信息
class proInfo
{
  public:
    pid_t pid; // 进程ID
    std::string name; // 进程名
    int timeout;      // 超时时间，如果超过这个时间没有心跳则认为进程死亡
    time_t lastHeart; // 上次心跳时间
    proInfo(const pid_t pid, const std::string name, const int timeout) : pid(pid), name(name), timeout(timeout)
    {
        lastHeart = time(NULL);
    }
};

// 封装了更新进程心跳的类
class procHeart
{
  public:
    int shid; // 共享内存ID
    int pos;  // 当前进程在共享内存数组中的位置
    proInfo *shmp; // 指向共享内存的指针，将进程绑定到共享内存上
    mysem sem;     // 信号量类，用于对共享内存的操作
  public:
    //初始化前三个参数。
    procHeart();
    // 初始化共享内存，绑定到当前进程
    bool addProcInfo(const pid_t pid, const std::string name, const int timeout,key_t key=0x5095,int maxProc=1000);
    // 更新进程心跳
    bool updateHeart();
    // 析构函数，从共享内存中删除心跳记录
    ~procHeart();
};

// 查看共享内存：  ipcs -m
// 删除共享内存：  ipcrm -m shmid
// 查看信号量：      ipcs -s
// 删除信号量：      ipcrm sem semid