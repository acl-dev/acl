#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./dbpool -s /tmp/mysql_front.csmail.sock -d csmail_userdb -u csmail -p 6322873429366825679
