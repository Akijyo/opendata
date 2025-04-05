#该脚本用于启动平台的全部服务程序

#启动守护模块
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 10 /home/akijyo/桌面/code/c++/opendata/tools/bin/checkproc /temp/log/checkproc.log

#生成站点观测数据，一分钟一次
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 60 /home/akijyo/桌面/code/c++/opendata/bin/opendata /temp/data/数据集合/01中国气象站点数据/中国气象站点参数.ini /temp/output/surfdata /temp/log/mylog.log

#定期清理一次文件，五分钟一次，只保留0.02天的文件
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 300 /home/akijyo/桌面/code/c++/opendata/tools/bin/deletefile /temp/output/surfdata ".*" 0.02

#定期将不在记录日志的日志文件压缩，五分钟进行一次，仅仅对0.02天（30分钟）之前的日志文件操作
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 300 /home/akijyo/桌面/code/c++/opendata/tools/bin/gzipfile /temp/log/ ".*\\.log\\.\\d+\\b" 0.02

#从/temp/output/surfdata目录下载原始的气象观测数据文件，存放在/idcdata/surfdata目录下
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 30 /home/akijyo/桌面/code/c++/opendata/tools/bin/ftpgetfiles /temp/log/download.log /home/akijyo/桌面/code/c++/opendata/tools/others/ftpgetfiles.json

#定期清理/idcdata/surfdata目录下0.04天之前的文件
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 300 /home/akijyo/桌面/code/c++/opendata/tools/bin/deletefile /idcdata/surfdata ".*" 0.04

#把/temp/output/surfdata目录下的原始气象观测数据文件，上传到/temp/upload目录下
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 30 /home/akijyo/桌面/code/c++/opendata/tools/bin/ftpputfiles /temp/log/upload.log /home/akijyo/桌面/code/c++/opendata/tools/others/ftpputfiles.json

#定期清理/temp/upload目录下0.04天之前的文件
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 300 /home/akijyo/桌面/code/c++/opendata/tools/bin/deletefile /temp/upload ".*" 0.04