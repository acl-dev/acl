#!/bin/sh

HOME_PATH={install_path}
PROG_NAME=acl_master
PID_FILE=$HOME_PATH/var/pid/acl_master.pid
EXE_PATH=$HOME_PATH/libexec/$PROG_NAME
CONF_PATH=$HOME_PATH/conf
LOG_PATH=$HOME_PATH/var/log/acl_master

RUNNING="no"
PID=0
OSTYPE=

guess_os()
{
	os_name=`uname -s`
	os_type=`uname -p`
	case $os_name in
	Linux)
		OSTYPE="linux"
		;;
	SunOS)
		OSTYPE="sunos"
		;;
	FreeBSD)
		OSTYPE="freebsd"
		;;
	*)
		echo "unknown OS - $os_name $os_type"
		exit 1
		;;
	esac
}

check_proc()
{
	guess_os

	if [ ! -f $PID_FILE ]; then
		RUNNING="no"
		return
	fi

	PID=`sed 1q $PID_FILE | awk '{print $NF}'`
	if [ ! -d "/proc/$PID" ]; then
		RUNNING="no"
		return
	fi

	if [ "$OSTYPE" != "linux" ]; then
		RUNNING="yes"
		return
	fi

	link_path=`ls -l /proc/$PID/exe | awk '{print $NF}'`
	if [ "$link_path" = "$EXE_PATH" ]; then
		RUNNING="yes"
	fi
}

start()
{
	trap '' 1
	ulimit -c unlimited
	ulimit -n 204800

	check_proc
	if [ "$RUNNING" = "yes" ]; then
		echo "$PROG_NAME (pid=$PID) running ..."
		exit 1
	fi

	echo "starting $PROG_NAME ..."

	$EXE_PATH -c $CONF_PATH -l $LOG_PATH &

	sleep 1

	check_proc
	if [ "$RUNNING" != "yes" ]; then
		echo "start $PROG_NAME failed!"
		exit 1
	fi

	echo "$PROG_NAME started!"
}

stop()
{
	check_proc
	if [ "$RUNNING" = "yes" ]; then
		if [ "$PID" -eq 0 ]; then
			echo "$PROG_NAME: pid($PID) invalid"
			exit 1
		fi
		echo "stoping $PROG_NAME now ..."
		kill $PID
		rm -f $PID_FILE
		echo "$PROG_NAME stoped!"
	else
		echo "$PROG_NAME not running!"
	fi
}

reload()
{
	check_proc
	if [ "$RUNNING" = "yes" ]; then
		if [ "$PID" -eq 0 ]; then
			echo "$PROG_NAME: pid($PID) invalid"
			exit 1
		fi
		kill -HUP $PID
		echo "$PROG_NAME reloaded!"
	else
		echo "$PROG_NAME not running!"
	fi
}

status()
{
	check_proc
	if [ "$RUNNING" = "yes" ]; then
        PID=`sed 1q $PID_FILE | awk '{print $NF}'`
        echo "$PROG_NAME running ($PID)"
	else
		echo "$PROG_NAME not running!"
	fi
}


# See how we were called.
case "$1" in
	start)
		start
		;;
	stop)
		stop
		;;
	reload)
		reload
		;;
	status)
	    status	
		;;
	*)
		echo $"Usage: $0 {start|stop|reload}"
		;;
esac
