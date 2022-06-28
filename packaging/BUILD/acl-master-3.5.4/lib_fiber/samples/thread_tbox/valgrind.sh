#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./thread_tbox -n 100000 -c 2
