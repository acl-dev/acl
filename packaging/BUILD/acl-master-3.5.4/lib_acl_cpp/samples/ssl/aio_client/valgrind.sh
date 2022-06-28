#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./aio_client -k -S -c 1 -n 10
