#!/bin/sh

HOME_PATH={install_path}
PID_FILE=$HOME_PATH/var/pid/acl_master.pid
kill `sed 1q $PID_FILE`
rm -f $PID_FILE

