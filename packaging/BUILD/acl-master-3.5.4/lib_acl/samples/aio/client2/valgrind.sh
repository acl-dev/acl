#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v  ./aclient -N 127.0.0.1:53 -s test.zsx.com:30082 -c 100 -n 1000
