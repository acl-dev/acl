#!/bin/sh

valgrind --tool=memcheck --leak-check=yes  -v ./session -s 127.0.0.1:11211 -n 1 -a redis
