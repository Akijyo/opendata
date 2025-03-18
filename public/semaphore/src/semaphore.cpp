#include "../include/semaphore.h"

bool mysem::init(int key, unsigned short value, short sem_flag)
{
    if (semid != -1)
        return false;
    this->sem_flag = sem_flag;
    if ((this->semid = semget(key, 1, 0666)) == -1)
    {
        // 如果信号量不存在则创建
        if (errno == ENOENT)
        {
            if ((this->semid = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
            {
                if (errno == EEXIST) // 如果错误代码是已存在，则获取他
                {
                    if ((this->semid = semget(key, 1, 0666)) == -1)
                    {
                        //perror("init 1 semget error!");
                        return false;
                    }
                    return true;
                }
                else // 其他错误
                {
                    //perror("init 2 semget error!");
                    return false;
                }
            }
            // 初始化信号量的值
            union semun sem_union;
            sem_union.val = value;
            if (semctl(this->semid, 0, SETVAL, sem_union) == -1)
            {
                //perror("init semctl error!");
                return false;
            }
        }
        else
        {
            //perror("init 3 semget error!");
            return false;
        }
    }
    return true;
}

bool mysem::sem_p(short value)
{
    if (this->semid == -1)
    {
        perror("no semid!");
        return false;
    }
    sembuf sem_b; // sem.h中定义的结构体 用于操作信号量
    /*
    sem_num：信号量集中的信号量编号。信号量集可以包含多个信号量，sem_num 指定要操作的信号量的索引。
    sem_op：信号量操作。可以是以下值：
        正值：增加信号量的值（释放资源）。
        负值：减少信号量的值（请求资源）。如果信号量的值小于 sem_op 的绝对值，操作将会阻塞，直到信号量的值足够。
        零：等待信号量的值变为零。
    sem_flg：操作标志。可以是以下值：
        IPC_NOWAIT：如果操作会阻塞，则立即返回错误。
        SEM_UNDO：当进程终止时，撤销未完成的操作。
    */
    sem_b.sem_num = 0;    // 信号量编号
    sem_b.sem_op = value; // p操作的值必须为负数（请求资源）
    sem_b.sem_flg = this->sem_flag;
    if (semop(this->semid, &sem_b, 1) == -1)
    {
        perror("P semop error!");
        return false;
    }
    /*
    semop 函数可以用于对信号量进行以下几种操作：
        P 操作（请求资源）：
            sem_op 设置为负值。
            该操作会尝试减少信号量的值。如果信号量的值小于 sem_op 的绝对值，操作将会阻塞，直到信号量的值足够。
        V 操作（释放资源）：
            sem_op 设置为正值。
            该操作会增加信号量的值，表示释放资源。
        等待信号量的值变为零：
            sem_op 设置为零。
            该操作会阻塞，直到信号量的值变为零。
    */
    return true;
}

bool mysem::sem_v(short value)
{
    if (this->semid == -1)
    {
        perror("no semid!");
        return false;
    }
    sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = value; // v操作的值必须为正数（释放资源）
    sem_b.sem_flg = this->sem_flag;
    if (semop(this->semid, &sem_b, 1) == -1)
    {
        perror("V semop error!");
        return false;
    }
    return true;
}

int mysem::getvalue()
{
    if(this->semid==-1)
    {
        perror("no semid!");
        return -1;
    }
    return semctl(this->semid,0,GETVAL);
}

bool mysem::destroy()
{
    if(this->semid==-1)
    {
        perror("no semid!");
        return false;
    }
    if(semctl(this->semid,0,IPC_RMID)==-1)
    {
        perror("destroy semctl error!");
        return false;
    }
    return true;
}