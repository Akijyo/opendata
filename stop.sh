#停止一切服务的程序

#先停止调度程序
killall -9 processctrl

#停止站点服务程序
killall opendata

#停止文件清理程序
killall deletefile

#停止压缩日志文件程序
killall gzipfile

sleep 5 

killall -9 opendata
killall -9 deletefile
killall -9 gzipfile
