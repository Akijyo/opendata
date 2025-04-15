// 这个文件是数据库接口
#pragma once
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>

class IConnection:public std::enable_shared_from_this<IConnection>
{
  protected:
    std::chrono::steady_clock::time_point aliveTime; // 连接存活时间，用于在连接池中判断是否超时
    std::unordered_map<std::string, int> column_map; // 列名和列索引的映射，在后面用于按字段去查找值
    virtual void build_column_map() = 0;             // 构建列名和列索引的映射
  public:
    // 错误处理
    std::string last_error_;
    int last_errno_ = 0;

    IConnection()
    {
        this->aliveTime = std::chrono::steady_clock::now();
    }
    IConnection(const IConnection &) = delete;
    IConnection &operator=(const IConnection &) = delete;
    virtual ~IConnection() = default;

    // 数据库基本操作
    virtual bool connect(const std::string host, const std::string user, const std::string passwd, const std::string db,
                         const unsigned int port) = 0; // 链接数据库
    virtual bool update(const std::string sql) = 0;    // 执行增删改语句
    virtual bool query(const std::string sql) = 0;     // 执行查询语句

    // 连接池时间操作
    void refreshTime() // 刷新时间
    {
        this->aliveTime = std::chrono::steady_clock::now();
    }
    long long getAliveTime() // 获取存在时间
    {
        std::chrono::nanoseconds result = std::chrono::steady_clock::now() - this->aliveTime;
        std::chrono::milliseconds result2 = std::chrono::duration_cast<std::chrono::milliseconds>(result);
        return result2.count();
    }

    // 事务操作
    virtual void start_transaction() = 0; // 开启事务
    virtual void commit() = 0;            // 提交事务
    virtual void rollback() = 0;          // 回滚事务

    // 查询结果操作
    virtual bool next() = 0;                                                   // 结果集导航，移动到下一行
    virtual bool seek(int row_num) = 0;                                        // 跳转到结果集的指定行
    virtual std::optional<std::string> value(int col) = 0;                     // 获取查询结果中的某一列的值
    virtual std::optional<std::string> value(const std::string &col_name) = 0; // 获取查询结果中的某一列的值，按字段找值
    virtual int get_int(int col) = 0;                                          // 获取某一列int类型的值
    virtual double get_double(int col) = 0;                                    // 获取某一列double类型的值
    // 元数据
    virtual int columns() = 0;                    // 获取列数
    virtual std::string column_name(int col) = 0; // 获取列名（字段名）
};