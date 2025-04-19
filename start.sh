#该脚本用于启动平台的全部服务程序

#启动守护模块
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 10 /home/akijyo/桌面/code/c++/opendata/tools/bin/checkproc /temp/log/checkproc.log

#生成站点观测数据，一分钟一次
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 60 /home/akijyo/桌面/code/c++/opendata/bin/crtsurfdata /temp/data/数据集合/01中国气象站点数据/中国气象站点参数.ini /temp/output/surfdata /temp/log/mylog.log

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

#启动TCP文件服务器
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 20 /home/akijyo/桌面/code/c++/opendata/tools/bin/fileserver /temp/log/fileserver.log

#启动TCP文件上传程序，把/temp/upload目录下的文件上传/temp/tcpupload目录下
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 30 /home/akijyo/桌面/code/c++/opendata/tools/bin/tcpputfiles /temp/log/tcpupload.log /home/akijyo/桌面/code/c++/opendata/tools/others/tcpputfiles.json

#启动TCP文件下载程序，把/idcdata/surfdata目录下的文件下载到/idcdata/tcpsurfdata目录下
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 30 /home/akijyo/桌面/code/c++/opendata/tools/bin/tcpgetfiles /temp/log/tcpdownload.log /home/akijyo/桌面/code/c++/opendata/tools/others/tcpgetfiles.json

#定期清理/temp/tcpupload目录下0.04天之前的文件
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 300 /home/akijyo/桌面/code/c++/opendata/tools/bin/deletefile /temp/tcpupload ".*" 0.04

#定期清理/idcdata/tcpsurfdata目录下0.04天之前的文件
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 300 /home/akijyo/桌面/code/c++/opendata/tools/bin/deletefile /idcdata/tcpsurfdata ".*" 0.04

#定期将站点参数文件内容更新到数据库中
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 180 /home/akijyo/桌面/code/c++/opendata/bin/obtcodetodb /temp/data/数据集合/01中国气象站点数据/中国气象站点参数.ini /home/akijyo/桌面/code/c++/opendata/database/dbsettings.json /temp/log/obtcodetodb.log

#定期将站点观测数据文件内容更新到数据库中，程序生成的观测数据文件最终会运送到/idcdata/tcpsurfdata(下载操作)，入库程序每运行一次都会把目录下已经处理的文件清空
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 20 /home/akijyo/桌面/code/c++/opendata/bin/obtmindtodb /idcdata/tcpsurfdata /home/akijyo/桌面/code/c++/opendata/database/dbsettings.json /temp/log/obtmindtodb.log

# 定期清理数据库中两小时前的数据，shell脚本obtmindtodb.sh里面利用linux的mysql客户端执行了/home/akijyo/桌面/code/c++/opendata/service/obtmindtodb/目录下的deletetable.sql文件，
# 里面的语句DELETE FROM T_ZHOBTMIND WHERE ddatetime < DATE_SUB(NOW(), INTERVAL 2 HOUR);会删除表T_ZHOBTMIND中两小时前的数据
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 360 /home/akijyo/桌面/code/c++/opendata/service/obtmindtodb/obtmindtodb.sh

# 每隔一小时将T_ZHOBTCODE表中的数据抽出出来，放入/idcdata/dminddata/dmindcode目录下，使用全量抽取的方法
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 3600 /home/akijyo/桌面/code/c++/opendata/tools/bin/dminingmysql /temp/log/dminingCode.log /home/akijyo/桌面/code/c++/opendata/tools/others/dminingCode.json

# 每隔一分钟将T_ZHOBTMIND表中的数据抽出出来，放入/idcdata/dminddata/dmindmind目录下，使用增量抽取的方法，按照自增字段keyid递增值进行抽取
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 60 /home/akijyo/桌面/code/c++/opendata/tools/bin/dminingmysql /temp/log/dminingMind.log /home/akijyo/桌面/code/c++/opendata/tools/others/dminingMind.json

#定期清理/idcdata/dminddata/dmindcode目录下1天之前的文件
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 300 /home/akijyo/桌面/code/c++/opendata/tools/bin/deletefile /idcdata/dminddata/dmindcode ".*\\.json\\b" 1

#定期清理/idcdata/dminddata/dmindmind目录下0.06天之前的文件
/home/akijyo/桌面/code/c++/opendata/tools/bin/processctrl 300 /home/akijyo/桌面/code/c++/opendata/tools/bin/deletefile /idcdata/dminddata/dmindmind ".*\\.json\\b" 0.06