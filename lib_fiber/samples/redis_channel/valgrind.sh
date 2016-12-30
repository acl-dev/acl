#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./redis -c 100 -n 100 -w 10
