#!/bin/sh
./mysql  -c 10 -n 100 -f /home/zsx/work/oschina/svn.acl/resource/mysql-connector-c-6.1.6-src/libmysql/libmysqlclient.so \
	-s /var/lib/mysql/mysql.sock -o get -u root
