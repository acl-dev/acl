#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a set -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a setex -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a setnx -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a append -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a get -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a getset -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a mset -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a mget -n 10
