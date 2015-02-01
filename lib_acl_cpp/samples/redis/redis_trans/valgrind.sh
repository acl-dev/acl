#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./redis_trans -s 127.0.0.1:6379 -a all -n 10
