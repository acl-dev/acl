#!/bin/sh

HOME_PATH={install_path}
trap '' 1

ulimit -c 10240
#$HOME_PATH/libexec/acl_master -v -c $HOME_PATH/conf -l $HOME_PATH/var/log/acl_master&
$HOME_PATH/libexec/acl_master -c $HOME_PATH/conf -l $HOME_PATH/var/log/acl_master&
