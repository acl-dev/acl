#!/bin/sh

HOME_PATH=/opt/soft/acl-master
trap '' 1
ulimit -n 204800
ulimit -c unlimited
#$HOME_PATH/libexec/acl_master -v -c $HOME_PATH/conf -l $HOME_PATH/var/log/acl_master&
$HOME_PATH/libexec/acl_master -k -c $HOME_PATH/conf -l $HOME_PATH/var/log/acl_master&
