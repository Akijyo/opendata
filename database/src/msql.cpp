#include "../include/msql.h"
#include <cstddef>
#include <iostream>
#include <mysql/mysql.h>
#include <optional>
#include <random>
#include <string>
using namespace std;
void mysql::build_column_map() //
{
    if (this->fields == nullptr)
    {
        return;
    }
    for (int i = 0; i < num_fields; i++)
    {
        column_map[fields[i].name] = i;
    }
}

mysql::mysql()
{
    this->con = mysql_init(nullptr);
    this->aliveTime = std::chrono::steady_clock::now();
    if (this->con == nullptr)
    {
        this->last_errno_ = 0;
        this->last_error_ = "MySQL initialization failed";
        return;
    }
}
mysql::~mysql()
{
    if (this->res != nullptr)
    {
        mysql_free_result(this->res);
    }
    if (this->con != nullptr)
    {
        mysql_close(this->con);
    }
}

bool mysql::connect(const std::string host, const std::string user, const std::string passwd, const std::string db,
                    const unsigned int port)
{
    con = mysql_real_connect(con, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr,
                             0); // 调用mysql_real_connect函数连接数据库
    if (con == nullptr)
    {
        this->last_errno_ = mysql_errno(con);
        this->last_error_ = mysql_error(con);
        return false;
    }
    mysql_set_character_set(con, "utf8mb4"); // 设置连接使用的编码utf8
    return true;
}

bool mysql::update(const std::string sql) // 执行增删改语句
{
    if (mysql_query(this->con, sql.c_str()) != 0)
    {
        this->last_errno_ = mysql_errno(con);
        this->last_error_ = mysql_error(con);
        return false;
    }
    return true;
}

bool mysql::query(const std::string sql)
{
    if (this->res != nullptr) // 若此次查询前结果集不为空，则释放结果集
    {
        mysql_free_result(this->res);
    }
    this->column_map.clear();                     // 清空原有的列名和列索引的映射
    if (mysql_query(this->con, sql.c_str()) != 0) // 执行查询语句
    {
        last_errno_ = mysql_errno(con);
        last_error_ = mysql_error(con);
        return false;
    }
    this->res = mysql_store_result(this->con);      // 获取查询结果集
    this->num_fields = mysql_num_fields(this->res); // 获取列数
    this->fields = mysql_fetch_fields(this->res);   // 获取列名（对应的结构体）
    build_column_map();                             // 构建列名和列索引的映射
    //this->next();                                   // 预先取下一行
    return true;
}

bool mysql::set_charset(const std::string charset)
{
    if (mysql_set_character_set(this->con, charset.c_str()) != 0)
    {
        this->last_errno_ = mysql_errno(con);
        this->last_error_ = mysql_error(con);
        return false;
    }
    return true;
}

bool mysql::next()
{
    if (this->res == nullptr)
    {
        return false;
    }
    this->current_row = mysql_fetch_row(this->res);      // 获取当前行(读取下一行)
    this->field_lengths = mysql_fetch_lengths(this->res); // 获取当前行每一列的数据长度
    return current_row != nullptr;
}

bool mysql::seek(int row_num)
{
    if (this->res == nullptr || row_num < 0 || row_num >= mysql_num_rows(this->res))
    {
        return false;
    }
    mysql_data_seek(this->res, row_num);                  // 跳转到结果集的指定行
    this->current_row = mysql_fetch_row(this->res);       // 获取当前行(读取下一行)
    this->field_lengths = mysql_fetch_lengths(this->res); // 获取当前行每一列的数据长度
    return current_row != nullptr;
}

optional<std::string> mysql::value(int col)
{
    if (this->current_row == nullptr || col < 0 || col >= this->num_fields) // 当前行为空或者索引越界
    {
        return nullopt;
    }
    if (this->current_row[col] == nullptr)
    {
        return nullopt;
    }
    return string(this->current_row[col], this->field_lengths[col]);
}

optional<std::string> mysql::value(const string &col_name)
{
    if (this->current_row == nullptr ||
        this->column_map.find(col_name) == this->column_map.end()) // 当前行为空或者字段值不存在
    {
        return nullopt;
    }
    unsigned int index = this->column_map[col_name];
    if (this->current_row[index] == nullptr)
    {
        return nullopt;
    }
    return string(this->current_row[index], this->field_lengths[index]);
}

int mysql::get_int(int col)
{
    auto value = this->value(col);
    if (value == nullopt)
    {
        return 0;
    }
    try
    {
        return std::stoi(value.value());
    }
    catch (const std::exception &e)
    {
        //std::cerr << "转换int类型出错" << std::endl;
        return 0;
    }
}

double mysql::get_double(int col)
{
    auto value = this->value(col);
    if (value == nullopt)
    {
        return 0.0;
    }
    try
    {
        return std::stod(value.value());
    }
    catch (const std::exception &e)
    {
        //std::cerr << "转换double类型出错" << std::endl;
        return 0.0;
    }
}

int mysql::columns()
{
    return this->num_fields;
}

string mysql::column_name(int col)
{
    if (col < 0 || col > this->num_fields)
    {
        //std::cerr << "列索引越界" << std::endl;
        return "";
    }
    return this->fields[col].name;
}

void mysql::start_transaction() // 开启事务
{
    mysql_autocommit(this->con, 0); // 关闭自动提交
}

void mysql::commit() // 提交事务
{
    mysql_commit(this->con);        // 提交事务
    mysql_autocommit(this->con, 1); // 开启自动提交
}

void mysql::rollback() // 回滚事务
{
    mysql_rollback(this->con);      // 回滚事务
    mysql_autocommit(this->con, 1); // 开启自动提交
}