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

sleep 5 

killall -9 opendata
killall -9 deletefile
killall -9 gzipfile
killall -9 ftpgetfiles
killall -9 ftpputfiles
