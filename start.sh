#该脚本用于启动平台的全部服务程序

#启动守护模块
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 10 /home/akijyo/桌面/code/c++/opendata/tools/bin/checkproc /temp/log/checkproc.log

#生成站点观测数据，一分钟一次
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 60 /home/akijyo/桌面/code/c++/opendata/bin/opendata /temp/data/数据集合/01中国气象站点数据/中国气象站点参数.ini /temp/output /temp/log/mylog.log

#定期清理一次文件，五分钟一次，只保留0.02天的文件
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 300 /home/akijyo/桌面/code/c++/opendata/tools/bin/deletefile /temp/output/ ".*" 0.02

#定期将不在记录日志的日志文件压缩，五分钟进行一次，仅仅对0.02天（30分钟）之前的日志文件操作
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 300 /home/akijyo/桌面/code/c++/opendata/tools/bin/gzipfile /temp/log/ ".*\\.log\\.\\d+\\b" 0.02
