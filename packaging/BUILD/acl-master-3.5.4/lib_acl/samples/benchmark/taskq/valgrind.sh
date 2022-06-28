#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./taskq -q 10 -c 10 -p 5 -n 10
