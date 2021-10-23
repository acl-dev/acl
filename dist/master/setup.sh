#!/bin/sh

###############################################################################
PATH=/bin:/usr/bin:/usr/sbin:/usr/etc:/sbin:/etc
tempdir="/tmp"

umask 022       

censored_ls() {
    ls "$@" | egrep -v '^\.|/\.|CVS|RCS|SCCS|linux\.d|solaris\.d|hp_ux\.d|example|service'
}               
        
compare_or_replace() {
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
guess_os() {
	os_name=`uname -s`
	os_type=`uname -p`
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

create_path()
{
	test -d $1 || mkdir -p $1 || {
		echo "can't mkdir $1"
		exit 1
	}
}

copy_file()
{
	for file in `censored_ls $2`
	do
		test -f $2/$file && {
			compare_or_replace $1 $2/$file $3/$file || {
				echo "copy file: $2/$file error"
				exit 1
			}
		}
	done
}

install_file()
{
	for file in `censored_ls $2`
	do
		rm -f $tempdir/junk2 || {
			echo "can't remove file: $tempdir/junk2"
			exit 1
		}
		test -f $2/$file && {
			cat $2/$file | sed -e 's;{install_path};'$INSTALL_PATH';;' >$tempdir/junk2 || {
				echo "can't create file: $tempdir/junk2"
				exit 1
			}
			compare_or_replace $1 $tempdir/junk2 $3/$file || {
				echo "can't move to file: $3/$file"
				exit 1
			}
		}
		rm -f $tempdir/junk2 || {
			echo "can't remove file: $tempdir/junk2"
			exit 1
		}
	done
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
SERVICE_PATH=$PREFIX_PATH$INSTALL_PATH/conf/service
LIBEXEC_PATH=$PREFIX_PATH$INSTALL_PATH/libexec
INIT_PATH=$PREFIX_PATH/etc/init.d/
SH_PATH=$PREFIX_PATH$INSTALL_PATH/sh
VAR_PATH=$PREFIX_PATH$INSTALL_PATH/var

###############################################################################
create_all_path()
{
	create_path $INSTALL_PATH
	create_path $BIN_PATH
	create_path $SBIN_PATH
	create_path $LIBEXEC_PATH
	create_path $SH_PATH
	create_path $CONF_PATH
	create_path $SERVICE_PATH
#	create_path $SERVICE_PATH/samples
	create_path $VAR_PATH
	create_path $VAR_PATH/log
	create_path $VAR_PATH/pid
	create_path $VAR_PATH/private
	create_path $VAR_PATH/public
	create_path $INIT_PATH

	chmod 700 $VAR_PATH/private
	chmod 1777 $VAR_PATH/log
}

copy_all_file()
{
	test -d bin/$RPATH && {
		copy_file a+x,go+rx bin/$RPATH $BIN_PATH
	}
	test -d sbin/$RPATH && {
		copy_file a+x,go+rx sbin/$RPATH $SBIN_PATH
	}
	test -d libexec/$RPATH && {
		copy_file a+x,go+rx libexec/$RPATH $LIBEXEC_PATH
	}
	install_file a+x,go-wrx sh $SH_PATH
	install_file a+x,go-wrx conf $CONF_PATH
	install_file a+x,go-wrx conf/service $SERVICE_PATH
#	install_file a+x,go-wrx conf/service/samples $SERVICE_PATH/samples
	install_file a+x,go-wrx init.d/ $INIT_PATH
}

guess_os
create_all_path
copy_all_file

###############################################################################
