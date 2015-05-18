#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./pkg_server -k
