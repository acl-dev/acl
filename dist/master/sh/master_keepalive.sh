#!/bin/bash
# use by dnsadmin

process="/opt/soft/acl-master/libexec/acl_master"
pidfile="/opt/soft/acl-master/var/pid/acl_master.pid"
pnum=`/sbin/pidof /opt/soft/acl-master/libexec/acl_master|wc -l`

# Try to start acl_master if it has been stopped abnormally. This shell will
# be called every minute by crond.
if [ $pnum == 0 -a -f $pidfile ]; then
    echo "acl_master stopped abnormally, try to start it."
    /sbin/service acl-master start
else
    echo "acl_master is running"
fi
