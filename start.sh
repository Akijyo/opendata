#该脚本用于启动平台的全部服务程序

#启动守护模块
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 10 /home/akijyo/桌面/code/c++/opendata/tools/bin/checkproc /temp/log/checkproc.log

#生成站点观测数据，一分钟一次
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 60 /home/akijyo/桌面/code/c++/opendata/bin/opendata /temp/data/数据集合/01中国气象站点数据/中国气象站点参数.ini /temp/output /temp/log/mylog.log
