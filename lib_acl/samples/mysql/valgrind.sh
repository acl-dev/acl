#!/bin/sh

valgrind --show-below-main=yes --show-reachable=yes --tool=memcheck --leak-check=yes -v \
./db_mysql -H /tmp/mysql.csmail.sock -N csmail_userdb -U csmail -P 6632857799266825679
