#!/bin/bash
# use by dnsadmin@qiyi.com

process="/opt/soft/acl-master/libexec/acl_master"
pidfile="/opt/soft/acl-master/var/pid/acl_master.pid"
pnum=`ps -ef|grep $process|grep -v grep|wc -l`

# 进程不存在，pid文件存在，则异常退出，需要cron拉起
if [ $pnum == 0 -a -f $pidfile ];then
    echo "acl_master is Abnormal stoped, start it"
    /sbin/service master start
else
    echo "acl_master is running"
fi
