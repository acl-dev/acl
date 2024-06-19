#!/bin/sh
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./dns -h 8.8.8.8 -p 53 -d www.baidu.com
