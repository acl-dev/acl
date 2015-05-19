#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./pkg_client -s 127.0.0.1:1900 -c 10 -n 100 -l 10
