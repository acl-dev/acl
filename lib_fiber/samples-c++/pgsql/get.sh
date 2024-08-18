#!/bin/sh
./pgsql  -c 10 -n 100 -f /home/zsx/download/db/postgresql/postgresql-9.6.2/src/interfaces/libpq/libpq.so \
	-s 127.0.0.1:26257 -o get -u root
