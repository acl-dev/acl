#!/bin/sh
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./cache2 -n 10 -t 10
