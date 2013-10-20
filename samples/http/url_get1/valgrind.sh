#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./url_get1 -t GET -r http://www.sina.com.cn/ -f url.dump
