#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./fiber_tbox -p 1 -c 1 -P 10 -C 10 -n 10000
