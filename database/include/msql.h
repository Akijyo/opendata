#pragma once
#include "Iconnection.h"
#include <mysql/mysql.h>

class mysql : public IConnection
{
  private:
    // MySQL 连接相关
    MYSQL *con = nullptr;

    // 结果集相关
    MYSQL_RES *res = nullptr;               // 保存结果集的指针
    MYSQL_ROW current_row = nullptr;        // 当前行
    int num_fields = 0;                     // 列数
    MYSQL_FIELD *fields = nullptr;          // 保存列名（字段名）的指针（数组）
    unsigned long *field_lengths = nullptr; // 保存那一行每一列的数据长度的指针（数组），用于取值时的string构造
    void build_column_map() override;       // 构建列名和列索引的映射

  public:
    mysql(); // 初始化mysql实例
    mysql(const mysql &) = delete;
    mysql &operator=(const mysql &) = delete;
    ~mysql(); // 释放mysql实例

    // 数据库基本操作
    bool connect(const std::string host, const std::string user, const std::string passwd, const std::string db,
                 const unsigned int port) override; // 链接数据库
    bool update(const std::string sql) override;    // 执行增删改语句
    bool query(const std::string sql) override;     // 执行查询语句
    bool set_charset(const std::string charset) override; // 设置字符集

    // 结果集处理操作
    bool next() override;            // 结果集导航，移动到下一行
    bool seek(int row_num) override; // 跳转到结果集的指定行
    // 采用std::optional，方便处理数据库中的NULL值，而且可以区分NULL值和空值，如果值为空，则返回""空字符串，如果NULL值则返回std::nullopt，外部可以实现判断
    //
    std::optional<std::string> value(int col) override;                     // 获取查询结果中的某一列的值
    std::optional<std::string> value(const std::string &col_name) override; // 获取查询结果中的某一列的值，按字段名找值
    int get_int(int col) override;                                          // 获取某一列int类型的值
    double get_double(int col) override;                                    // 获取某一列double类型的值
    // 元数据
    int columns() override;                    // 获取列数
    std::string column_name(int col) override; // 获取列名（字段名）

    // 事务操作
    void start_transaction() override; // 开启事务
    void commit() override;            // 提交事务
    void rollback() override;          // 回滚事务
};