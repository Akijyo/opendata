#pragma once
#include "fileframe.h"
#include <fstream>
#include <ios>
#include <string>

// 在本项目中，这两个类不需要出现同一时间下多个线程同时操作一个文件，所以不需要考虑线程安全
// 同时在操作中尽量避免多个线程使用同一实例的情况

// 写文件的类
class wtfile
{
  private:
    std::ofstream fout;
    std::string filename;
    std::string tempfilename;

  public:
    wtfile() {};
    wtfile(const wtfile &) = delete;
    wtfile &operator=(const wtfile &) = delete;
    // 判断文件是否打开
    bool isOpen() const;

    /**
     * @brief 打开文件
     *
     * @param filename 文件命，使用绝对路径
     * @param temp 是否启用临时文件,如test.txt,则临时文件为test.txt.temp
     * @param mode 打开文件的方式
     * @param buffer 是否启用缓冲区
     * @return true
     * @return false
     */
    bool open(const std::string &filename, bool temp = true, std::ios::openmode mode = std::ios::out,
              bool buffer = true);

    /**
     * @brief 格式化行写入，类似c风格的printf
     *
     * @tparam Arg 使用可变参数模板
     * @param fmt 格式化字符串
     * @param args 可变参数
     * @return true fout.good()表示在文件流中没有发生错误
     * @return false
     */
    template <class... Arg> bool writeLine(const std::string &fmt, Arg... args)
    {
        if (fout.is_open() == false)
        {
            return false;
        }
        std::string str;                   // 存放格式化后的字符串
        sFomat(str, fmt.c_str(), args...); // 格式化字符串，调用stringop.h中的sFomat函数
        fout << str << std::endl;          // 写入文件
        return fout.good();
    }

    // 重载<<运算符，让类能像ofstream一样使用
    template <class T> wtfile &operator<<(const T &data)
    {
        if (fout.is_open() == false)
        {
            return *this;
        }
        fout << data;
        return *this; // 返回自身，以便链式调用
    }

    // 把二进制数据写入文件，需要打开文件时使用std::ios::binary
    bool writeBin(void *data, size_t size);

    // 关闭文件并将临时文件改为正式文件
    void close();

    ~wtfile();
};

// 读文件的类
class rdfile
{
  private:
    std::ifstream fin;
    std::string filename;

  public:
    rdfile() {};
    rdfile(const rdfile &) = delete;
    rdfile &operator=(const rdfile &) = delete;
    // 判断文件是否打开
    bool isOpen() const;

    /**
     * @brief 打开文件
     *
     * @param filename 文件名，绝对路径
     * @param mode 打开文件的方式
     * @return true
     * @return false
     */
    bool open(const std::string &filename, std::ios::openmode mode = std::ios::in);

    /**
     * @brief 读文件中的一行（一段内容）
     *
     * @param str 读取到的内容
     * @param endSign 结束符
     * @return true
     * @return false
     */
    bool readLine(std::string &str, const std::string &endSign = "");

    // 重载>>运算符，让类能像ifstream一样使用
    template <class T> rdfile &operator>>(T &data)
    {
        if (fin.is_open() == false)
        {
            return *this;
        }
        fin >> data;
        return *this;
    }

    // 读取二进制数据，需要打开文件时使用std::ios::binary
    bool readBin(void *data, size_t size);

    // 关闭文件
    void close();

    ~rdfile();
};