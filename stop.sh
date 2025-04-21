#停止一切服务的程序

#先停止调度程序
killall -9 processctrl

#停止站点服务程序
killall opendata

#停止文件清理程序
killall deletefile

#停止压缩日志文件程序
killall gzipfile

#停止下载文件程序
killall ftpgetfiles

#停止上传文件程序
killall ftpputfiles

#停止TCP文件相关程序
killall fileserver tcpgetfiles tcpputfiles

#停止数据库更新相关程序
killall obtcodetodb obtmindtodb obtmindtodb.sh

#停止数据抽取程序
killall dminingmysql

#停止数据入库模块
killall jsontodb

sleep 5 

killall -9 opendata
killall -9 deletefile
killall -9 gzipfile
killall -9 ftpgetfiles
killall -9 ftpputfiles
killall -9 fileserver tcpgetfiles tcpputfiles
killall -9 obtcodetodb obtmindtodb obtmindtodb.sh
killall -9 dminingmysql
killall -9 jsontodb
