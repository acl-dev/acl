#!/bin/sh
./mysql  -c 10 -n 100 -f /home/zsx/work/oschina/svn.acl/resource/mysql-connector-c-6.1.6-src/libmysql/libmysqlclient.so \
	-s /var/lib/mysql/mysql.sock -o get -u root
# ./mysql -c 10 -n 100 -f ./libmysqlclient_r.so -o get -d -u acl_user -p 111111 -d acl_test_db -D
