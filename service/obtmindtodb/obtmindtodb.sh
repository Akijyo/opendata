#!/bin/bash

# 连接参数从 dbsettings.json 获取
HOST="127.0.0.1"
PORT=3307
USER="root"
PASSWD="13690919281qq"
DB="idc"

# 执行删除操作，将所有输出重定向到/dev/null
mysql -h $HOST -P $PORT -u $USER -p$PASSWD $DB < /home/akijyo/桌面/code/c++/opendata/service/obtmindtodb/deletetable.sql > /dev/null 2>&1