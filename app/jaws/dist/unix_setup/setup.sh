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
	echo "parameter not enougth($#)"
	echo "usage:$0 install_path"
	exit 1
fi

INSTALL_PATH=$1

case $INSTALL_PATH in
/*) ;;
no) ;;
*) echo Error: $INSTALL_PATH should be an absolute path name. 1>&2; exit 1;;
esac

echo Installing to $INSTALL_PATH...

BIN_PATH=$INSTALL_PATH/bin
SBIN_PATH=$INSTALL_PATH/sbin
CONF_PATH=$INSTALL_PATH/conf
SERVICE_PATH=$CONF_PATH/service
WWW_CONF=$CONF_PATH/www
MODULE_CONF=$CONF_PATH/module
PLUGIN_CONF=$CONF_PATH/plugin
LIBEXEC_PATH=$INSTALL_PATH/libexec
SH_PATH=$INSTALL_PATH/sh
VAR_PATH=$INSTALL_PATH/var
WWW_PATH=$INSTALL_PATH/www
MODULE_PATH=$INSTALL_PATH/module
PLUGIN_PATH=$INSTALL_PATH/plugin

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

	create_path $WWW_CONF
	create_path $WWW_CONF/vhost
	create_path $WWW_PATH
	create_path $WWW_PATH/htdocs
	create_path $WWW_PATH/htdocs/test.domain
	create_path $VAR_PATH
	create_path $VAR_PATH/log
	create_path $VAR_PATH/pid
	create_path $VAR_PATH/private
	create_path $VAR_PATH/public
	create_path $VAR_PATH/run

	create_path $MODULE_PATH
	create_path $MODULE_CONF

	create_path $PLUGIN_PATH
	create_path $PLUGIN_CONF

	chmod 700 $VAR_PATH/private
	chmod 1777 $VAR_PATH/log
	chmod 1777 $VAR_PATH/run
}

copy_all_file()
{
	copy_file a+x,go+rx bin $BIN_PATH
	copy_file a+x,go+rx sbin $SBIN_PATH
	copy_file a+x,go+rx libexec/$RPATH $LIBEXEC_PATH
	copy_file a+x,go+rx module/$RPATH  $MODULE_PATH
	copy_file a+x,go+rx plugin/$RPATH  $PLUGIN_PATH
	install_file a+x,go-wrx sh $SH_PATH
	install_file a+x,go-wrx conf $CONF_PATH
	install_file a+x,go-wrx conf/service $SERVICE_PATH
	install_file a+x,go-wrx conf/www $WWW_CONF
	install_file a+x,go-wrx conf/www/vhost $WWW_CONF/vhost
	install_file a+x,go-wrx www $WWW_PATH/htdocs
	install_file a+x,go-wrx conf/module $MODULE_CONF
	install_file a+x,go-wrx conf/plugin $PLUGIN_CONF
}

guess_os
create_all_path
copy_all_file

###############################################################################
