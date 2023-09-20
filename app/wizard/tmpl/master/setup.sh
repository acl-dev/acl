#!/bin/sh

###############################################################################
PATH=/bin:/usr/bin:/usr/sbin:/usr/etc:/sbin:/etc
tempdir="/tmp"

umask 022       

function censored_ls() {
    ls "$@" | egrep -v '^\.|/\.|CVS|RCS|SCCS|linux\.d|solaris\.d|hp_ux\.d|example'
}               
        
function compare_or_replace() {
    (cmp $2 $3 >/dev/null 2>&1 && echo Skipping $3...) || { 
        echo Updating $3...
        rm -f $tempdir/junk || exit 1
        cp $2 $tempdir/junk || exit 1
        chmod $1 $tempdir/junk || exit 1
        mv -f $tempdir/junk $3 || exit 1
        chmod $1 $3 || exit 1
    }   
}    
###############################################################################
RPATH=
function guess_os() {
	os_name=`uname -s`
	os_type=`uname -m`
	case $os_name in
	Linux)
		case $os_type in
		x86_64)
			RPATH="linux64"
			;;
		i686)
			RPATH="linux32"
			;;
		aarch64)
			RPATH="aarch64"
			;;
		*)
			echo "unknown OS - $os_name $os_type"
			exit 1
			;;
		esac
		;;
	SunOS)
		case $os_type in
		i386)
			RPATH="sunos_x86"
			;;
		*)
			echo "unknown OS - $os_name $os_type"
			exit 1
			;;
		esac
		;;
	FreeBSD)
		RPATH="freebsd"
		;;
	Darwin)
		RPATH="macos"
		;;
	*)
		echo "unknown OS - $os_name $os_type"
		exit 1
		;;
	esac
}

function create_path() {
	test -d $1 || mkdir -p $1 || {
		echo "can't mkdir $1"
		exit 1
	}
}

function copy_file() {
	test -f $2 && {
		compare_or_replace $1 $2 $3 || {
			echo "copy file: $2 error"
			exit 1
		}
	}
}

function install_file() {
	rm -f $tempdir/junk2 || {
		echo "can't remove file: $tempdir/junk2"
		exit 1
	}
	test -f $2 && {
		cat $2 | sed -e 's;{install_path};'$INSTALL_PATH';;' >$tempdir/junk2 || {
			echo "can't create file: $tempdir/junk2"
			exit 1
		}
		compare_or_replace $1 $tempdir/junk2 $3 || {
			echo "can't move to file: $3"
			exit 1
		}
	}
	rm -f $tempdir/junk2 || {
		echo "can't remove file: $tempdir/junk2"
		exit 1
	}
}

###############################################################################
INSTALL_PATH=

if [ $# -lt 1 ]
then
#	echo "parameter not enougth($#)"
	echo "usage:$0 install_path"
	exit 1
fi

if [ $# -eq 2 ]
then
    PREFIX_PATH=$1
    INSTALL_PATH=$2
else
    INSTALL_PATH=$1
    PREFIX_PATH=
fi

case $INSTALL_PATH in
/*) ;;
no) ;;
*) echo Error: $INSTALL_PATH should be an absolute path name. 1>&2; exit 1;;
esac

echo Installing to $INSTALL_PATH...

BIN_PATH=$PREFIX_PATH$INSTALL_PATH/bin
SBIN_PATH=$PREFIX_PATH$INSTALL_PATH/sbin
CONF_PATH=$PREFIX_PATH$INSTALL_PATH/conf
SH_PATH=$PREFIX_PATH$INSTALL_PATH/sh
VAR_PATH=$PREFIX_PATH$INSTALL_PATH/var

SERVICE_NAME=$<PROGRAM>
SERVICE_BIN=$SBIN_PATH/$SERVICE_NAME
SERVICE_CONF=$CONF_PATH/$SERVICE_NAME.cf
###############################################################################

function create_all_path() {
	create_path $INSTALL_PATH
	create_path $BIN_PATH
	create_path $SBIN_PATH
	create_path $SH_PATH
	create_path $CONF_PATH
	create_path $VAR_PATH
	create_path $VAR_PATH/log
	create_path $VAR_PATH/pid
	chmod 1777 $VAR_PATH/log
}

function copy_all_file() {
	copy_file a+x,go+rx $SERVICE_NAME $SERVICE_BIN
	install_file a+x,go-wrx $SERVICE_NAME.cf $SERVICE_CONF
}

MASTER_PATH=/opt/soft/acl-master
MASTER_CONF=$MASTER_PATH/conf
MASTER_SERVICES=$MASTER_CONF/services.cf
MASTER_CTL=$MASTER_PATH/bin/master_ctl

function add_master_service() {
	echo ""

	if [ ! -d $MASTER_CONF ]; then
		echo "$MASTER_CONF not exist!"
		return
	fi

	if [ -f $MASTER_SERVICES ]; then
		has=`cat $MASTER_SERVICES | grep $SERVICE_CONF | wc -l`
		if [ $has != 0 ]; then
			echo "Service for $SERVICE_CONF already in $MASTER_SERVICES!"
			return
		fi
	fi

	echo "$SERVICE_CONF" >> $MASTER_SERVICES
	echo "Service added for $SERVICE_CONF"

	if [ -f $MASTER_CTL ]; then
		echo "Start your service by running:"
		echo "$MASTER_CTL -f $SERVICE_CONF -a start"
	fi
}

guess_os
create_all_path
copy_all_file
add_master_service

###############################################################################
