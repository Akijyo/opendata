#停止一切服务的程序

#先停止调度程序
killall -9 processctrl

#停止站点服务程序
killall opendata

sleep 5 

killall -9 opendata
