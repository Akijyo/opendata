#include "../include/procheart.h"
#include <csignal>
#include <cstring>
#include <ctime>
#include <sys/ipc.h>
#include <sys/shm.h>
using namespace std;

procHeart::procHeart()
{
    this->shid = 0;
    this->pos = -1;
    this->shmp = nullptr;
    this->sem.init(0x5095);
}

bool procHeart::addProcInfo(const pid_t pid, const std::string name, const int timeout, key_t key, int maxProc)
{
    // 已经初始化
    if (this->pos != -1)
    {
        return false;
    }
    // 初始化并获取共享内存
    this->shid = shmget(key, maxProc * sizeof(proInfo), 0666 | IPC_CREAT);
    if (this->shid == -1)
    {
        return false;
    }
    // 将当前进程绑定到共享内存上，即获取共享内存指针
    this->shmp = (proInfo *)shmat(this->shid, 0, 0);
    if (this->shmp == (void *)-1)
    {
        return false;
    }
    // 创建进程信息对象
    proInfo pc(pid, name, timeout);

    this->sem.sem_p();

    // 如果有一个进程因为莫名其妙的原因（kill-9，段错误）而导致异常退出，那么这个进程的心跳信息会留在共享中
    // 进程号是会复用的，如果有一个异常终止的进程，和新开的一个进程号相同，那么在这里复用这个共享内存的进程信息
    for (int i = 0; i < maxProc; i++)
    {
        if (this->shmp[i].pid == pc.pid)
        {
            this->pos = i;
            break;
        }
    }
    // 如果没有复用，那么就找到一个空位置
    if (this->pos == -1)
    {
        for (int i = 0; i < maxProc; i++)
        {
            if (this->shmp[i].pid == 0)
            {
                this->pos = i;
                break;
            }
        }
    }
    //没有找到空位置，共享内存已满
    if (this->pos == -1)
    {
        this->sem.sem_v();
        return false;
    }
    this->sem.sem_v();
    // 将进程信息写入共享内存
    // this->shmp[this->pos] = pc;
    memcpy(&this->shmp[this->pos], &pc, sizeof(proInfo));

    return true;
}

// 更新心跳
bool procHeart::updateHeart()
{
    //说明未初始化
    if(this->pos==-1)
    {
        return false;
    }
    // 互斥锁保护共享内存
    this->mtx.lock();
    this->shmp[this->pos].lastHeart = time(0);
    this->mtx.unlock();
    return true;
}

// 析构函数，将当前进程的心跳信息从共享内存中删除
procHeart::~procHeart()
{
    if (this->pos == -1)
    {
        return;
    }
    // 在共享内存中删除当前进程的心跳信息
    memset(&this->shmp[this->pos], 0, sizeof(proInfo));
    if (this->shmp != nullptr)
    {
        // 解除共享内存绑定
        shmdt(this->shmp);
    }
}


//////
void closeiosignal(bool io)
{
    for(int i=0;i<64;i++)
    {
        if (io)
        {
            close(i);
        }
        signal(i, SIG_IGN);
    }
}
