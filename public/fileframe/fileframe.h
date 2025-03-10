#include "../cpublic.h"
#include "../timeframe/timeframe.h"
// 这个操作框架混用了linuxC和C++17的文件操作，后有待改进为统一的C++17文件操作
// 统一为c++17的文件操作将会功能更完善，更加简洁



/**
 * @brief 根据提供的文件的 绝对路径的目录路径或者文件路径逐级创建目录
 *
 * @param dir 目录或文件的绝对路径
 * @param isFile 路径是否是文件路径，默认为false
 * @return true 操作成功
 * @return false 操作失败，失败原因有：1.磁盘空间不足 2.权限不足 3.路径有误，不合法
 */
bool newdir(const std::string &dir, bool isFile = false);

/**
 * @brief 重命名绝对路径的文件或目录,类似mv命令，用于替代rename函数
 *
 * @param oldname 旧的文件或目录的绝对路径，如/old/file.cpp
 * @param newname 新的文件或目录的绝对路径，若不存在会创建如/new/file.cpp
 * @return true 操作成功
 * @return false 操作失败，可能原因为：1.权限不足 2.磁盘空间不足 3.路径有误，不合法
 */
bool renamefile(const std::string &oldname, const std::string &newname);

/**
 * @brief 复制文件，类似cp命令，用于替代copy函数
 * 对于目录复制，如果执行：copyfile("/temp/aaa/2aaa", "/temp/bbb")
 * 则会将2aaa目录下的所有文件复制到bbb目录下，若bbb目录不存在则会创建
 * @param src 源文件的绝对路径
 * @param dest 复制目标的绝对路径，若不存在会创建
 * @return true 操作成功，复制成功后的文件与源文件内容一致
 * @return false 操作失败，可能得原因是：1.权限不足 2.磁盘空间不足 3.路径有误，不合法
 */
bool copyfile(const std::string &src, const std::string &dest);

/**
 * @brief 删除指定绝对路径的文件
 * !不能删目录
 * @param file 要删除的文件名
 * @return true 操作成功
 * @return false 操作失败，可能原因：1.权限不足 2.路径有误，不合法或不存在
 */
bool deletefile(const std::string &file);

/**
 * @brief 读取文件的大小，单位为字节
 *
 * @param file 绝对路径的文件
 * @return int 文件的大小，单位为字节，若操作失败则返回-1，代表没有权限或者文件不存在或者路径有误
 */
int fileSize(const std::string &file);

/**
 * @brief 获取指定绝对路径的文件的最后修改时间
 *
 * @param file 指定绝对路径的文件
 * @param time 返回文件的最后修改时间
 * @param type 时间的类型
 * @return true 操作成功
 * @return false 操作失败，可能得原因是：1.权限不足 2.路径有误，不合法或不存在
 */
bool fileTime(const std::string &file, std::string &time, TimeType type = TimeType::TIME_TYPE_ONE);
bool fileTime(const std::string &file, char *time, TimeType type = TimeType::TIME_TYPE_ONE);

/**
 * @brief 设置文件的修改时间
 *
 * @param file 指定绝对路径的文件
 * @param time 要设置成的时间
 * @param type 时间的类型，一定要传入年月日时分秒全齐的时间，这部分在timeframe中有处理
 * @return true 操作成功
 * @return false 操作失败，可能得原因是：1.权限不足 2.路径有误，不合法或不存在 3.时间格式不正确
 */
bool setFileTime(const std::string &file, const std::string &time, TimeType type = TimeType::TIME_TYPE_ONE);