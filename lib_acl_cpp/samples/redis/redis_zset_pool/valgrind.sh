#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./redis_zset_pool -s 127.0.0.1:6369 -a all -n 10 -c 10 -l 10240 -b 1024 -S
