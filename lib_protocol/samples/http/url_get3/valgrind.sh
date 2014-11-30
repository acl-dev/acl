#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./url_get3 -t GET -r http://www.sina.com.cn -f dump.txt
