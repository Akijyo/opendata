#pragma once
#include "../cpublic.h"
#include "../timeframe/timeframe.h"
#include "../stringop/stringop.h"
#include <vector>
// 这个文件定义了一个打开目录的类，用于保存和遍历目录下的文件
//本文件采用c++17的文件操作

class cdir
{
  private:
    std::vector<std::string> filelist;//保存读取到的目录的全路径
    TimeType type;//类中有关时间读取所用到的时间格式，默认是第一种类型
    unsigned int pos;      // 当前读取到的文件的位置

  public:
    // /temp/aaa/test.cpp
    std::string dirname; // pos位置的文件的目录路径，如/temp/aaa
    std::string filename; // pos位置的文件的文件名，如test.cpp
    std::string fullpath; // pos位置的文件的全路径，如/temp/aaa/test.cpp
    int filesize;         // pos位置的文件的大小，单位为字节
    std::string modifytime; // pos位置的文件的最后修改时间
    std::string createtime; // pos位置的文件的创建时间
    std::string accesstime; // pos位置的文件的最后访问时间

    cdir() : type(TimeType::TIME_TYPE_ONE), pos(0) {};//默认构造函数
    cdir(TimeType t) : type(t), pos(0) {};
    cdir(const cdir &) = delete; // 禁用拷贝构造函数
    cdir &operator=(const cdir &) = delete; // 禁用赋值构造函数

    /**
     * @brief 打开并读取目录
     * 
     * @param dir 传入目录的绝对路径
     * @param match 匹配的文件名，正则表达式
     * @param maxsize 读取的最大文件数
     * @param recursive 是否递归读取子目录
     * @param sort 是否按文件名排序
     * @return true 
     * @return false 
     */
    bool openDir(const std::string &dir, const std::string &match,
                 const int maxsize=10000,bool recursive=false,bool isSort=false);

    //根据pos位置读取文件信息，调用一次就读一个文件
    bool readFile();

    // 获取文件列表的大小
    unsigned int size();
};
