#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./redis_list -s 127.0.0.1:6379 -n 1000 -c -a lpush
echo "enter any key to continue"
read n
valgrind --tool=memcheck --leak-check=yes -v ./redis_list -s 127.0.0.1:6379 -n 1000 -c -a lrange

