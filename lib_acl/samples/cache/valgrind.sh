#!/bin/sh
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./cache -n 10 -t 10
