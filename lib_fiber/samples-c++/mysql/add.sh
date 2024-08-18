#!/bin/sh
./mysql  -c 10 -n 100 -f /home/zsx/work/oschina/svn.acl/resource/mysql-connector-c-6.1.6-src/libmysql/libmysqlclient.so \
	-s /var/lib/mysql/mysql.sock -o add -u root

# ./mysql -c 10 -n 100 -f ./libmysqlclient_r.so -o add -d -u acl_user -p 111111 -d acl_test_db -D
