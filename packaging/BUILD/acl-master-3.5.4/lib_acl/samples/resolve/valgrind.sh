#!/bin/sh
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./resolve -n www.baidu.com
