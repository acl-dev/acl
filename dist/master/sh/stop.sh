#!/bin/sh

HOME_PATH={install_path}
PID_FILE=$HOME_PATH/var/pid/acl_master.pid
if [ -f $PID_FILE ]; then
	echo "stoping acl-master service now ..."
	kill `sed 1q $PID_FILE`
	rm -f $PID_FILE
	echo "acl-master serivce stoped!"
else
	echo "acl-master service not running!"
fi

