#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a set -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a setex -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a setnx -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a append -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a get -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a getset -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a strlen -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a mset -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a mget -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a msetnx -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a setrange -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a getrange -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a setbit -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a getbit -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a bitcount -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a bitop_and -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a bitop_or -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a bitop_xor -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a incr -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a incrby -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a incrbyfloat -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a decr -n 10
valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a decrby -n 10

valgrind --tool=memcheck --leak-check=yes -v ./redis_string -s 127.0.0.1:6379 -a all -n 10
