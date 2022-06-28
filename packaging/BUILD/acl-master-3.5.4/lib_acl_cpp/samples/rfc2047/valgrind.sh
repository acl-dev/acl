#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes -v ./rfc2047 -e "中国人民"
valgrind --tool=memcheck --leak-check=yes -v ./rfc2047
