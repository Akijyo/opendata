#include "fileframe.h"
#include "../../stringop/include/stringop.h"
#include "../../timeframe/include/timeframe.h"


// 处理日志文件的类
class logfile
{
  private:
    std::ofstream lfout;     // 文件输出流
    std::string filename;    // 文件名，使用绝对路径
    std::mutex mtx;          // 互斥锁
    int maxsize;             // 文件最大大小，单位为MB，超过这个大小并且备份标志位为true时，备份文件
    bool isBackup;           // 判断文件是否需要备份,与上面那个参数配合使用
    std::ios::openmode mode; // 文件打开方式
    bool isBuffer;           // 是否使用缓冲区
    // 由于本类可能会被多个线程同时调用，所以需要加锁，同时需要将open函数赋予的值保存到成员变量中，以便其他函数使用

    // 备份文件的封装函数，仅内部调用
    bool backupFile();

  public:
    logfile() {};
    logfile(const logfile &) = delete;
    logfile &operator=(const logfile &) = delete;

    // 判断文件是否打开
    bool isOpen();

    /**
     * @brief 打开文件
     *
     * @param filename 文件名，采用绝对路径
     * @param mode 文件的打开方式，默认为输出模式
     * @param isBackup 文件是否需要备份，默认为true，传入的这个参数会复制给成员变量
     * @param maxsize 最大文件大小，单位为MB，默认为100MB
     * @return true
     * @return false
     */
    bool open(const std::string &filename, std::ios::openmode mode = std::ios::out, bool isBackup = true,
              int maxsize = 100, bool isBuffer = true);

    /**
     * @brief 行写入文件，类似c风格的printf
     *  若文件大小超过最大值，且备份标志位为true，则备份文件
     * 备份的文件名为：日期+原文件名
     * 这个写入函数会自动换行
     * 写入的内容前面自带时间戳
     * @tparam Arg 使用可变参数模版
     * @param fmt 格式化字符串
     * @param args 可变参数
     * @return true
     * @return false
     */
    template <class... Arg> bool writeLine(const std::string &fmt, Arg... args)
    {
        if (this->lfout.is_open() == false)
        {
            return false;
        }

        // 判断是否需要备份文件
        if (this->backupFile() == false)
        {
            return false;
        }

        // 格式化字符串
        std::string str;
        sFomat(str, fmt.c_str(), args...);

        // 写入文件，这里需要上锁
        std::unique_lock<std::mutex> lck(this->mtx);
        {
            this->lfout << getCurTime() << "  " << str << std::endl;
        }
        return this->lfout.good();
    }

    // 重载<<运算符，让类能像ofstream一样使用，不能用endl，不知道为啥
    // 上一个函数自带时间戳，这个函数不带时间戳，不然输出结果有问题
    // 这个函数的锁设计的有问题，不论如何锁都不会很完美，非单线程尽量不使用
    template <class T> logfile &operator<<(const T &data)
    {
        if (this->lfout.is_open() == false)
        {
            return *this;
        }

        // 判断是否需要备份文件
        if (this->backupFile() == false)
        {
            return *this;
        }

        // 多线程下写入文件的格式乱是因为这里，因为每次调用<<算调用一次这个函数
        // 锁的获取在外部同一行语句间会中断，导致格式极其混乱
        std::unique_lock<std::mutex> lck(this->mtx);
        {
            this->lfout << data;
        }
        return *this;
    }

    // 处理流控制符，不然<<运算符不能用endl
    logfile &operator<<(std::ostream &(*manip)(std::ostream &))
    {
        if (this->lfout.is_open() == false)
        {
            return *this;
        }

        if (this->backupFile() == false)
        {
            return *this;
        }

        std::unique_lock<std::mutex> lck(this->mtx);
        manip(this->lfout);
        return *this;
    }
    ~logfile();
};