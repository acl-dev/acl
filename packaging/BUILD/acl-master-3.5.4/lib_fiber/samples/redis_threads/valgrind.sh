#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./redis  -n 10 -c 10 -m 1
