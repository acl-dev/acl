#!/bin/sh

HOME_PATH=/opt/soft/acl-master
PID_FILE=$HOME_PATH/var/pid/acl_master.pid
kill -HUP `sed 1q $PID_FILE`

