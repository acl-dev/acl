#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./tcp_keeper  -s 127.0.0.1:5300 -c 10 -n 1000
